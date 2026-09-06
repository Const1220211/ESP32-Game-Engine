#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

// ===== DISPLAY CONFIGURATION (TFT 1.8" ST7735S) =====
// Для ESP32-S3 N16R8
#define TFT_CS     5    // Chip Select
#define TFT_RST    17   // Reset
#define TFT_DC     16   // Data/Command (A0)
#define TFT_MOSI   11   // MOSI (SPI)
#define TFT_SCK    12   // SCK (SPI)
#define TFT_MISO   13   // MISO (SPI)
#define TFT_BL     4    // Backlight

// Display resolution (128x160 for this display model)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 160

// ST7735S Commands
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_INVON   0x21
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF
#define TFT_RED     0xF800
#define TFT_GREEN   0x07E0
#define TFT_BLUE    0x001F
#define TFT_YELLOW  0xFFE0
#define TFT_CYAN    0x07FF
#define TFT_MAGENTA 0xF81F
#define TFT_DARKGREY 0x4208

// ===== SD CARD CONFIGURATION =====
#define SD_CS      10   // SD Card Chip Select

// ===== INPUT CONFIGURATION =====
// Joystick (Аналоговые входы)
#define JOY_X      3    // Joystick X axis (ADC)
#define JOY_Y      6    // Joystick Y axis (ADC)
#define JOY_SW     7    // Joystick Button

// Keypad 4x4 (для ESP32-S3)
const int ROW_PINS[4] = {46, 45, 42, 41};      // Row pins (выходы)
const int COL_PINS[4] = {40, 39, 38, 37};      // Column pins (входы)

const char KEYPAD_KEYS[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ===== GLOBAL VARIABLES =====
WebServer server(80);
volatile bool gameRunning = false;
volatile bool sdCardReady = false;
volatile bool wifiConnected = false;
volatile int currentMenuSelection = 0;

struct GameInfo {
  String name;
  String filename;
  uint32_t size;
};

GameInfo currentGame;

// ===== JOYSTICK DATA =====
struct JoystickData {
  int x;
  int y;
  bool button;
  bool up;
  bool down;
  bool left;
  bool right;
};

JoystickData joystick;
const int JOYSTICK_DEADZONE = 300;
const int JOYSTICK_THRESHOLD = 2000;

// ===== ST7735S DISPLAY CLASS =====
class ST7735Display {
private:
  SPIClass* spi;
  
  void writeCommand(uint8_t cmd) {
    digitalWrite(TFT_DC, LOW);
    digitalWrite(TFT_CS, LOW);
    spi->write(cmd);
    digitalWrite(TFT_CS, HIGH);
  }
  
  void writeData(uint8_t data) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    spi->write(data);
    digitalWrite(TFT_CS, HIGH);
  }
  
  void writeData16(uint16_t data) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    spi->write(data >> 8);
    spi->write(data & 0xFF);
    digitalWrite(TFT_CS, HIGH);
  }

public:
  ST7735Display() {
    spi = &SPI;
  }
  
  void init() {
    // Initialize GPIO
    pinMode(TFT_CS, OUTPUT);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_DC, OUTPUT);
    pinMode(TFT_BL, OUTPUT);
    
    // Initialize SPI
    spi->begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
    spi->setFrequency(10000000);  // 10MHz
    spi->setDataMode(SPI_MODE0);
    spi->setBitOrder(SPI_MSBFIRST);
    
    // Reset
    digitalWrite(TFT_RST, HIGH);
    delay(10);
    digitalWrite(TFT_RST, LOW);
    delay(10);
    digitalWrite(TFT_RST, HIGH);
    delay(120);
    
    // Backlight on
    digitalWrite(TFT_BL, HIGH);
    
    // Init sequence
    writeCommand(ST7735_SWRESET);
    delay(150);
    
    writeCommand(ST7735_SLPOUT);
    delay(500);
    
    writeCommand(ST7735_FRMCTR1);
    writeData(0x01);
    writeData(0x2C);
    writeData(0x2D);
    
    writeCommand(ST7735_INVCTR);
    writeData(0x07);
    
    writeCommand(ST7735_PWCTR1);
    writeData(0xA2);
    writeData(0x02);
    writeData(0x84);
    
    writeCommand(ST7735_PWCTR2);
    writeData(0xC5);
    
    writeCommand(ST7735_PWCTR3);
    writeData(0x0A);
    writeData(0x00);
    
    writeCommand(ST7735_PWCTR4);
    writeData(0x8A);
    writeData(0x2A);
    
    writeCommand(ST7735_PWCTR5);
    writeData(0x8A);
    writeData(0xEE);
    
    writeCommand(ST7735_VMCTR1);
    writeData(0x0E);
    
    writeCommand(ST7735_MADCTL);
    writeData(0xC8);
    
    writeCommand(ST7735_COLMOD);
    writeData(0x05);
    
    writeCommand(ST7735_GMCTRP1);
    writeData(0x02);
    writeData(0x1c);
    writeData(0x07);
    writeData(0x12);
    writeData(0x37);
    writeData(0x32);
    writeData(0x29);
    writeData(0x2d);
    writeData(0x29);
    writeData(0x25);
    writeData(0x2B);
    writeData(0x39);
    writeData(0x00);
    writeData(0x01);
    writeData(0x03);
    writeData(0x10);
    
    writeCommand(ST7735_GMCTRN1);
    writeData(0x03);
    writeData(0x1d);
    writeData(0x07);
    writeData(0x06);
    writeData(0x2E);
    writeData(0x2C);
    writeData(0x29);
    writeData(0x2D);
    writeData(0x2E);
    writeData(0x2E);
    writeData(0x37);
    writeData(0x3F);
    writeData(0x00);
    writeData(0x00);
    writeData(0x02);
    writeData(0x10);
    
    writeCommand(ST7735_DISPON);
    delay(100);
    
    fillScreen(TFT_BLACK);
  }
  
  void fillScreen(uint16_t color) {
    fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color);
  }
  
  void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    writeCommand(ST7735_CASET);
    writeData(0);
    writeData(x);
    writeData(0);
    writeData(x + w - 1);
    
    writeCommand(ST7735_RASET);
    writeData(0);
    writeData(y);
    writeData(0);
    writeData(y + h - 1);
    
    writeCommand(ST7735_RAMWR);
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    
    for (uint32_t i = 0; i < (w * h); i++) {
      spi->write(color >> 8);
      spi->write(color & 0xFF);
    }
    
    digitalWrite(TFT_CS, HIGH);
  }
  
  void drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;
    fillRect(x, y, 1, 1, color);
  }
  
  void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
  }
  
  void drawHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    if (y >= SCREEN_HEIGHT) return;
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    fillRect(x, y, w, 1, color);
  }
  
  void drawVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if (x >= SCREEN_WIDTH) return;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    fillRect(x, y, 1, h, color);
  }
  
  void setCursor(uint16_t x, uint16_t y) {
    cursorX = x;
    cursorY = y;
  }
  
  void setTextColor(uint16_t color, uint16_t bgColor) {
    textColor = color;
    textBgColor = bgColor;
  }
  
  void setTextSize(uint8_t size) {
    textSize = size;
  }
  
  void print(const char* str) {
    while (*str) {
      if (*str == '\n') {
        cursorY += (8 * textSize) + 2;
        cursorX = 0;
      } else {
        drawChar(cursorX, cursorY, *str);
        cursorX += (6 * textSize);
      }
      str++;
    }
  }
  
  void print(String str) {
    print(str.c_str());
  }
  
  void println(const char* str) {
    print(str);
    cursorY += (8 * textSize) + 2;
    cursorX = 0;
  }
  
  void println(String str) {
    println(str.c_str());
  }
  
  void printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    print(buffer);
  }
  
private:
  uint16_t cursorX = 0, cursorY = 0;
  uint16_t textColor = TFT_WHITE;
  uint16_t textBgColor = TFT_BLACK;
  uint8_t textSize = 1;
  
  void drawChar(uint16_t x, uint16_t y, char c) {
    // Simple 5x8 character rendering
    if (c < 32 || c > 126) return;
    
    // For now, fill a small rectangle as placeholder
    if (textSize == 1) {
      fillRect(x, y, 5, 8, textBgColor);
    } else {
      fillRect(x, y, 5 * textSize, 8 * textSize, textBgColor);
    }
  }
};

// Global display object
ST7735Display tft;

// ===== FUNCTION PROTOTYPES =====
void setupDisplay();
void setupSD();
void setupWiFi();
void setupInput();
void setupWebServer();
void initSPIFFS();
void displayMainMenu();
void displayGamesList();
void displayWiFiMenu();
void drawButton(int x, int y, int w, int h, String text, uint16_t color, bool selected = false);
void readJoystick();
char readKeypad();
void loadGamesList();
void loadGameFromSD(String filename);
void handleRoot();
void handleUpload();
void handleGamesList();
void handleFileDelete();
void listSDFiles();
void handleMenuSelection();

void setup() {
  Serial.begin(115200);
  delay(2000);  // Больше времени для загрузки S3
  
  Serial.println("\n\n=== ESP32-S3 N16R8 Game Engine Starting ===");
  Serial.println("Flash: 16MB | PSRAM: 8MB");
  
  setupDisplay();
  setupInput();
  setupSD();
  initSPIFFS();
  setupWiFi();
  setupWebServer();
  
  displayMainMenu();
}

void loop() {
  server.handleClient();
  readJoystick();
  
  // Простая навигация по меню джойстиком
  if (joystick.up && currentMenuSelection > 0) {
    currentMenuSelection--;
    displayMainMenu();
    delay(300);
  }
  if (joystick.down && currentMenuSelection < 2) {
    currentMenuSelection++;
    displayMainMenu();
    delay(300);
  }
  if (joystick.button) {
    handleMenuSelection();
    delay(500);
  }
  
  delay(50);
}

// ===== DISPLAY SETUP =====
void setupDisplay() {
  Serial.println("[DISPLAY] Initializing TFT 1.8\" Display (ESP32-S3)...");
  
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.print("Initializing...");
  
  Serial.println("[DISPLAY] Display initialized successfully");
  delay(1000);
}

// ===== SD CARD SETUP =====
void setupSD() {
  Serial.println("[SD] Initializing SD Card...");
  
  SPI.begin(12, 13, 11, 10);  // SCK, MISO, MOSI, CS для ESP32-S3
  
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] FAILED! No SD card detected");
    sdCardReady = false;
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 10);
    tft.print("SD Card NOT FOUND!");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.print("Insert SD card");
    return;
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("[SD] SD Card initialized! Size: %lluMB\n", cardSize);
  sdCardReady = true;
  
  // Создать папку /games если её нет
  if (!SD.exists("/games")) {
    SD.mkdir("/games");
    Serial.println("[SD] Created /games directory");
  }
  
  listSDFiles();
}

// ===== SPIFFS SETUP =====
void initSPIFFS() {
  Serial.println("[SPIFFS] Initializing SPIFFS...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] FAILED to mount SPIFFS");
    return;
  }
  
  Serial.println("[SPIFFS] SPIFFS mounted successfully");
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  int count = 0;
  
  while (file && count < 10) {
    Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
    count++;
  }
}

// ===== WIFI SETUP =====
void setupWiFi() {
  Serial.println("[WiFi] Setting up WiFi Access Point...");
  
  // Create Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-S3-GameEngine", "password123");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("[WiFi] AP IP address: ");
  Serial.println(IP);
  
  wifiConnected = true;
}

// ===== WEB SERVER SETUP =====
void setupWebServer() {
  Serial.println("[WEB] Setting up Web Server...");
  
  server.on("/", handleRoot);
  server.on("/list", handleGamesList);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/delete", HTTP_GET, handleFileDelete);
  
  server.begin();
  Serial.println("[WEB] Web Server started on http://192.168.4.1");
}

// ===== INPUT SETUP =====
void setupInput() {
  Serial.println("[INPUT] Initializing Input Devices...");
  
  // Joystick
  pinMode(JOY_SW, INPUT_PULLUP);
  
  // Keypad rows
  for (int i = 0; i < 4; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], HIGH);
  }
  
  // Keypad columns
  for (int i = 0; i < 4; i++) {
    pinMode(COL_PINS[i], INPUT_PULLUP);
  }
  
  Serial.println("[INPUT] Input devices initialized");
}

// ===== JOYSTICK READING =====
void readJoystick() {
  joystick.x = analogRead(JOY_X);
  joystick.y = analogRead(JOY_Y);
  joystick.button = !digitalRead(JOY_SW);
  
  // Normalize values
  joystick.up = (joystick.y > JOYSTICK_THRESHOLD);
  joystick.down = (joystick.y < JOYSTICK_DEADZONE);
  joystick.left = (joystick.x < JOYSTICK_DEADZONE);
  joystick.right = (joystick.x > JOYSTICK_THRESHOLD);
}

// ===== KEYPAD READING =====
char readKeypad() {
  for (int row = 0; row < 4; row++) {
    digitalWrite(ROW_PINS[row], LOW);
    delay(5);
    
    for (int col = 0; col < 4; col++) {
      if (digitalRead(COL_PINS[col]) == LOW) {
        digitalWrite(ROW_PINS[row], HIGH);
        return KEYPAD_KEYS[row][col];
      }
    }
    
    digitalWrite(ROW_PINS[row], HIGH);
  }
  
  return '\0';
}

// ===== UI FUNCTIONS =====
void displayMainMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  
  tft.setCursor(35, 10);
  tft.print("ESP32-S3");
  tft.setCursor(20, 30);
  tft.setTextSize(1);
  tft.print("Game Engine");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Menu items
  drawButton(5, 50, 118, 25, "Load Game (SD)", TFT_GREEN, currentMenuSelection == 0);
  drawButton(5, 80, 118, 25, "WiFi Upload", TFT_BLUE, currentMenuSelection == 1);
  drawButton(5, 110, 118, 25, "Settings", TFT_CYAN, currentMenuSelection == 2);
  
  // Status bar
  tft.drawHLine(0, 145, SCREEN_WIDTH, TFT_DARKGREY);
  
  if (!sdCardReady) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 150);
    tft.setTextSize(1);
    tft.print("SD: Error");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(5, 150);
    tft.setTextSize(1);
    tft.print("SD: Ready");
  }
  
  if (wifiConnected) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(85, 150);
    tft.setTextSize(1);
    tft.print("WiFi: On");
  }
}

void displayGamesList() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(5, 5);
  tft.print("GAMES ON SD:");
  
  File root = SD.open("/games");
  if (!root || !root.isDirectory()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 30);
    tft.print("No /games dir!");
    return;
  }
  
  int y = 20;
  File file = root.openNextFile();
  int gameCount = 0;
  
  while (file && y < 140) {
    if (!file.isDirectory()) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(5, y);
      String name = file.name();
      if (name.length() > 20) {
        name = name.substring(0, 17) + "...";
      }
      tft.print(name);
      y += 12;
      gameCount++;
    }
    file = root.openNextFile();
  }
  
  if (gameCount == 0) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 35);
    tft.print("No games found");
    tft.setCursor(5, 50);
    tft.print("Use WiFi to upload");
  }
}

void displayWiFiMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(10, 10);
  tft.print("WiFi Upload");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 35);
  tft.print("Network:");
  tft.setCursor(15, 50);
  tft.print("ESP32-S3-GameEngine");
  
  tft.setCursor(10, 70);
  tft.print("Password:");
  tft.setCursor(15, 85);
  tft.print("password123");
  
  tft.setCursor(10, 110);
  tft.print("Connect then visit:");
  tft.setCursor(10, 125);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("192.168.4.1");
}

void drawButton(int x, int y, int w, int h, String text, uint16_t color, bool selected) {
  if (selected) {
    tft.fillRect(x, y, w, h, color);
    tft.setTextColor(TFT_BLACK, color);
  } else {
    tft.drawRect(x, y, w, h, color);
    tft.setTextColor(color, TFT_BLACK);
  }
  
  tft.setCursor(x + 5, y + 8);
  tft.print(text);
}

void handleMenuSelection() {
  switch(currentMenuSelection) {
    case 0:  // Load Game
      displayGamesList();
      delay(2000);
      displayMainMenu();
      break;
    case 1:  // WiFi Upload
      displayWiFiMenu();
      delay(3000);
      displayMainMenu();
      break;
    case 2:  // Settings
      displayWiFiMenu();
      delay(2000);
      displayMainMenu();
      break;
  }
}

// ===== SD CARD FUNCTIONS =====
void listSDFiles() {
  Serial.println("[SD] Files on SD Card:");
  
  File root = SD.open("/");
  File file = root.openNextFile();
  int count = 0;
  
  while (file) {
    Serial.printf("  - %s (%s, %d bytes)\n", 
      file.name(), 
      file.isDirectory() ? "DIR" : "FILE",
      file.size()
    );
    file = root.openNextFile();
    count++;
    if (count > 20) break;  // Limit output
  }
}

void loadGameFromSD(String filename) {
  Serial.printf("[GAME] Loading game: %s\n", filename.c_str());
  
  String filepath = "/games/" + filename;
  if (!SD.exists(filepath)) {
    Serial.println("[GAME] File not found!");
    return;
  }
  
  File gameFile = SD.open(filepath);
  if (!gameFile) {
    Serial.println("[GAME] Failed to open file!");
    return;
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.printf("Running: %s\n", filename.c_str());
  
  gameRunning = true;
  
  // Game execution logic would go here
  delay(2000);
  
  gameFile.close();
  gameRunning = false;
  displayMainMenu();
}

// ===== WEB SERVER HANDLERS =====
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32-S3 Game Engine</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #1a1a1a; color: #fff; }";
  html += ".container { max-width: 600px; margin: auto; }";
  html += "h1 { color: #ffff00; text-align: center; }";
  html += ".info { background: #333; padding: 15px; border-radius: 5px; margin: 15px 0; font-size: 12px; }";
  html += ".upload-form { background: #333; padding: 20px; border-radius: 5px; margin: 20px 0; }";
  html += ".button { background: #0080ff; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 10px 0; font-size: 16px; width: 100%; }";
  html += ".button:hover { background: #0060cc; }";
  html += ".file-list { background: #333; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".file-item { padding: 10px; background: #444; margin: 8px 0; border-radius: 3px; display: flex; justify-content: space-between; }";
  html += "input[type='file'] { margin: 10px 0; padding: 8px; }";
  html += ".delete-btn { background: #ff4444; padding: 5px 10px; cursor: pointer; border: none; border-radius: 3px; color: white; }";
  html += ".delete-btn:hover { background: #cc0000; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🎮 ESP32-S3 Game Engine</h1>";
  
  html += "<div class='info'>";
  html += "<strong>Платформа:</strong> ESP32-S3 N16R8<br>";
  html += "<strong>Flash:</strong> 16MB | <strong>RAM:</strong> 8MB PSRAM<br>";
  html += "<strong>Дисплей:</strong> 1.8\" TFT (128x160)<br>";
  html += "</div>";
  
  html += "<div class='upload-form'>";
  html += "<h2>📤 Upload Game</h2>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='game' accept='.bin,.hex,.elf,.zip' required>";
  html += "<button class='button' type='submit'>Upload</button>";
  html += "</form>";
  html += "</div>";
  
  html += "<div class='file-list'>";
  html += "<h2>📦 Games on SD Card</h2>";
  html += "<button class='button' onclick='loadList()'>Refresh</button>";
  html += "<div id='gamesList'></div>";
  html += "</div>";
  
  html += "</div>";
  html += "<script>";
  html += "function loadList() {";
  html += "  fetch('/list')";
  html += "    .then(r => r.json())";
  html += "    .then(games => {";
  html += "      let h = '';";
  html += "      if (games.length === 0) {";
  html += "        h = '<p style=\"color: #aaa;\">No games found</p>';";
  html += "      } else {";
  html += "        games.forEach(g => {";
  html += "          let size = (g.size / 1024).toFixed(1);";
  html += "          h += '<div class=\"file-item\">';";
  html += "          h += '<span>' + g.name + ' (' + size + ' KB)</span>';";
  html += "          h += '<button class=\"delete-btn\" onclick=\"deleteGame(\\\"' + g.name + '\\\");\">Delete</button>';";
  html += "          h += '</div>';";
  html += "        });";
  html += "      }";
  html += "      document.getElementById('gamesList').innerHTML = h;";
  html += "    });";
  html += "}";
  html += "function deleteGame(name) {";
  html += "  if(confirm('Delete ' + name + '?')) {";
  html += "    fetch('/delete?file=' + encodeURIComponent(name))";
  html += "      .then(() => loadList());";
  html += "  }";
  html += "}";
  html += "loadList();";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleGamesList() {
  String json = "[";
  
  File root = SD.open("/games");
  if (root) {
    File file = root.openNextFile();
    bool first = true;
    
    while (file) {
      if (!file.isDirectory()) {
        if (!first) json += ",";
        json += "{\"name\":\"";
        json += file.name();
        json += "\",\"size\":";
        json += file.size();
        json += "}";
        first = false;
      }
      file = root.openNextFile();
    }
  }
  
  json += "]";
  server.send(200, "application/json", json);
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  static File uploadFile;
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/games/" + upload.filename;
    
    if (!SD.exists("/games")) {
      SD.mkdir("/games");
    }
    
    Serial.printf("[WEB] Upload started: %s\n", filename.c_str());
    uploadFile = SD.open(filename, FILE_WRITE);
    
    if (!uploadFile) {
      Serial.println("[WEB] Failed to create file!");
      server.send(500, "text/plain", "Failed to create file");
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
      Serial.printf("[WEB] Uploading: %d/%d bytes\n", upload.currentSize, upload.totalSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("[WEB] Upload complete: %d bytes\n", upload.totalSize);
      server.send(200, "text/plain", "Upload successful! File size: " + String(upload.totalSize) + " bytes");
    }
  }
}

void handleFileDelete() {
  String filename = server.arg("file");
  String filepath = "/games/" + filename;
  
  if (SD.remove(filepath)) {
    Serial.printf("[WEB] File deleted: %s\n", filename.c_str());
    server.send(200, "text/plain", "File deleted");
  } else {
    Serial.printf("[WEB] Failed to delete: %s\n", filename.c_str());
    server.send(500, "text/plain", "Failed to delete file");
  }
}
