# PRI_QMS S3 — Hệ Thống Xếp Hàng Thông Minh (ESP32-S3)

> **Firmware quản lý hàng đợi (Queue Management System)** chạy trên **ESP32-S3**, hỗ trợ kết nối WiFi, MQTT, in vé nhiệt, và giao diện web quản trị tích hợp.

---

## 📋 Mục Lục

- [Tổng Quan Dự Án](#tổng-quan-dự-án)
- [Kiến Trúc Hệ Thống](#kiến-trúc-hệ-thống)
- [Yêu Cầu Phần Cứng](#yêu-cầu-phần-cứng)
- [Cấu Trúc Thư Mục](#cấu-trúc-thư-mục)
- [Cấu Hình ESP32-S3](#cấu-hình-esp32-s3)
- [Hướng Dẫn Build & Flash](#hướng-dẫn-build--flash)
- [Giao Diện Web Quản Trị](#giao-diện-web-quản-trị)
- [Cấu Hình Qua Web](#cấu-hình-qua-web)
- [MQTT Protocol](#mqtt-protocol)
- [GPIO & Nút Bấm](#gpio--nút-bấm)
- [OTA Update](#ota-update)
- [Cloud Builder GitHub Actions](#cloud-builder-github-actions)
- [Troubleshooting](#troubleshooting)

---

## Tổng Quan Dự Án

`PRI_QMS S3` là firmware **ESP-IDF + Arduino** dành cho thiết bị **điều phối xếp hàng** tại quầy dịch vụ (phòng khám, ngân hàng, bưu điện...). Thiết bị:

- **Nhận lệnh qua MQTT** từ server quản lý hàng đợi
- **In vé giấy nhiệt** qua máy in mini UART (ESC/POS)
- **Điều khiển GPIO** để phát hiện nút bấm gọi số
- **Tự lưu cấu hình** vào NVS (Non-Volatile Storage) — không mất khi mất điện
- **Giao diện Web** nhúng để cấu hình mà không cần flash lại
- **OTA Update** tải firmware mới qua HTTPS

---

## Kiến Trúc Hệ Thống

```
+-----------------------------------------------+
|                  ESP32-S3                     |
|                                               |
|  +----------+   +----------+   +-----------+ |
|  |  WiFi    |   |  MQTT    |   |  HTTP     | |
|  |  STA/AP  |-->|  Client  |   |  Server   | |
|  +----------+   +-----+----+   +-----+-----+ |
|                       |              |        |
|  +----------+   +-----v----+   +-----v-----+ |
|  |  GPIO    |   |  QMS     |   |  Web      | |
|  |  Buttons |-->|  Handler |   |  Pages    | |
|  +----------+   +-----+----+   +-----------+ |
|                       |                       |
|  +----------+   +-----v----+                 |
|  |  NVS     |   |  Thermal |                 |
|  |  Storage |   |  Printer |                 |
|  +----------+   +----------+                 |
+-----------------------------------------------+
         | MQTT
         v
+-------------------+
|   MQTT Broker     |
|  qms1.camdvr.org  |
|  Port: 1993       |
+-------------------+
```

**Luồng khởi động:**
1. Đọc WiFi credentials từ NVS → Kết nối WiFi (STA mode)
2. Nếu không có credentials → Bật AP mode (`ESP32_WiFi_Config`) → Captive Portal
3. Sau khi kết nối WiFi → Start Web Server + MQTT Client
4. Khởi động `button_task` (scan GPIO) và `terminal_task`

---

## Yêu Cầu Phần Cứng

| Thành Phần | Thông Số |
|---|---|
| **MCU** | ESP32-S3 (khuyến nghị: ESP32-S3-DevKitC-1) |
| **Flash** | 4MB (tối thiểu) |
| **Máy in nhiệt** | Mini thermal printer, giao tiếp UART TTL 9600 baud |
| **Nút bấm** | Nút nhấn thường, kéo lên nội bộ (GPIO_PULLUP) |
| **Nguồn** | 5V/1A (USB hoặc adapter) |

**Kết nối máy in nhiệt:**
```
ESP32-S3 GPIO17 (TX) ----> Printer RX
ESP32-S3 GPIO18 (RX) <---- Printer TX
GND ----------------------- GND
```

> **Lưu ý ESP32-S3:** Không dùng GPIO 19, 20 (USB D+/D-); tránh GPIO 26-32 (flash SPI).

---

## Cấu Trúc Thư Mục

```
PRI_QMS S3/
├── pri_NOW.cpp              # Source code chính (1345 dòng)
├── mqtt_handler.h           # MQTT client + QMS handler + global vars
├── thermal_printer.h        # Driver máy in nhiệt (ESC/POS protocol)
├── receipt_template.h       # Template HTML in vé
├── page_config.h            # HTML: trang cấu hình WiFi/MQTT/Device
├── page_gpio.h              # HTML: trang cấu hình GPIO mapping
├── page_log.h               # HTML: trang xem logs realtime
├── page_login.h             # HTML: trang đăng nhập
├── page_ota.h               # HTML: trang OTA update
├── wifi_config_html.h       # HTML: captive portal (AP mode)
├── partitions.csv           # Bảng phân vùng flash (OTA-enabled)
├── sdkconfig.defaults       # Cấu hình mặc định ESP-IDF
├── CMakeLists_main.txt      # CMakeLists.txt cho thư mục main/
├── idf_component.yml        # Component manager config
├── cloud_builder.py         # Script build tự động qua GitHub Actions
├── linux_cloud_builder.py   # Script cloud builder cho Linux
├── builder_config.json      # Cấu hình build (COM port, GitHub token)
├── pri_NOW.bin              # Binary slot ota_0 (flash tại 0x10000)
├── pri_NOW_FULL.bin         # Binary đầy đủ (bao gồm bootloader)
├── ota_data_initial.bin     # OTA data khởi tạo
└── bootloader/              # Bootloader binaries
```

---

## Cấu Hình ESP32-S3

### `sdkconfig.defaults` — Cấu Hình Mặc Định

```ini
# CPU 240MHz (tối đa ESP32-S3)
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y

# System event queue
CONFIG_ESP_SYSTEM_EVENT_QUEUE_SIZE=32
CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=4096
CONFIG_MAIN_TASK_STACK_SIZE=4096

# HTTP Server cho request lớn (HTML pages)
CONFIG_HTTPD_MAX_REQ_HDR_LEN=2048
CONFIG_HTTPD_MAX_URI_LEN=2048

# FreeRTOS tick 1000Hz (1ms resolution)
CONFIG_FREERTOS_HZ=1000

# Custom partition table
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_FILENAME="partitions.csv"
CONFIG_PARTITION_TABLE_OFFSET=0x8000
CONFIG_PARTITION_TABLE_MD5=y

# Flash 4MB
CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y
CONFIG_ESPTOOLPY_FLASHSIZE="4MB"

# TLS certificate bundle cho HTTPS/OTA
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=y
```

> **Quan trọng cho ESP32-S3:** Chạy `idf.py set-target esp32s3` TRƯỚC khi build lần đầu.

### `partitions.csv` — Phân Vùng Flash

```
# Name,   Type, SubType, Offset,    Size,    Mô tả
nvs,      data, nvs,     0x9000,    0x5000   # 20KB  - Config WiFi/MQTT/GPIO
otadata,  data, ota,     0xe000,    0x2000   # 8KB   - OTA slot tracker
ota_0,    app,  ota_0,   0x10000,   0x180000 # 1.5MB - Firmware slot A
ota_1,    app,  ota_1,   0x190000,  0x180000 # 1.5MB - Firmware slot B
spiffs,   data, spiffs,  0x310000,  0xF0000  # 960KB - Reserved (SPIFFS)
```

**OTA hoạt động:** Hai slot ota_0/ota_1, thiết bị tự chuyển slot sau update thành công.

### CMakeLists.txt (được tạo khi build)

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pri_NOW)
```

### Các Component Cần Thiết

```
# project/main/CMakeLists.txt
idf_component_register(
  SRCS "main.cpp"
  INCLUDE_DIRS "."
  PRIV_REQUIRES
    nvs_flash esp_wifi esp_event esp_netif
    esp_http_server mqtt json arduino
    esp_https_ota esp_http_client
)
```

---

## Hướng Dẫn Build & Flash

### Cách 1: Cloud Build qua GitHub Actions (Khuyến Nghị)

Không cần cài ESP-IDF cục bộ. Script tự động build trên cloud.

```bash
# Linux/Mac
python3 cloud_builder.py

# Windows
python cloud_builder.py
```

**Lần đầu chạy:** Script hỏi thông tin và tạo `builder_config.json`:
- GitHub repo URL
- GitHub username
- GitHub Personal Access Token (cần quyền `repo` + `workflow`)
- COM port (ví dụ: `/dev/ttyUSB0` hoặc `COM5`)
- Baud rate (mặc định: `921600`)

**Cấu hình `builder_config.json`:**
```json
{
    "project_name": "pri_NOW",
    "com_port": "/dev/ttyUSB0",
    "baud_rate": "921600",
    "github_repo": "https://github.com/USERNAME/REPO",
    "github_username": "USERNAME",
    "github_token": "ghp_xxxxxxxxxxxx"
}
```

> **Bảo mật:** File này được thêm vào `.gitignore`. KHÔNG commit lên GitHub.

### Cách 2: Build Cục Bộ với ESP-IDF

Yêu cầu: ESP-IDF v5.1+ đã cài đặt và kích hoạt.

```bash
# Bước 1: Set target ESP32-S3
idf.py set-target esp32s3

# Bước 2: Tạo cấu trúc project
mkdir -p project/main project/components
cp pri_NOW.cpp project/main/main.cpp
cp *.h project/main/
cp CMakeLists_main.txt project/main/CMakeLists.txt
cp sdkconfig.defaults project/
cp partitions.csv project/

# Tạo CMakeLists.txt gốc
cat > project/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pri_NOW)
EOF

# Bước 3: Clone Arduino ESP32 component
git clone --depth 1 -b release/v3.0.x \
  https://github.com/espressif/arduino-esp32.git \
  project/components/arduino

# Bước 4: Build
cd project
idf.py build

# Bước 5: Flash + Monitor
idf.py -p /dev/ttyUSB0 -b 921600 flash monitor
```

### Cách 3: Flash Binary Có Sẵn (Nhanh Nhất)

```bash
# Dùng esptool.py (đã có sẵn khi cài ESP-IDF)
esptool.py --chip esp32s3 \
  --port /dev/ttyUSB0 \
  --baud 921600 \
  write_flash \
  0x0      bootloader/bootloader.bin \
  0x8000   partition_table/partition-table.bin \
  0xe000   ota_data_initial.bin \
  0x10000  pri_NOW.bin
```

**Trên Windows với Flash Download Tool:**

| File | Địa Chỉ Flash |
|---|---|
| `bootloader.bin` | `0x000000` |
| `partition-table.bin` | `0x008000` |
| `ota_data_initial.bin` | `0x00e000` |
| `pri_NOW.bin` | `0x010000` |

> **Lưu ý:** Chọn chip `ESP32-S3` trong Flash Download Tool.

---

## Giao Diện Web Quản Trị

### Đăng Nhập
- **URL:** `http://<IP_THIET_BI>/login`
- **Mật khẩu mặc định:** `thien1991`
- Session lưu qua cookie trình duyệt

### Các Trang Chức Năng

| URL | Mô Tả |
|---|---|
| `/` | Trang cấu hình chính (WiFi, MQTT, WebSocket, Device ID) |
| `/log` | Logs hệ thống realtime (auto-refresh) |
| `/gpio` | Ánh xạ GPIO Pin ↔ Service ID |
| `/ota` | Khởi động OTA update từ HTTPS URL |
| `/login` | Trang đăng nhập |

### API Endpoints (dùng cho tích hợp)

| Method | URL | Mô Tả |
|---|---|---|
| `POST` | `/config` | Lưu cấu hình (WiFi/MQTT/WS/Device) |
| `GET` | `/log-data` | Lấy logs dưới dạng text/plain |
| `POST` | `/publish` | Publish MQTT message |
| `GET` | `/api/services` | Lấy danh sách dịch vụ (JSON) |
| `GET` | `/api/gpio-config` | Lấy cấu hình GPIO (JSON) |
| `POST` | `/api/gpio-config` | Lưu cấu hình GPIO (JSON) |
| `POST` | `/api/ota-start` | Bắt đầu OTA từ URL |

---

## Cấu Hình Qua Web

Tất cả cấu hình lưu vào **NVS Flash** — giữ nguyên khi tắt nguồn.

### Section: NETWORK (WiFi)
```
ssid     = Tên mạng WiFi (2.4GHz)
password = Mật khẩu WiFi
```

### Section: MQTT
```
mqtt_server = qms1.camdvr.org   (MQTT broker host)
mqtt_port   = 1993               (MQTT broker port)
mqtt_user   = thom               (MQTT username)
mqtt_pass   = 301258             (MQTT password)
mqtt_topic  = qms/display        (Topic subscribe)
```

### Section: DEV (Device Identity)
```
dev_id  = ID định danh thiết bị (gửi kèm khi publish)
dev_key = Khóa bảo mật thiết bị
```

### Section: WS (WebSocket)
```
ws_url = wss://example.com/ws   (WebSocket server URL)
```

---

## MQTT Protocol

### Topic Subscribe
Thiết bị nhận lệnh từ topic trong cấu hình (mặc định: `qms/display`).

### Commands Nhận Được

**Hiển thị số vé:**
```json
{
  "cmd": "display_ticket",
  "data": {
    "ticket": "A001",
    "color": "#FF5733",
    "service": "Dich vu A"
  }
}
```

**Xóa màn hình:**
```json
{ "cmd": "clear_display" }
```

**Kích hoạt OTA:**
```json
{
  "cmd": "ota_update",
  "url": "https://example.com/firmware.bin"
}
```

### Publish Từ Thiết Bị
Khi người dùng nhấn nút vật lý:
```json
{
  "dev_id": "DEVICE_001",
  "dev_key": "SECRET_KEY",
  "service_id": 1,
  "action": "request_ticket"
}
```

---

## GPIO & Nút Bấm

### Nguyên Lý Hoạt Động

- Mỗi nút bấm kết nối: một chân vào GPIO, chân còn lại vào GND
- GPIO cấu hình `INPUT + PULLUP` — mặc định HIGH, nhấn nút = LOW
- Debounce 300ms (phần mềm)
- `button_task` quét 50ms/lần

### Cấu Hình Qua Trang `/gpio`

Lưu JSON vào NVS với format:
```json
{
  "board": "ESP32-S3",
  "mappings": [
    {"service_id": 1, "pin": 4},
    {"service_id": 2, "pin": 5},
    {"service_id": 3, "pin": 6}
  ]
}
```

### GPIO Hợp Lệ Cho ESP32-S3

| GPIO | Ghi Chú |
|---|---|
| 0-18 | An toàn (0 có strapping function) |
| 19, 20 | USB D-/D+ — TRÁNH nếu dùng USB CDC |
| 21 | An toàn |
| 22-25 | Không có trên S3 |
| 26-32 | Flash SPI — KHÔNG dùng |
| 33-48 | An toàn (33-37 chỉ input nếu có PSRAM) |

---

## OTA Update

### Qua Web (`/ota`)
1. Truy cập `http://<IP>/ota`
2. Nhập HTTPS URL của file `.bin`
3. Nhấn "Start OTA Update"
4. Thiết bị tải, kiểm tra và flash firmware mới
5. Tự khởi động lại sau thành công

### Yêu Cầu File OTA
- Chỉ chứa app binary (không có bootloader)
- Server phải dùng HTTPS (TLS) với certificate hợp lệ
- Kích thước tối đa: **1.5MB** (giới hạn partition)
- Chip: ESP32-S3

### Kiểm Tra Kết Quả OTA
- Log tại `/log`: `"OTA Update successful! Restarting..."`
- Nếu thất bại: `"OTA Update failed! Error: ..."`

---

## Cloud Builder (GitHub Actions)

### Workflow File: `.github/workflows/build.yml`

```yaml
name: Build ESP32 Firmware
on:
  push:
    branches: ["main", "master"]
jobs:
  build:
    runs-on: ubuntu-latest
    container: espressif/idf:release-v5.1
    steps:
      - Checkout Code
      - Reconstruct ESP-IDF Project Structure
      - Clone Arduino ESP32 component (release/v3.0.x)
      - idf.py build
      - Upload Firmware Artifacts (*.bin)
```

### Chạy Script

```bash
# Linux
python3 cloud_builder.py

# Script tự động:
# 1. Tạo/cập nhật workflow file
# 2. Push code lên GitHub
# 3. Poll GitHub API cho đến khi build xong
# 4. Download artifacts ZIP
# 5. Flash qua esptool
```

---

## Troubleshooting

### Không kết nối WiFi
- Chỉ hỗ trợ **2.4GHz** — không hỗ trợ 5GHz
- Kiểm tra SSID/Password tại trang `/`
- Xem logs: `/log`

### Không tìm thấy IP thiết bị
- Kết nối WiFi `ESP32_WiFi_Config` (AP mode — khi chưa cấu hình)
- Truy cập `http://192.168.4.1`
- Cấu hình WiFi → lưu → thiết bị restart vào STA mode

### MQTT không kết nối
- Kiểm tra broker host/port/user/pass
- Kiểm tra network firewall (port 1993)
- Thử telnet: `telnet qms1.camdvr.org 1993`

### Máy in nhiệt không in
- Kiểm tra kết nối: TX=GPIO17, RX=GPIO18
- Baud rate máy in phải = 9600
- Nguồn máy in: cần 5V riêng (không dùng 3.3V từ ESP)

### Build lỗi: "esp32s3 not set"
```bash
idf.py set-target esp32s3
idf.py fullclean
idf.py build
```

### Flash lỗi trên Linux
```bash
# Cấp quyền truy cập serial port
sudo usermod -a -G dialout $USER
# Logout và login lại
```

### GitHub Actions build thất bại
- Kiểm tra token tại GitHub → Settings → Developer settings → PAT
- Token cần quyền: `repo` (full) + `workflow`
- Xem logs chi tiết tại GitHub → Actions tab

---

## Thông Tin Thêm

| Thuộc Tính | Giá Trị |
|---|---|
| **Firmware target** | ESP32-S3 |
| **ESP-IDF version** | v5.1 (release) |
| **Arduino-ESP32** | release/v3.0.x |
| **Flash size** | 4MB |
| **CPU frequency** | 240MHz |
| **UART Printer** | UART2, TX=GPIO17, RX=GPIO18, 9600 baud |
| **Web password** | `thien1991` (thay đổi trước khi deploy) |
| **MQTT default broker** | `qms1.camdvr.org:1993` |

---

*Cập nhật: 2026-07-22 | Project: PRI_QMS S3*
