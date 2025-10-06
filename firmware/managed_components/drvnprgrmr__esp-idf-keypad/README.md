# ESP-IDF Keypad Component
[![Component Registry](https://components.espressif.com/components/drvnprgrmr/esp-idf-keypad/badge.svg)](https://components.espressif.com/components/drvnprgrmr/esp-idf-keypad)



A lightweight, configurable **matrix keypad driver** for ESP-IDF.  
Supports key scanning, debouncing, press/hold detection, and buffering with FreeRTOS queues.  
Works with any keypad size (rows × columns) via a C++ template API.

---

## ✨ Features

- Supports **any matrix keypad size** (`rows × cols`)
- Non-blocking and blocking key reads
- **Press** and **hold** event detection
- Configurable **debounce time** and **hold time**
- Thread-safe buffering using FreeRTOS queues
- Configurable buffer size via `menuconfig`
- Works with ESP-IDF logging system (`esp_log`)

---

## 📦 Installation

Add the component to your ESP-IDF project:

```bash
idf.py add-dependency "drvnprgrmr/keypad^1.0.0"
```
or add manually to components/ folder.

## ⚙️ Configuration (menuconfig)
```text
Component config  --->
    Keypad Configuration  --->
        [*] Enable logging
        (3) Default log level
        (10) Max number of keys in buffer
```