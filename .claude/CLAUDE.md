# AutoMate V2 — CLAUDE.md

## Project Overview

Dự án giao tiếp giữa người và ô tô thông minh, ứng dụng IoT đọc log CAN từ ô tô bằng STM32, giải mã log và đưa thông tin về Raspberry Pi 5, Pi5 xử lí và đưa thông tin lên web server qua web socket, đồng thời gửi tín hiệu cho esp32, esp32 điều khiển màn hình OLED mini bot giao tiếp thân thiện với người lái
```
AutoMate_V2/
├── stm32/          # STM32 firmware
├── esp32/          # ESP32 firmware (ESP-IDF or Arduino)
├── raspberry_pi5/  # Python / Linux applications
├── 3D_model/       # Mechanical design files
└── docs/           # Architecture, protocol specs
```

## Build & Flash

### STM32 (đã xong, không cần quan tâm đến code)
Chip STM32F103C8T6
Code,compile và program bằng STM32CubeIDE, low-level programming
Đọc và giải mã log can trên ô tô qua CAN transciever 2551, giải mã log và gửi về raspberry Pi5 qua uart

### ESP32 (Arduino IDE)
Dử dụng arduino ide, nhận log uart từ raspberry pi5 điều khiển màn oled và mini bot

### Raspberry Pi 5
như mô tả, và cần dựng kiến trúc thêm

## Architecture Constraints


## Coding Standards

### C / C++ (STM32, ESP32)


### Python (Raspberry Pi 5)

## Key Conventions


## HARD RULE



