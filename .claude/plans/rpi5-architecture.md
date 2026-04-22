# Raspberry Pi 5 — Architecture & Folder Structure

## Context

AutoMate V2 is a human-car interaction IoT system. The Pi5 acts as the central orchestrator between three planes:

1. **Telemetry plane (passive, STM32 → Pi5 → ESP32/web)**: STM32 sniffs CAN bus via MCP2551, decodes frames, and pushes them to Pi5 over UART. Pi5 broadcasts to a local web dashboard (WebSocket) and forwards selected events to ESP32 (OLED mini-bot).

2. **Diagnostic plane (active, web → Pi5 → STM32 → OBD)**: user connects to Pi5 local WiFi hotspot, opens browser, issues OBD-II requests (read/clear DTC, live PID polling, read VIN, actuator test). Pi5 forwards request to STM32 over UART; STM32 transmits an OBD-II request frame to the vehicle's OBD port and returns the ECU response.

3. **Display plane**: Pi5 → ESP32 over UART; ESP32 drives OLED animation.

The `raspberry_pi5/` folder is currently empty. This plan defines the architecture, folder layout, data flow, protocols, safety rules, and toolchain so implementation can start cleanly.

**Confirmed requirements**:
- Language: Python 3.11+, asyncio, single-process.
- STM32↔Pi5: UART, **full-duplex** (telemetry up + diag request/response).
- Pi5↔ESP32: UART, one-way (display commands down).
- Web: local server on Pi5, served over Pi5-hosted WiFi hotspot. Any device joining the hotspot can access `http://<pi-ip>:8000` — no password layer (physical access to the vehicle + hotspot = trust boundary).
- Persistence: rotating log files, no database.
- Diagnostic protocol: OBD-II Mode 01/03/04/09. **Note on actuator test**: standard OBD-II Mode 08 is rarely implemented by modern ECUs; real actuator tests typically require UDS (ISO 14229) service 0x2F, which is manufacturer-specific. This plan scopes actuator test to "OBD Mode 08 where supported; UDS 0x2F reserved as a follow-up once target vehicle/ECU is known".
- **Safety hard-lock**: any actuator test or clear-DTC command is refused unless `vehicle_speed == 0 AND engine_rpm == 0` (read from live telemetry within the last 500 ms).

## Architecture (Data Flow, Bi-Directional)

```
┌──────────┐ WS  ┌──────────────────────────────────────────────────────┐ UART ┌───────┐ CAN ┌─────┐
│ Browser  │◀──▶│                        RPi5                           │◀────▶│ STM32 │◀───▶│ ECU │
└──────────┘    │                                                        │      └───────┘     └─────┘
                │  ┌──────────────────┐       ┌────────────────────┐    │           │
                │  │  stm32_link      │──────▶│ inbound dispatcher │    │           │ UART
                │  │ (read+write)     │◀──────│                    │    │           ▼
                │  └────────┬─────────┘       └─────────┬──────────┘    │       ┌───────┐
                │           ▲                           │                │       │ ESP32 │
                │           │                   ┌───────┴───────┐        │       └───────┘
                │           │                   ▼               ▼        │
                │           │          ┌──────────────┐ ┌───────────────┐│
                │           │          │  telemetry   │ │  diag response│ │
                │           │          │   decoder    │ │    matcher    │ │
                │           │          └──────┬───────┘ └───────┬───────┘ │
                │           │                 │                 │         │
                │           │                 ▼                 │         │
                │           │        ┌─────────────────┐        │         │
                │           │        │    EventBus     │◀───────┘         │
                │           │        │ (asyncio.Queue) │                  │
                │           │        └────────┬────────┘                  │
                │           │                 │                           │
                │           │      ┌──────────┼───────────────────────┐   │
                │           │      ▼          ▼                       ▼   │
                │           │  ┌─────────┐ ┌──────────────┐  ┌────────────┴──┐
                │           │  │ state   │ │ WS broadcast │  │ esp32_writer  │
                │           │  │ machine │ └──────────────┘  └───────────────┘
                │           │  └────┬────┘                                   │
                │           │       ▼                                        │
                │           │  ┌────────────┐                                │
                │           │  │safety gate │ (checks speed==0 && rpm==0)    │
                │           │  └─────┬──────┘                                │
                │           │        ▼                                       │
                │           │  ┌──────────────────┐                          │
                │           │  │ DiagnosticService│◀─── WS incoming command  │
                │           │  │ (req/resp, corr  │                          │
                │           │  │  id, timeout)    │─────── request frame ────┘
                │           │  └──────────────────┘
                │           │       │
                │           │       ▼
                │           │   rotating logfile (all diag commands audited)
                └───────────┴───────────────────────────────────────────────┘
```

**Key concurrency primitives**:
- `EventBus` = `asyncio.Queue` for one-to-many telemetry fanout (via per-subscriber queue copy).
- `DiagnosticService._pending: dict[corr_id, asyncio.Future]` for request→response matching with timeout.
- `stm32_link.send_queue: asyncio.Queue[bytes]` so reader and writer coroutines never contend on the pyserial port.

## Frame Format: STM32 ↔ Pi5 (duplex)

Because the link is now bi-directional and carries both telemetry and diagnostic traffic, every frame needs a type tag + correlation id.

```
┌───────┬────────┬──────┬───────────┬──────────┬─────────┬─────┬───────┐
│ 0xAA  │ LEN(2) │ TYPE │ CORR_ID(2)│ SEQ(1)   │ PAYLOAD │ CRC │ 0x55  │
│ SOF   │ LE u16 │ u8   │ LE u16    │ u8       │ N bytes │ CRC16│ EOF   │
└───────┴────────┴──────┴───────────┴──────────┴─────────┴─────┴───────┘
```

`TYPE` values:
- `0x01` TELEMETRY — STM32→Pi5, `CORR_ID=0`, payload = decoded CAN signal batch
- `0x10` DIAG_REQUEST — Pi5→STM32, `CORR_ID=n`, payload = OBD Mode + PID (+ actuator params)
- `0x11` DIAG_RESPONSE — STM32→Pi5, `CORR_ID=n` (echoes request), payload = ECU bytes
- `0x12` DIAG_NEGATIVE — STM32→Pi5, `CORR_ID=n`, payload = NRC (negative response code)
- `0x7F` ERROR / BUS_OFF notification from STM32

Full field layout and CRC polynomial live in `raspberry_pi5/src/automate/serial_io/protocol.md` (to be written with STM32 team).

## Folder Structure

```
raspberry_pi5/
├── README.md                     # quickstart, wiring, hotspot setup
├── pyproject.toml
├── requirements.txt
├── .env.example
├── .gitignore
│
├── config/
│   ├── config.yaml               # UART ports, baud, web port, safety flags
│   └── pids.yaml                 # OBD-II PID table: id → name, formula, unit
│
├── deploy/                       # host-side setup (NOT Python)
│   ├── hostapd.conf              # WiFi hotspot (SSID, WPA2)
│   ├── dnsmasq.conf              # DHCP for hotspot clients
│   └── automate.service          # systemd unit for auto-start
│
├── logs/                         # gitignored: app.log + diag_audit.log
│
├── src/automate/
│   ├── __init__.py
│   ├── __main__.py
│   ├── main.py                   # wire tasks into asyncio loop
│   ├── config.py                 # pydantic-settings loader
│   │
│   ├── serial_io/                # hardware boundary, duplex
│   │   ├── __init__.py
│   │   ├── stm32_link.py         # reader+writer coroutines, frame codec
│   │   ├── esp32_writer.py       # one-way writer to ESP32
│   │   └── protocol.md           # STM32↔Pi5 frame spec
│   │
│   ├── can/                      # passive telemetry decode
│   │   ├── __init__.py
│   │   ├── schema.py             # CanSignal, CarState dataclasses
│   │   └── decoder.py            # telemetry payload → CanSignal
│   │
│   ├── diag/                     # OBD-II request/response layer
│   │   ├── __init__.py
│   │   ├── modes.py              # enum: Mode01, Mode03, Mode04, Mode09, Mode08
│   │   ├── pids.py               # load pids.yaml, encode Mode01 requests
│   │   ├── dtc.py                # DTC bytes → "P0420" string + description
│   │   ├── codec.py              # request builder + response parser
│   │   └── service.py            # DiagnosticService: async request/response
│   │
│   ├── logic/                    # pure, no I/O
│   │   ├── __init__.py
│   │   ├── events.py             # EventBus (asyncio.Queue fanout)
│   │   ├── state.py              # CarState aggregator (speed, rpm, etc.)
│   │   ├── rules.py              # CarState → DisplayCommand
│   │   └── safety.py             # can_run_diag(state) guard
│   │
│   ├── display/                  # ESP32 command layer
│   │   ├── __init__.py
│   │   ├── commands.py           # DisplayCommand + serializer
│   │   └── protocol.md           # Pi5↔ESP32 frame spec
│   │
│   ├── web/                      # FastAPI app + WS
│   │   ├── __init__.py
│   │   ├── server.py             # app factory, mount static + routes
│   │   ├── routes.py             # /api/health, /api/dtc, /api/vin, etc.
│   │   ├── websocket.py          # /ws — bi-directional: tx telemetry, rx diag cmd
│   │   ├── schemas.py            # pydantic models for WS messages
│   │   └── static/               # index.html + dashboard.js + diagnostic.js
│   │
│   └── utils/
│       ├── __init__.py
│       ├── logging.py            # rotating handler + diag_audit logger
│       └── crc.py                # CRC16 for frame codec
│
└── tests/
    ├── conftest.py
    ├── test_telemetry_decoder.py
    ├── test_diag_codec.py        # Mode 03 DTC parsing, Mode 01 PID formulas
    ├── test_diag_service.py      # request/response matching, timeout
    ├── test_safety.py            # refuses command when speed>0
    └── test_state.py
```

### Layer responsibilities

| Layer | Touches hardware? | Has state? | Purpose |
|-------|-------------------|-----------|---------|
| `serial_io/` | yes (pyserial-asyncio) | no (frame codec only) | Read/write UART frames |
| `can/` | no | no | Decode telemetry payload |
| `diag/` | no | only in `service.py` (`_pending`) | OBD request/response codec + matcher |
| `logic/` | no | yes (`CarState`) | Business rules, safety gate |
| `display/` | no | no | Build ESP32 command bytes |
| `web/` | socket only | per-connection | REST + WS boundary |
| `utils/` | no | no | Logging, CRC |

### Why split `can/` and `diag/`

`can/` handles *passive* telemetry — continuous, best-effort, no reply expected. `diag/` handles *active* OBD-II — request/response, correlation IDs, timeouts, negative response codes (NRC). Different lifecycles, different state, different failure modes → different modules.

## WebSocket Message Schema (`web/schemas.py`)

Inbound (client → Pi5):
```json
{"type": "diag.request", "id": "client-uuid", "mode": 3}
{"type": "diag.request", "id": "client-uuid", "mode": 4}              // clear DTC
{"type": "diag.request", "id": "client-uuid", "mode": 1, "pid": "0C"}  // live RPM
{"type": "diag.request", "id": "client-uuid", "mode": 9, "pid": "02"}  // VIN
{"type": "diag.request", "id": "client-uuid", "mode": 8,
                         "tid": "...", "params": {...}}                 // actuator
```

Outbound (Pi5 → client):
```json
{"type": "telemetry", "signals": [{"name":"rpm","value":820,"unit":"rpm"}]}
{"type": "diag.response", "id":"client-uuid", "ok": true, "data": {...}}
{"type": "diag.response", "id":"client-uuid", "ok": false, "error": "safety_lock"}
{"type": "diag.response", "id":"client-uuid", "ok": false, "error": "timeout"}
{"type": "diag.response", "id":"client-uuid", "ok": false, "error": "negative_response", "nrc": "0x11"}
```

## Safety Gate

`logic/safety.py` exposes `can_run_diag(state: CarState, cmd: DiagRequest) -> Result`:
- Always allow: Mode 01 (read live), Mode 09 (read VIN), Mode 03 (read DTC).
- Require `speed == 0 AND rpm == 0`: Mode 04 (clear DTC), Mode 08 (actuator), and any future UDS 0x2F/0x31.
- Require telemetry age ≤ 500 ms (reject if stale — don't trust last-seen speed from 10 s ago).
- Every decision (allow *and* deny) is written to `logs/diag_audit.log` with timestamp, request, state snapshot.

## WiFi Hotspot Setup (Pi5 host-side)

Out of Python scope — configured via `deploy/hostapd.conf` + `deploy/dnsmasq.conf`. Pi5 runs as AP on `wlan0`, assigns IPs in `192.168.4.0/24`, web server binds `0.0.0.0:8000`. README has step-by-step `sudo apt install hostapd dnsmasq` instructions. No captive portal for MVP.

## Key Dependencies

Runtime:
- `pyserial-asyncio`
- `fastapi`, `uvicorn[standard]`
- `pydantic`, `pydantic-settings`
- `pyyaml`
- `structlog`

Dev:
- `pytest`, `pytest-asyncio`
- `ruff`, `mypy`

**Not using `python-obd`** — that library targets ELM327 USB dongles. We have a custom STM32 front-end, so we build our own thin OBD-II codec in `diag/` (it's small: Mode 01/03/04/09 is a few hundred lines).

## Config Schema (config/config.yaml)

```yaml
serial:
  stm32:
    port: /dev/ttyAMA0
    baud: 500000              # high baud for diag throughput
  esp32:
    port: /dev/ttyAMA2
    baud: 115200

web:
  host: 0.0.0.0
  port: 8000

diagnostic:
  request_timeout_ms: 2000
  max_retries: 1
  telemetry_max_age_ms: 500   # safety gate freshness

logging:
  level: INFO
  dir: logs
  app_file: app.log
  audit_file: diag_audit.log
  max_bytes: 10485760
  backup_count: 5
```

## Critical Files to Create (execution order)

1. `raspberry_pi5/pyproject.toml`, `requirements.txt`, `.env.example`, `.gitignore`
2. `raspberry_pi5/config/config.yaml`, `config/pids.yaml`
3. `raspberry_pi5/src/automate/utils/logging.py`, `utils/crc.py`
4. `raspberry_pi5/src/automate/config.py`
5. `raspberry_pi5/src/automate/can/schema.py`, `can/decoder.py`
6. `raspberry_pi5/src/automate/diag/modes.py`, `diag/pids.py`, `diag/dtc.py`, `diag/codec.py`
7. `raspberry_pi5/src/automate/logic/events.py`, `logic/state.py`, `logic/safety.py`, `logic/rules.py`
8. `raspberry_pi5/src/automate/display/commands.py`
9. `raspberry_pi5/src/automate/serial_io/stm32_link.py`, `serial_io/esp32_writer.py`, `serial_io/protocol.md`
10. `raspberry_pi5/src/automate/diag/service.py` (depends on stm32_link + safety)
11. `raspberry_pi5/src/automate/web/schemas.py`, `web/routes.py`, `web/websocket.py`, `web/server.py`
12. `raspberry_pi5/src/automate/web/static/` (index.html, dashboard.js, diagnostic.js)
13. `raspberry_pi5/src/automate/main.py`, `__main__.py`
14. `raspberry_pi5/tests/` (all test files)
15. `raspberry_pi5/deploy/hostapd.conf`, `deploy/dnsmasq.conf`, `deploy/automate.service`
16. `raspberry_pi5/README.md` (wiring, hotspot setup, how to run)
17. Update root `.gitignore` — add `raspberry_pi5/logs/`, `raspberry_pi5/.env`

## Verification Plan

**Dev machine (no hardware, no hotspot)**:
```bash
cd raspberry_pi5
pip install -e ".[dev]"
pytest                              # telemetry decoder, diag codec, safety, service
ruff check src tests
mypy src
```

**On Pi5 (hardware, hotspot active)**:
```bash
sudo systemctl enable --now hostapd dnsmasq automate
journalctl -u automate -f           # watch app boot

# Phone joins Pi5 hotspot → browser → http://192.168.4.1:8000
# Dashboard shows live RPM/speed streaming via WS → OK, telemetry path works.
#
# Click "Read DTCs" → WS diag.request (mode=3) → response rendered with P-codes → OK.
# Click "Clear DTCs" while engine running → response "safety_lock" → OK, guard works.
# Turn engine off → retry → response ok=true → OK.
# tail -f logs/diag_audit.log → each command logged with state snapshot.
```

**End-to-end smoke tests**:
1. Telemetry: start engine → dashboard RPM updates continuously; ESP32 OLED reacts.
2. DTC read: induce a fault (disconnect O2 sensor) → "Read DTCs" shows P013x.
3. Safety gate: engine running, try "Clear DTCs" → rejected; engine off → accepted.
4. Timeout: pull STM32 UART mid-request → WS returns `error: "timeout"` after 2 s.
5. Concurrent load: stream Mode 01 PIDs at 10 Hz while telemetry runs → no frame loss (check `diag_audit.log` sequence gaps).

## Out of Scope (follow-up plans)

- **UDS (ISO 14229)**: needed for manufacturer-specific actuator test, ECU coding, security access. Requires ISO-TP and per-OEM knowledge. Add once target vehicle known.
- **HTTPS / TLS**: hotspot is LAN-only for now; add self-signed cert + browser warning if threat model changes.
- **Multi-user / roles**: single-user trust model sufficient for MVP.
- **Frontend framework**: start with vanilla HTML+JS; upgrade to React/Vue if UI complexity grows.
- **Captive portal**: auto-redirect to dashboard when joining hotspot — nice-to-have.
- **Data export**: CSV/JSON export of diag audit log for workshop records.
