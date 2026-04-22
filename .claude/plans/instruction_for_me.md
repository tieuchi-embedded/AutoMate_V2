



Cấu trúc thư mục raspberry_pi5/ — giải thích chi tiết

raspberry_pi5/
├── README.md                     ← hướng dẫn chạy, wiring, setup hotspot
├── pyproject.toml                ← metadata package + deps + config ruff/mypy
├── requirements.txt              ← pinned deps để pip install trên Pi
├── .env.example                  ← template biến môi trường (copy → .env)
├── .gitignore                    ← ignore logs/, .env, __pycache__/
│
├── config/                       ← CẤU HÌNH RUNTIME (YAML, không phải code)
│   ├── config.yaml               ← UART port, baud, web port, safety threshold
│   └── pids.yaml                 ← bảng tra OBD-II PID: 0x0C→RPM, 0x0D→speed...
│
├── deploy/                       ← SETUP HỆ ĐIỀU HÀNH (không phải Python)
│   ├── hostapd.conf              ← config WiFi hotspot (SSID, WPA2)
│   ├── dnsmasq.conf              ← DHCP server cấp IP cho client join hotspot
│   └── automate.service          ← systemd unit → auto-start khi Pi boot
│
├── logs/                         ← GITIGNORED — rotating file runtime
│   ├── app.log                   ← log chung của app
│   └── diag_audit.log            ← riêng: mọi lệnh diagnostic (bằng chứng pháp lý)
│
├── src/automate/                 ← MÃ NGUỒN PYTHON (src-layout)
│   │
│   ├── __init__.py
│   ├── __main__.py               ← cho phép `python -m automate`
│   ├── main.py                   ← entry: khởi tạo asyncio loop, spawn tasks
│   ├── config.py                 ← pydantic-settings load config.yaml + .env
│   │
│   ├── serial_io/                ← 【LAYER PHẦN CỨNG】 — chỉ layer này chạm UART
│   │   ├── stm32_link.py         ← duplex: read+write UART STM32, encode/decode frame
│   │   ├── esp32_writer.py       ← 1 chiều: gửi lệnh xuống ESP32
│   │   └── protocol.md           ← SPEC frame STM32↔Pi5 (byte layout, CRC)
│   │
│   ├── can/                      ← 【TELEMETRY PASSIVE】 — parse log CAN liên tục
│   │   ├── schema.py             ← dataclass: CanSignal, CarState
│   │   └── decoder.py            ← bytes → CanSignal (pure function)
│   │
│   ├── diag/                     ← 【OBD DIAGNOSTIC ACTIVE】 — request/response
│   │   ├── modes.py              ← enum Mode01/03/04/08/09
│   │   ├── pids.py               ← load pids.yaml, build Mode 01 request
│   │   ├── dtc.py                ← decode 2 byte DTC → "P0420" string
│   │   ├── codec.py              ← build request bytes / parse response bytes
│   │   └── service.py            ← ASYNC: gửi request, await response, timeout, retry
│   │
│   ├── logic/                    ← 【NGHIỆP VỤ THUẦN】 — zero I/O, zero HTTP
│   │   ├── events.py             ← EventBus (asyncio.Queue fanout nhiều subscriber)
│   │   ├── state.py              ← CarState: tổng hợp speed/rpm/temp... mới nhất
│   │   ├── safety.py             ← can_run_diag(state, cmd) → cho/từ chối
│   │   └── rules.py              ← CarState → DisplayCommand (state hiện gì lên OLED)
│   │
│   ├── display/                  ← 【LỆNH ESP32】
│   │   ├── commands.py           ← DisplayCommand dataclass + serializer
│   │   └── protocol.md           ← SPEC frame Pi5↔ESP32
│   │
│   ├── web/                      ← 【BOUNDARY WEB】 — HTTP + WebSocket
│   │   ├── server.py             ← FastAPI app factory, mount static + route
│   │   ├── routes.py             ← REST: /api/health, /api/dtc (read-only alt path)
│   │   ├── websocket.py          ← /ws: DUPLEX — tx telemetry, rx diag command
│   │   ├── schemas.py            ← pydantic model cho message WS inbound/outbound
│   │   └── static/               ← frontend vanilla
│   │       ├── index.html
│   │       ├── dashboard.js      ← hiển thị telemetry live
│   │       └── diagnostic.js     ← nút "Read DTC", "Clear DTC", v.v.
│   │
│   └── utils/                    ← 【CROSS-CUTTING】
│       ├── logging.py            ← rotating handler + audit logger riêng
│       └── crc.py                ← CRC16 dùng chung cho frame codec
│
└── tests/                        ← UNIT TEST — chạy trên laptop, không cần Pi
    ├── conftest.py
    ├── test_telemetry_decoder.py
    ├── test_diag_codec.py        ← encode Mode01 PID 0x0C → đúng bytes?
    ├── test_diag_service.py      ← giả lập STM32, test req/resp matching + timeout
    ├── test_safety.py            ← speed=50 + clear DTC → refused?
    └── test_state.py
Nguyên tắc thiết kế
1. Tách hardware boundary duy nhất ở serial_io/
Chỉ module này import pyserial_asyncio. Các layer khác nhận/gửi dữ liệu qua asyncio.Queue và dataclass — nên test được 100% trên laptop, không cần Pi.

2. Tách can/ ↔ diag/ — 2 bản chất khác nhau
can/	diag/
Chiều	1 chiều (vào)	2 chiều (req/resp)
Lifecycle	liên tục, best-effort	sự kiện, có timeout
State	stateless	có _pending: dict[corr_id, Future]
Lỗi	mất frame OK	phải match ID + retry + NRC
Gộp lại sẽ lẫn lộn concerns.

3. logic/ là core nghiệp vụ, không biết UART/HTTP
state.py, rules.py, safety.py đều là pure function hoặc class không dính I/O. Đây là phần quan trọng nhất để test — nếu logic sai thì hậu quả có thể là xe bị lệnh sai. Test dễ vì không cần mock serial/network.

4. web/ là presentation, không tự nghĩ
websocket.py chỉ: nhận message → parse bằng schemas.py → gọi diag.service → trả lời. Không có rule business nào ở đây.

5. config/ và deploy/ tách khỏi src/
config/ = runtime tunables, đổi không cần rebuild.
deploy/ = OS-level (hostapd, systemd), không phải Python code. Tách ra để dev trên laptop không nhầm.
6. logs/diag_audit.log tách riêng
Lệnh diagnostic có thể ảnh hưởng vật lý xe → cần audit trail riêng, không lẫn với app.log chi tiết. File riêng rotate theo policy khác.

7. tests/ song song với src/, không lồng
Layout src/ + tests/ chuẩn Python → pytest discover tự động, không đụng __init__.py lung tung.

Ánh xạ thư mục → data flow
Telemetry (passive):


STM32 ──▶ serial_io/stm32_link ──▶ can/decoder ──▶ logic/state
                                                       ├─▶ web/websocket (broadcast)
                                                       └─▶ logic/rules ─▶ display/commands ─▶ serial_io/esp32_writer ─▶ ESP32
Diagnostic (active):


Browser ──▶ web/websocket ──▶ web/schemas (parse) ──▶ logic/safety (gate)
                                                              │ pass
                                                              ▼
                                                    diag/service ──▶ diag/codec ──▶ serial_io/stm32_link ──▶ STM32 ──▶ OBD
                                                              ▲
                                                              │ response matched by corr_id
                                                    serial_io/stm32_link ◀── STM32 ◀── OBD
                                                              │
                                                              ▼
                                                    diag/codec (parse) ──▶ web/websocket ──▶ Browser
                                                              │
                                                              └─▶ utils/logging (audit)
Mỗi mũi tên = một import hoặc một message qua queue. Dependency graph là DAG, không có vòng → dễ đọc, dễ refactor, compile-time rõ ràng bằng mypy.

vẫn không cần ipc à, vậy pi hoạt động việc vừa gửi vừa nhận nhiều lớp như vậy bằng cơ chế gì
Cơ chế: asyncio event loop + cooperative multitasking
Không cần IPC vì tất cả task chạy trong cùng 1 process, cùng 1 thread, nhưng không block nhau nhờ I/O multiplexing.

1. Mô hình chạy

┌──────────────────────────────────────────────────────────┐
│                  1 OS Process                             │
│  ┌────────────────────────────────────────────────────┐  │
│  │          1 Thread (main)                            │  │
│  │  ┌──────────────────────────────────────────────┐  │  │
│  │  │       asyncio Event Loop                      │  │  │
│  │  │                                                │  │  │
│  │  │  Task_stm32_reader  ─┐                         │  │  │
│  │  │  Task_stm32_writer  ─┤                         │  │  │
│  │  │  Task_esp32_writer  ─┼─▶ chạy luân phiên       │  │  │
│  │  │  Task_ws_server     ─┤    tại mỗi `await`      │  │  │
│  │  │  Task_ws_client_1   ─┤                         │  │  │
│  │  │  Task_ws_client_N   ─┤                         │  │  │
│  │  │  Task_state_machine ─┘                         │  │  │
│  │  └──────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────┘  │
│                                                           │
│  kernel syscall: epoll_wait([fd_uart1, fd_uart2, fd_tcp,  │
│                               fd_tcp_client_1, ...])      │
└──────────────────────────────────────────────────────────┘
Chỉ 1 thread chạy code Python tại 1 thời điểm. Không có 2 task chạy song song thực sự. Chúng luân phiên nhau.

2. Vì sao không block dù chỉ 1 thread
Mấu chốt là I/O multiplexing của kernel (epoll trên Linux). Khi 1 task gọi await reader.read(n):

Task đó nói với event loop: "tôi đang đợi data trên fd này, đánh thức tôi khi có".
Event loop không đứng chờ — nó quay sang chạy task khác đang sẵn sàng.
Kernel theo dõi tất cả fd cùng lúc (UART1, UART2, mọi TCP socket WS). Khi 1 fd có data → loop được báo → đánh thức task tương ứng.
Nên CPU Python chỉ chạy khi có việc thật. Lúc không có data, loop "ngủ" trên epoll_wait() — zero CPU.

3. Cơ chế giao tiếp giữa các layer — asyncio.Queue
Không phải IPC. Là hàng đợi in-memory trong cùng process, lock-free về mặt logic vì chỉ 1 thread:


# logic/events.py
event_bus = asyncio.Queue(maxsize=1000)

# serial_io/stm32_link.py — producer
async def reader_task():
    while True:
        frame = await read_frame()     # yield khi chưa có data
        await event_bus.put(frame)     # yield nếu queue đầy

# logic/state.py — consumer  
async def state_task():
    while True:
        frame = await event_bus.get()  # yield khi queue rỗng
        car_state.update(frame)
Producer và consumer "thấy" cùng 1 object Python vì cùng process. Không serialize, không socket, không shared memory syscall. Chi phí ~microsecond.

4. Cơ chế request/response (phần diagnostic) — asyncio.Future
Đây là điểm tinh tế. Web gửi "read DTC" → phải đợi phản hồi từ xe → trả về browser. Nhưng UART là stream, response có thể về sau 500 ms, trong lúc đó task không được chặn cả hệ thống:


# diag/service.py
class DiagnosticService:
    _pending: dict[int, asyncio.Future] = {}
    _next_id = 0

    async def request(self, mode, pid) -> bytes:
        corr_id = self._next_id; self._next_id += 1
        fut = asyncio.get_event_loop().create_future()
        self._pending[corr_id] = fut
        
        await stm32_link.send(build_frame(corr_id, mode, pid))
        
        try:
            return await asyncio.wait_for(fut, timeout=2.0)
        finally:
            self._pending.pop(corr_id, None)

    # được gọi bởi stm32_link khi có DIAG_RESPONSE
    def on_response(self, corr_id, payload):
        if fut := self._pending.get(corr_id):
            fut.set_result(payload)
Task WS gọi request() → suspend ở await fut → event loop đi xử lý task khác (telemetry, client WS khác, v.v.). Khi STM32 trả về, stm32_link parse frame, match corr_id, gọi set_result() → task WS được đánh thức → trả kết quả về browser.

Kết quả: hàng chục request diagnostic đồng thời từ nhiều client vẫn chạy tốt với 1 thread.

5. Tại sao không cần lock
Vì chỉ 1 thread:

Giữa 2 điểm await, code Python chạy nguyên tử (không task nào chen vào).
Dictionary _pending đọc/ghi an toàn không cần Lock.
Counter _next_id tăng không cần atomic.
Đây là lợi thế lớn so với threading — không có race condition, không có deadlock từ lock sai thứ tự.

6. So sánh khi nào phải chuyển sang IPC/threading
Tình huống	asyncio đủ?	Cần gì thêm
Nhiều I/O đồng thời (UART, socket)	✅ Tuyệt vời	—
Tính toán nặng CPU (computer vision, crypto)	❌ Block loop	run_in_executor (thread) hoặc multiprocessing
Lib blocking không hỗ trợ async (OpenCV, numpy long op)	❌	run_in_executor
Cô lập crash (web lỗi không ảnh hưởng CAN logger)	❌	Tách process, dùng IPC (Unix socket, pipe)
Tận dụng nhiều core CPU	❌	multiprocessing hoặc nhiều process
Với Pi5 làm gateway I/O thuần (UART ↔ UART ↔ WebSocket), asyncio là match hoàn hảo: nó được thiết kế đúng cho use case "nhiều kết nối, I/O-bound, ít CPU".

7. Timeline minh họa
Giả sử trong 10 ms có: 1 frame telemetry từ STM32, 1 command từ web client, cần gửi 1 lệnh display cho ESP32.


t=0.0 ms  epoll báo UART_STM32 sẵn data
          → task_reader chạy → parse frame → put vào event_bus → await
t=0.3 ms  task_state chạy → get frame → update car_state → publish → await
t=0.4 ms  task_rules chạy → sinh DisplayCommand → put vào esp32_tx_queue → await
t=0.5 ms  task_esp32_writer chạy → get command → write() → await (đang gửi bytes)
t=0.6 ms  epoll báo WS_CLIENT_1 có data
          → task_ws chạy → parse "diag.request mode=3" → gọi diag.request()
          → diag.service gửi xuống STM32 → await Future
t=0.9 ms  task_ws_broadcast chạy → lấy telemetry mới → gửi cho 2 client WS → await
t=1.0 ms  epoll báo UART_STM32 có byte (DIAG_RESPONSE)
          → task_reader chạy → parse → gọi service.on_response(corr_id)
          → Future set_result → task_ws được schedule
t=1.2 ms  task_ws resume → format JSON → gửi xuống browser → await
t=1.3 ms  ...loop tiếp tục
Tất cả trong 1 thread, không lock, không IPC, tổng latency end-to-end browser→xe→browser dưới ~10 ms (chưa tính CAN bus).

Tóm lại: IPC chỉ cần khi phải chia process. Dự án này I/O-bound, logic thống nhất → 1 process asyncio là chuẩn. Nếu sau này thêm computer vision (Pi5 đọc camera ADAS chẳng hạn), lúc đó mới cần tách CV sang process riêng và giao tiếp qua Unix socket — nhưng đó là follow-up.




