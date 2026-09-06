#include <SPI.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>

// ===== DISPLAY CONFIGURATION (TFT 1.8" ST7735S) =====
#define TFT_CS     5    // Chip Select
#define TFT_RST    17   // Reset
#define TFT_DC     16   // Data/Command
#define TFT_MOSI   23   // MOSI
#define TFT_SCK    18   // SCK
#define TFT_MISO   19   // MISO
#define TFT_BL     4    // Backlight

// ===== SD CARD CONFIGURATION =====
#define SD_CS      27   // SD Card Chip Select

// ===== INPUT CONFIGURATION =====
// Joystick
#define JOY_X      32   // Joystick X axis (ADC)
#define JOY_Y      33   // Joystick Y axis (ADC)
#define JOY_SW     25   // Joystick Button

// Keypad 4x4
const int ROW_PINS[4] = {14, 12, 13, 26};      // Row pins
const int COL_PINS[4] = {2, 15, 0, 35};        // Column pins
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
void drawButton(int x, int y, int w, int h, String text, uint16_t color);
void readJoystick();
char readKeypad();
void loadGamesList();
void loadGameFromSD(String filename);
void handleRoot();
void handleUpload();
void handleGamesList();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32 Game Engine Starting ===");
  
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
  delay(50);
}

// ===== DISPLAY SETUP =====
void setupDisplay() {
  Serial.println("[DISPLAY] Initializing TFT 1.8\" Display...");
  
  tft.init();
  tft.setRotation(1);  // Landscape
  tft.fillScreen(TFT_BLACK);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // Backlight ON
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Initializing...");
  
  Serial.println("[DISPLAY] Display initialized successfully");
}

// ===== SD CARD SETUP =====
void setupSD() {
  Serial.println("[SD] Initializing SD Card...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] FAILED! No SD card detected");
    sdCardReady = false;
    return;
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("[SD] SD Card initialized! Size: %lluMB\n", cardSize);
  sdCardReady = true;
  
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
  
  while (file) {
    Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

// ===== WIFI SETUP =====
void setupWiFi() {
  Serial.println("[WiFi] Setting up WiFi Access Point...");
  
  // Create Access Point
  WiFi.softAP("ESP32-GameEngine", "password123");
  
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
  
  tft.setCursor(10, 10);
  tft.println("GAME ENGINE");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  drawButton(10, 40, 100, 30, "Load Game (SD)", TFT_GREEN);
  drawButton(10, 80, 100, 30, "WiFi Upload", TFT_BLUE);
  drawButton(10, 120, 100, 30, "Settings", TFT_CYAN);
  
  if (!sdCardReady) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 155);
    tft.println("SD Card: Not Ready");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(10, 155);
    tft.println("SD Card: Ready");
  }
}

void displayGamesList() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(10, 10);
  tft.println("GAMES ON SD CARD:");
  
  File root = SD.open("/games");
  if (!root || !root.isDirectory()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(10, 50);
    tft.println("No /games directory!");
    return;
  }
  
  int y = 30;
  File file = root.openNextFile();
  
  while (file && y < 160) {
    if (!file.isDirectory()) {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10, y);
      tft.println(file.name());
      y += 15;
    }
    file = root.openNextFile();
  }
}

void displayWiFiMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  
  tft.setCursor(10, 10);
  tft.println("WiFi Upload");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 40);
  tft.println("AP: ESP32-GameEngine");
  tft.setCursor(10, 60);
  tft.println("Pass: password123");
  
  tft.setCursor(10, 100);
  tft.println("Visit:");
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("http://192.168.4.1");
}

void drawButton(int x, int y, int w, int h, String text, uint16_t color) {
  tft.drawRect(x, y, w, h, color);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(x + 5, y + 8);
  tft.println(text);
}

// ===== SD CARD FUNCTIONS =====
void listSDFiles() {
  Serial.println("[SD] Files on SD Card:");
  
  File root = SD.open("/");
  File file = root.openNextFile();
  
  while (file) {
    Serial.printf("  - %s (%s)\n", 
      file.name(), 
      file.isDirectory() ? "DIR" : "FILE"
    );
    file = root.openNextFile();
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
  // Read game data and execute game loop
  
  gameFile.close();
  gameRunning = false;
}

// ===== WEB SERVER HANDLERS =====
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Game Engine</title>
  <style>
    body { font-family: Arial; margin: 20px; background: #222; color: #fff; }
    .container { max-width: 600px; margin: auto; }
    h1 { color: #ffff00; }
    .button { 
      background: #0080ff; 
      color: white; 
      padding: 10px 20px; 
      border: none; 
      border-radius: 5px; 
      cursor: pointer; 
      margin: 10px 0;
      font-size: 16px;
    }
    .button:hover { background: #0060cc; }
    .upload-form { background: #333; padding: 20px; border-radius: 5px; margin: 20px 0; }
    input[type="file"] { margin: 10px 0; }
    .file-list { background: #333; padding: 10px; border-radius: 5px; margin: 20px 0; }
    .file-item { padding: 10px; background: #444; margin: 5px 0; border-radius: 3px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎮 ESP32 Game Engine</h1>
    
    <div class="upload-form">
      <h2>Upload Game</h2>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="game" accept=".bin,.hex,.elf" required>
        <button class="button" type="submit">Upload</button>
      </form>
    </div>
    
    <div class="file-list">
      <h2>Loaded Games</h2>
      <button class="button" onclick="loadList()">Refresh</button>
      <div id="gamesList"></div>
    </div>
  </div>
  
  <script>
    function loadList() {
      fetch('/list')
        .then(r => r.json())
        .then(games => {
          let html = '';
          games.forEach(game => {
            html += `<div class="file-item">📦 ${game.name} (${game.size} bytes)</div>`;
          });
          document.getElementById('gamesList').innerHTML = html || '<p>No games found</p>';
        });
    }
    loadList();
  </script>
</body>
</html>
  )";
  
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
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + file.size() + "}";
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
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/games/" + upload.filename;
    
    if (!SD.exists("/games")) {
      SD.mkdir("/games");
    }
    
    Serial.printf("[WEB] Upload started: %s\n", filename.c_str());
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Handle file write
    Serial.printf("[WEB] Uploading: %d bytes\n", upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("[WEB] Upload complete: %d bytes\n", upload.totalSize);
    server.send(200, "text/plain", "Upload successful!");
  }
}
