#include <SPI.h>
#include <TFT_eSPI.h>
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
TFT_eSPI tft = TFT_eSPI();
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
  tft.setRotation(1);  // Landscape (160x128)
  tft.fillScreen(TFT_BLACK);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // Backlight ON
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("ESP32-S3 Initializing...");
  
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
    tft.println("SD Card NOT FOUND!");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 30);
    tft.println("Insert SD card");
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
  
  tft.setCursor(20, 10);
  tft.println("ESP32-S3");
  tft.setCursor(5, 30);
  tft.setTextSize(1);
  tft.println("Game Engine");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Menu items
  drawButton(5, 50, 150, 25, "Load Game (SD)", TFT_GREEN, currentMenuSelection == 0);
  drawButton(5, 82, 150, 25, "WiFi Upload", TFT_BLUE, currentMenuSelection == 1);
  drawButton(5, 114, 150, 25, "Settings", TFT_CYAN, currentMenuSelection == 2);
  
  // Status bar
  tft.drawFastHLine(0, 145, 160, TFT_DARKGREY);
  
  if (!sdCardReady) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 150);
    tft.setTextSize(1);
    tft.println("SD: Error");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(5, 150);
    tft.setTextSize(1);
    tft.println("SD: Ready");
  }
  
  if (wifiConnected) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(120, 150);
    tft.setTextSize(1);
    tft.println("WiFi: On");
  }
}

void displayGamesList() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(5, 5);
  tft.println("GAMES ON SD:");
  
  File root = SD.open("/games");
  if (!root || !root.isDirectory()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(5, 30);
    tft.println("No /games dir!");
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
      tft.println(name);
      y += 12;
      gameCount++;
    }
    file = root.openNextFile();
  }
  
  if (gameCount == 0) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(5, 35);
    tft.println("No games found");
    tft.setCursor(5, 50);
    tft.println("Use WiFi to upload");
  }
}

void displayWiFiMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(10, 10);
  tft.println("WiFi Upload");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 35);
  tft.println("Network:");
  tft.setCursor(15, 50);
  tft.println("ESP32-S3-GameEngine");
  
  tft.setCursor(10, 70);
  tft.println("Password:");
  tft.setCursor(15, 85);
  tft.println("password123");
  
  tft.setCursor(10, 110);
  tft.println("Connect then visit:");
  tft.setCursor(10, 125);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("192.168.4.1");
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
  tft.println(text);
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
  html += "<strong>Дисплей:</strong> 1.8\" TFT (160x128)<br>";
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
