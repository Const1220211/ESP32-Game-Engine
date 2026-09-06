# 📚 Требуемые Библиотеки и Установка

## 🔧 Установка Arduino IDE и ESP32

### 1. Скачайте Arduino IDE
- Официально: https://www.arduino.cc/en/software
- Рекомендуется Arduino IDE 2.0+

### 2. Добавьте поддержку ESP32 в Arduino IDE

#### Способ 1: Через Boards Manager (Рекомендуется)
1. Откройте **Arduino IDE**
2. Перейдите в **File → Preferences**
3. В поле "Additional Boards Manager URLs" добавьте:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. Нажмите **OK**
5. Перейдите в **Tools → Board → Boards Manager**
6. Найдите **esp32** и установите последнюю версию

#### Способ 2: Через git (Продвинутый)
```bash
cd ~/Documents/Arduino/hardware
mkdir esp32
cd esp32
git clone https://github.com/espressif/arduino-esp32.git esp32
cd esp32
git submodule update --init --recursive
cd tools
python3 get.py
```

---

## 📦 Требуемые Библиотеки

Установите через **Sketch → Include Library → Manage Libraries** (Ctrl+Shift+I)

### 1. **TFT_eSPI** (Дисплей ST7735S)
- **Поиск в Boards Manager**: `TFT_eSPI`
- **Автор**: Bodmer
- **Версия**: 2.4.8+
- **Что делает**: Управление TFT дисплеем

**После установки TFT_eSPI:**
1. Откройте файл конфига:
   ```
   Arduino/libraries/TFT_eSPI/User_Setup.h
   ```
2. Найдите и отредактируйте строки:
   ```cpp
   #define ST7735_DRIVER
   #define TFT_WIDTH  160
   #define TFT_HEIGHT 128
   #define TFT_CS     5
   #define TFT_DC     16
   #define TFT_RST    17
   #define TFT_MOSI   23
   #define TFT_MISO   19
   #define TFT_SCLK   18
   #define SPI_FREQUENCY 40000000
   ```

### 2. **SD** (встроенная библиотека)
- Идет с Arduino IDE по умолчанию
- Используется для работы с SD картой

### 3. **WiFi** (встроенная библиотека)
- Идет с ESP32 по умолчанию
- Управление WiFi соединением

### 4. **WebServer** (встроенная библиотека)
- Идет с ESP32 по умолчанию
- Веб-сервер для загрузки игр

### 5. **FS** (встроенная библиотека)
- Идет с ESP32 по умолчанию
- Файловая система

### 6. **SPIFFS** (встроенная библиотека)
- Идет с ESP32 по умолчанию
- Встроенная флеш память

---

## 🚀 Список всех библиотек для Copy-Paste

В **Boards Manager** установите следующие пакеты:

```
1. TFT_eSPI - Bodmer
2. esp32 (Board Support Package)
```

**Встроенные библиотеки** (не требуют установки):
- SPI.h
- SD.h
- WiFi.h
- WebServer.h
- FS.h
- SPIFFS.h

---

## ⚙️ Конфигурация Arduino IDE

### Выбор правильной платы

1. **Tools → Board** выберите **ESP32 → ESP32 Dev Module**

### Правильные настройки компиляции

```
Board: ESP32 Dev Module
Upload Speed: 115200 baud
CPU Frequency: 240 MHz
Flash Frequency: 80 MHz
Flash Mode: QIO
Flash Size: 4MB
Partition Scheme: Default 4MB with spiffs
Core Debug Level: Info
PSRAM: Disabled
```

### Порт и Скорость
- **Port**: COM3 (или ваш) - выберите в Tools → Port
- **Upload Speed**: 115200

---

## 🔗 Файл User_Setup.h для TFT_eSPI (Полный конфиг)

**Путь к файлу:**
```
C:\Users\[ВАШ_ПОЛЬЗОВАТЕЛЬ]\Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
```

**Скопируйте и замените содержимое:**

```cpp
// User_Setup.h for ST7735S 1.8" Display

#define ST7735_DRIVER

// Display resolution
#define TFT_WIDTH 160
#define TFT_HEIGHT 128

// SPI Pins
#define TFT_CS     5
#define TFT_DC     16  
#define TFT_RST    17
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_SCLK   18

// SPI speed (MHz)
#define SPI_FREQUENCY 40000000

// Color mode
#define TFT_RGB_ORDER TFT_RGB

// Character display settings
#define TFT_INVERT_TEXT 0
```

---

## 📥 Быстрая Установка (Все за раз)

### Windows/Mac/Linux:

1. **Скачайте Arduino IDE 2.0+**
2. **Откройте IDE и добавьте ESP32**:
   - File → Preferences
   - Additional Boards Manager URLs:
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
3. **Установите библиотеки**:
   - Tools → Manage Libraries (Ctrl+Shift+I)
   - Найдите и установите: **TFT_eSPI** (Bodmer)

4. **Скопируйте код** из репозитория в новый sketch

5. **Выберите плату**:
   - Tools → Board → ESP32 → ESP32 Dev Module

6. **Выберите COM порт** где подключен ESP32

7. **Нажмите Upload** (кнопка со стрелкой вправо)

---

## 🔧 Решение Проблем

### Ошибка: "Board esp32:esp32:esp32 not found"
- Заново установите ESP32 Board Support Package через Boards Manager
- Перезагрузите IDE

### Ошибка: "TFT_eSPI not found"
- Убедитесь что установили TFT_eSPI из Manage Libraries
- Проверьте что используется правильный User_Setup.h

### Дисплей не работает / Черный экран
- Проверьте все SPI соединения (GPIO 18, 23, 19)
- Проверьте GPIO для CS (5), DC (16), RST (17)
- Убедитесь что дисплей получает 3.3V!
- Попробуйте пример из TFT_eSPI: File → Examples → TFT_eSPI → 320x240

### Upload медленный / зависает
- Измените Upload Speed на 921600 в Tools
- Используйте USB 2.0 кабель (не USB 3.0)

### SD Card не работает
- Отформатируйте SD карту на FAT32
- Проверьте все SPI соединения (GPIO 18, 23, 19, 27)
- Создайте папку `/games` на SD карте

---

## ✅ Проверка Установки

После установки всего, скомпилируйте код:

1. Загрузите **main.ino** из репозитория
2. Нажмите **Verify** (галочка слева вверху)
3. Если компилируется без ошибок - все установлено правильно!

Если есть ошибки - проверьте консоль для подробной информации.
