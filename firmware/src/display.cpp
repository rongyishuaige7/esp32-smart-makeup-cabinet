#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cstring>

// OLED显示屏配置
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 当前显示状态
static bool isInitialized = false;

void initDisplay() {
    // 初始化I2C通信
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    // 初始化OLED显示屏
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED display initialization failed!");
        isInitialized = false;
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();

    isInitialized = true;
    Serial.println("Display initialized");
}

void displayWelcome() {
    if (!isInitialized) return;

    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.println("Smart");
    display.setCursor(10, 40);
    display.println("Makeup Box");
    display.display();
}

void displaySensors(SensorData data) {
    if (!isInitialized) return;

    display.clearDisplay();
    display.setTextSize(1);

    // 第一行：温度
    display.setCursor(0, 0);
    display.print("Temp: ");
    if (data.climateAvailable) {
        display.print(data.temperature, 1);
        display.print(" C");
    } else {
        display.print("N/A");
    }

    // 第二行：湿度
    display.setCursor(0, 16);
    display.print("Humid: ");
    if (data.climateAvailable) {
        display.print(data.humidity, 1);
        display.print(" %");
    } else {
        display.print("N/A");
    }

    // 第三行：紫外线
    display.setCursor(0, 32);
    display.print("UV: ");
    if (data.uvAvailable) display.print(data.uvValue);
    else display.print("N/A");

    // 第四行：人体感应状态
    display.setCursor(0, 48);
    display.print("Motion: ");
    display.print(data.pirDetected ? "DETECTED" : "None");

    display.display();
}

void displayMessage(const char* line1, const char* line2) {
    if (!isInitialized) return;

    display.clearDisplay();
    display.setTextSize(2);

    display.setCursor(0, 20);
    display.println(line1);

    if (line2 && strlen(line2) > 0) {
        display.setTextSize(1);
        display.setCursor(0, 45);
        display.println(line2);
    }

    display.display();
}

void displayUVWarning(int uvValue) {
    if (!isInitialized) return;

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(10, 15);
    display.println("UV estimate");

    display.setTextSize(1);
    display.setCursor(10, 40);
    display.print("Module value: ");
    display.println(uvValue);

    display.setCursor(10, 52);
    display.println("Not safety advice");

    display.display();
}

void displayExpireWarning(const char* productName) {
    if (!isInitialized) return;

    display.clearDisplay();

    // This is a local data-record warning, not an item-authenticity or safety claim.
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 10);
    display.println("DATE FLAG");

    display.setTextSize(2);
    display.setCursor(0, 30);
    display.println(productName);

    display.display();
}

void clearDisplay() {
    if (!isInitialized) return;
    display.clearDisplay();
    display.display();
}

void updateDisplay() {
    if (!isInitialized) return;
    display.display();
}
