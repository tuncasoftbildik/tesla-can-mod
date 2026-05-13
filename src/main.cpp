#include <Arduino.h>
#include <memory>

// ─── CAN Driver & Application ───
#include "can_frame_types.h"
#include "drivers/can_driver.h"
#include "drivers/twai_driver.h"
#include "can_helpers.h"
#include "handlers.h"
#include "log_buffer.h"

// ─── LCD Display ───
#include "lcd_display.h"

// ─── Web Server ───
#include "web/web_server.h"

// ─── Flipper Zero UART Bridge ───
#include "uart_bridge.h"

// ─── Pin Configuration (Waveshare ESP32-C6-LCD-1.47) ───
#ifndef TWAI_TX_PIN
#define TWAI_TX_PIN 0
#endif
#ifndef TWAI_RX_PIN
#define TWAI_RX_PIN 1
#endif
#ifndef PIN_LED
#define PIN_LED 8
#endif

// ─── Globals ───
static std::unique_ptr<TWAIDriver> canDriver;
static std::unique_ptr<HW4Handler> handler;
static LCDDisplay lcd;

// Flipper Zero UART bridge (definition matches `extern` in uart_bridge.h)
UartBridge uartBridge;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("TeslaCAN ESP32-C6 starting...");

    // Init LED
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    // Init LCD
    lcd.init();
    lcd.showMessage("Initializing...", 0xFFE0); // Yellow
    globalLog.add("LCD initialized");

    // Init handler
    handler = std::make_unique<HW4Handler>();

    // Init CAN driver
    canDriver = std::make_unique<TWAIDriver>(
        static_cast<gpio_num_t>(TWAI_TX_PIN),
        static_cast<gpio_num_t>(TWAI_RX_PIN));

    if (!canDriver->init())
    {
        Serial.println("CAN init FAILED!");
        lcd.showMessage("CAN INIT FAILED!", 0xF800); // Red
        globalLog.add("CAN init FAILED");
    }
    else
    {
        canDriver->setFilters(handler->filterIds(), handler->filterIdCount());
        Serial.println("CAN init OK (500kbps)");
        lcd.showMessage("CAN Ready", 0x07E0); // Green
        globalLog.add("CAN init OK 500kbps");
    }

    // Init Web Server
    delay(500);
    webHandler = handler.get();
    webTwaiDriver = canDriver.get();
    webServerInit();
    lcd.showMessage("WiFi: TeslaCAN", 0x07FF); // Cyan
    globalLog.add("System ready");

#ifdef FLIPPER_UART_ENABLE
    uartBridge.begin(handler.get(), canDriver.get());
    globalLog.add("Flipper UART ready");
#endif

    Serial.println("Setup complete. Connect to WiFi: TeslaCAN");
    Serial.println("Dashboard: http://192.168.4.1");
}

void loop()
{
    // Process CAN frames
    CanFrame frame;
    while (canDriver->read(frame))
    {
        digitalWrite(PIN_LED, LOW);
        handler->frameCount++;
        handler->handleMessage(frame, *canDriver);
    }
    digitalWrite(PIN_LED, HIGH);

    // Update LCD display
    lcd.update(handler.get());

    // Handle web requests
    webServerLoop();

#ifdef FLIPPER_UART_ENABLE
    uartBridge.loop();
#endif
}
