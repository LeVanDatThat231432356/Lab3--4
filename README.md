
# 🌊 IoT Aquaculture Monitoring System

### Mạch tự động giám sát thông số môi trường cho bể nuôi thủy, hải sản

---

## 👨‍🎓 Thông tin sinh viên

| Thông tin | Nội dung |
|---|---|
| **Họ và tên** | Lê Văn Đạt Thật |
| **Mã sinh viên** | 231432356 |
| **Trường** | Trường Đại học Giao thông Vận tải |
| **Ngành** | Kỹ thuật Thông tin và Truyền thông |
| **Học phần** | HệthốngNhúng/ FreeRTOS |
| **Bài thực hành** | Lab 3 |

---

## 🚀 Demo hệ thống

| Nền tảng | Liên kết |
|---|---|
| 🔧 **Wokwi Simulation** | [▶ Chạy mô phỏng Wokwi](https://wokwi.com/projects/480412432565047297) |
| 🌐 **Web Application** | [▶ Truy cập Website](file:///C:/Users/ADMIN/Downloads/index.html) |

---

## 📌 Giới thiệu đề tài

Hệ thống sử dụng ESP32 kết hợp FreeRTOS, cảm biến HC-SR04 và DS18B20 để giám sát mực nước, nhiệt độ nước trong bể nuôi thủy, hải sản.

Dữ liệu được truyền qua WiFi và MQTT Broker, hiển thị trực quan trên Website, hỗ trợ giám sát và điều khiển thiết bị từ xa.

## ⚙️ Công nghệ sử dụng

- ESP32
- FreeRTOS
- HC-SR04 Ultrasonic Sensor
- DS18B20 Waterproof Temperature Sensor
- MQTT / EMQX
- HTML / CSS / JavaScript
- MQTT.js
- Chart.js
- Wokwi

## ✨ Chức năng chính

- 📏 Đo và giám sát mực nước.
- 🌡️ Đo nhiệt độ nước.
- 📊 Hiển thị biểu đồ realtime.
- 📡 Truyền dữ liệu qua MQTT.
- 💧 Điều khiển máy bơm và van xả.
- 🔄 Hỗ trợ Auto / Manual.
- 🖥️ Giám sát và điều khiển trên Website.

## 🏗️ Kiến trúc hệ thống

```text
HC-SR04 ─────┐
              ├── ESP32 + FreeRTOS
DS18B20 ─────┘         │
                       │ WiFi / MQTT
                       ▼
                  MQTT Broker
                       │
                       ▼
                  Web App
          ┌────────────┴────────────┐
          │                        │
      Monitoring               Control
    Water / Temp             Pump / Valve
```



## 📷 System Preview

<!-- Thêm ảnh chụp Wokwi hoặc Website tại đây -->

![System Preview](<img width="420" height="728" alt="image" src="https://github.com/user-attachments/assets/776352e4-afe2-49f7-94a4-3dba1009c183" />
)

---


**IoT Aquaculture Monitoring System**  
*ESP32 · FreeRTOS · MQTT · Web Application*
