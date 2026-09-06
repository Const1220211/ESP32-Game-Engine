// ST7735S 128x160 Initialization Fix для ESP32-S3
// Прямая инициализация без TFT_eSPI библиотеки

#include <SPI.h>

// Pin definitions
#define TFT_CS   5
#define TFT_RST  17
#define TFT_DC   16
#define TFT_MOSI 11
#define TFT_SCK  12
#define TFT_MISO 13
#define TFT_BL   4

// ST7735S Commands
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09
#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E
#define ST7735_PTLAR   0x30
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD
#define ST7735_PWCTR6  0xFC
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

class ST7735S_Display {
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
    
    void writeDataBurst(uint8_t* data, uint32_t len) {
        digitalWrite(TFT_DC, HIGH);
        digitalWrite(TFT_CS, LOW);
        spi->writeBytes(data, len);
        digitalWrite(TFT_CS, HIGH);
    }

public:
    ST7735S_Display() {
        spi = &SPI;
    }
    
    void init() {
        // Инициализация GPIO
        pinMode(TFT_CS, OUTPUT);
        pinMode(TFT_RST, OUTPUT);
        pinMode(TFT_DC, OUTPUT);
        pinMode(TFT_BL, OUTPUT);
        
        // Инициализация SPI
        spi->begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
        spi->setFrequency(10000000);  // 10MHz для стабильности
        spi->setDataMode(SPI_MODE0);
        spi->setBitOrder(SPI_MSBFIRST);
        
        // Reset дисплея
        digitalWrite(TFT_RST, HIGH);
        delay(10);
        digitalWrite(TFT_RST, LOW);
        delay(10);
        digitalWrite(TFT_RST, HIGH);
        delay(120);
        
        // Включить подсветку
        digitalWrite(TFT_BL, HIGH);
        
        // Инициализация ST7735S
        writeCommand(ST7735_SWRESET);  // Software reset
        delay(150);
        
        writeCommand(ST7735_SLPOUT);   // Sleep out
        delay(500);
        
        // Frame rate control
        writeCommand(ST7735_FRMCTR1);
        writeData(0x01);
        writeData(0x2C);
        writeData(0x2D);
        
        writeCommand(ST7735_FRMCTR2);
        writeData(0x01);
        writeData(0x2C);
        writeData(0x2D);
        
        writeCommand(ST7735_FRMCTR3);
        writeData(0x01);
        writeData(0x2C);
        writeData(0x2D);
        
        // Inversion control
        writeCommand(ST7735_INVCTR);
        writeData(0x07);
        
        // Power control
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
        
        // Voltage control
        writeCommand(ST7735_VMCTR1);
        writeData(0x0E);
        
        // Memory access control (RGB, не инвертировано)
        writeCommand(ST7735_MADCTL);
        writeData(0xC8);  // BGR, Y-mirror
        
        // Color mode
        writeCommand(ST7735_COLMOD);
        writeData(0x05);  // 16-bit color
        
        // Gamma correction
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
        
        // Display on
        writeCommand(ST7735_DISPON);
        delay(100);
        
        Serial.println("[ST7735S] Display initialized successfully!");
    }
    
    void fillScreen(uint16_t color) {
        fillRect(0, 0, 128, 160, color);
    }
    
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
        // Column address set
        writeCommand(ST7735_CASET);
        writeData(0);
        writeData(x);
        writeData(0);
        writeData(x + w - 1);
        
        // Row address set
        writeCommand(ST7735_RASET);
        writeData(0);
        writeData(y);
        writeData(0);
        writeData(y + h - 1);
        
        // Write memory
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
        fillRect(x, y, 1, 1, color);
    }
    
    void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
        int16_t dx = abs(x1 - x0);
        int16_t dy = abs(y1 - y0);
        int16_t sx = (x0 < x1) ? 1 : -1;
        int16_t sy = (y0 < y1) ? 1 : -1;
        int16_t err = dx - dy;
        
        while (true) {
            drawPixel(x0, y0, color);
            
            if (x0 == x1 && y0 == y1) break;
            
            int16_t e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
    
    void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
        drawLine(x, y, x + w - 1, y, color);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
        drawLine(x + w - 1, y + h - 1, x, y + h - 1, color);
        drawLine(x, y + h - 1, x, y, color);
    }
    
    void drawText(uint16_t x, uint16_t y, const char* text, uint16_t color) {
        // Простая текстовая функция - выводит символы 5x8
        uint16_t cx = x;
        uint16_t cy = y;
        
        while (*text) {
            if (*text == '\n') {
                cx = x;
                cy += 10;
            } else {
                drawChar(cx, cy, *text, color);
                cx += 6;
            }
            text++;
        }
    }
    
    void drawChar(uint16_t x, uint16_t y, char c, uint16_t color) {
        // Простой символ 5x8
        if (c >= 32 && c <= 126) {
            fillRect(x, y, 5, 8, COLOR_BLACK);
            // Здесь можно добавить битовые шрифты
        }
    }
    
    void setBacklight(bool on) {
        digitalWrite(TFT_BL, on ? HIGH : LOW);
    }
};

// Глобальный объект дисплея
ST7735S_Display display;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n=== ST7735S 128x160 Test ===");
    
    // Инициализация дисплея
    display.init();
    delay(500);
    
    // Test: заливка цветами
    Serial.println("Testing colors...");
    display.fillScreen(COLOR_RED);
    delay(500);
    display.fillScreen(COLOR_GREEN);
    delay(500);
    display.fillScreen(COLOR_BLUE);
    delay(500);
    display.fillScreen(COLOR_BLACK);
    delay(500);
    
    // Test: рисование линий
    Serial.println("Drawing lines...");
    display.drawLine(0, 0, 127, 159, COLOR_WHITE);
    display.drawLine(127, 0, 0, 159, COLOR_WHITE);
    display.drawLine(0, 80, 127, 80, COLOR_YELLOW);
    display.drawLine(64, 0, 64, 159, COLOR_CYAN);
    
    // Test: рисование прямоугольников
    Serial.println("Drawing rectangles...");
    display.drawRect(10, 10, 50, 50, COLOR_GREEN);
    display.drawRect(70, 100, 50, 50, COLOR_MAGENTA);
    
    Serial.println("Test complete!");
}

void loop() {
    delay(1000);
}
