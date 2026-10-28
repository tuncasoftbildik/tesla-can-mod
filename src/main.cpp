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

#ifdef TOUCH_ENABLE
#include "touch_driver.h"
static TouchDriver touch;
static unsigned long lastTouchTime = 0;
#endif

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
#if defined(BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_2)
    Serial.println("TeslaCAN ESP32-S3 Touch starting...");
#else
    Serial.println("TeslaCAN ESP32-C6 starting...");
#endif

    // Init LED (if available)
#if defined(PIN_LED) && (PIN_LED >= 0)
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);
#endif
    // Init LCD
    lcd.init();
    lcd.showMessage("Initializing...", 0xFFE0); // Yellow
    globalLog.add("LCD initialized");

#ifdef TOUCH_ENABLE
    if (touch.init())
    {
        Serial.println("Touch controller CST816D initialized");
        globalLog.add("Touch screen ready");
    }
    else
    {
        Serial.println("Touch controller CST816D not detected (optional)");
    }
#endif

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
#if defined(PIN_LED) && (PIN_LED >= 0)
        digitalWrite(PIN_LED, LOW);
#endif
        handler->frameCount++;
        handler->handleMessage(frame, *canDriver);
    }
#if defined(PIN_LED) && (PIN_LED >= 0)
    digitalWrite(PIN_LED, HIGH);
#endif

#ifdef TOUCH_ENABLE
    static unsigned long lastTouchPoll = 0;
    unsigned long nowTouch = millis();
    if (nowTouch - lastTouchPoll >= 30) // Poll I2C touch at ~33 Hz
    {
        lastTouchPoll = nowTouch;
        uint16_t tx = 0, ty = 0, rawX = 0, rawY = 0;
        if (touch.getTouch(tx, ty, &rawX, &rawY))
        {
            if (nowTouch - lastTouchTime > 350)
            {
                lastTouchTime = nowTouch;
                Serial.printf("[Touch] x=%u, y=%u (raw rx=%u, ry=%u)\n", tx, ty, rawX, rawY);

                // Tappable status rows on portrait layout:
                if (ty >= 45 && ty < 85) // FSD row
                {
                    handler->fsdEnabled = !handler->fsdEnabled;
                    globalLog.add(handler->fsdEnabled ? "Touch: FSD enabled" : "Touch: FSD disabled");
                    lcd.showMessage(handler->fsdEnabled ? "FSD: ACTIVE" : "FSD: OFF", handler->fsdEnabled ? 0x07E0 : 0x4208);
                    lcd.update(handler.get());
                }
                else if (ty >= 105 && ty < 145) // Mode row
                {
                    handler->speedProfile = (handler->speedProfile + 1) % 5;
                    const char *pnames[] = {"Chill", "Normal", "Sport", "P3", "P4"};
                    char msg[32];
                    snprintf(msg, sizeof(msg), "Mode: %s", pnames[handler->speedProfile]);
                    globalLog.add(msg);
                    lcd.showMessage(msg, 0x07FF);
                    lcd.update(handler.get());
                }
                else if (ty >= 250 && ty < 295) // Precond row
                {
                    handler->precondRequested = !handler->precondRequested;
                    if (!handler->precondRequested)
                        handler->precondActive = false;
                    globalLog.add(handler->precondRequested ? "Touch: Precond ON" : "Touch: Precond OFF");
                    lcd.showMessage(handler->precondRequested ? "Precond: START" : "Precond: STOP", handler->precondRequested ? 0xFFE0 : 0x4208);
                    lcd.update(handler.get());
                }
            }
        }
    }
#endif
    lcd.update(handler.get());

    // Handle web requests
    webServerLoop();

#ifdef FLIPPER_UART_ENABLE
    uartBridge.loop();
#endif
}
