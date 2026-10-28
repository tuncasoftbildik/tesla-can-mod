#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "handlers.h"

// Waveshare ESP32-C6-LCD-1.47: ST7789V, 172x320
// SPI: MOSI=6, SCLK=7, CS=14, DC=15, RST=21, BL=22

#ifndef LCD_SCK_PIN
#define LCD_SCK_PIN 7
#endif
#ifndef LCD_MOSI_PIN
#define LCD_MOSI_PIN 6
#endif
#ifndef LCD_MISO_PIN
#define LCD_MISO_PIN -1
#endif
#ifndef LCD_CS_PIN
#define LCD_CS_PIN 14
#endif
#ifndef LCD_DC_PIN
#define LCD_DC_PIN 15
#endif
#ifndef LCD_RST_PIN
#define LCD_RST_PIN 21
#endif
#ifndef LCD_BL_PIN
#define LCD_BL_PIN 22
#endif
#ifndef LCD_WIDTH
#define LCD_WIDTH 172
#endif
#ifndef LCD_HEIGHT
#define LCD_HEIGHT 320
#endif
#ifndef LCD_ROTATION
#define LCD_ROTATION 0
#endif

// The vertical 8-row dashboard layout requires portrait orientation (height >= 320)
static_assert(LCD_ROTATION == 0 || LCD_ROTATION == 2,
              "LCDDisplay requires portrait orientation (LCD_ROTATION must be 0 or 2)");

class LCDDisplay
{
public:
    void init()
    {
        SPI.begin(LCD_SCK_PIN, LCD_MISO_PIN, LCD_MOSI_PIN, LCD_CS_PIN);
        tft_ = new Adafruit_ST7789(&SPI, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);
        tft_->init(LCD_WIDTH, LCD_HEIGHT);
        // Boost SPI clock to 40MHz for fast, flicker-free rendering
        tft_->setSPISpeed(40000000);
        // Enforce portrait rotation (0 or 2); landscape rotations 1 and 3 are rejected by the layout
        uint8_t rot = (LCD_ROTATION == 2) ? 2 : 0;
        tft_->setRotation(rot);
#if defined(LCD_INVERT) && (LCD_INVERT != 0)
        tft_->invertDisplay(true);
#endif
        tft_->fillScreen(BG_COLOR);

        // Backlight on
#if defined(LCD_BL_PIN) && (LCD_BL_PIN >= 0)
        pinMode(LCD_BL_PIN, OUTPUT);
        digitalWrite(LCD_BL_PIN, HIGH);
#endif

        drawHeader();
        drawStaticLayout();
    }

    void update(CarManagerBase *handler)
    {
        unsigned long now = millis();
        if (now - lastUpdate_ < 200)
            return;
        lastUpdate_ = now;

        // 1. FSD (Row 0, Y=55)
        bool fsd = handler ? handler->fsdEnabled : false;
        drawStatusValue(0, 55, fsd ? "ACTIVE" : "OFF", fsd ? GREEN : GRAY);

        // 2. CAN (Row 1, Y=85)
        bool canOK = handler && handler->frameCount > lastFrameCount_;
        lastFrameCount_ = handler ? handler->frameCount : 0;
        drawStatusValue(1, 85, canOK ? "RUNNING" : "WAITING", canOK ? GREEN : YELLOW);

        // 3. Mode (Row 2, Y=115)
        const char *profiles[] = {"Chill", "Normal", "Sport", "P3", "P4"};
        int prof = handler ? handler->speedProfile : 0;
        if (prof < 0 || prof > 4) prof = 0;
        drawStatusValue(2, 115, profiles[prof], CYAN);

        char buf[32];

        // 4. Battery SoC + Power (Row 3, Y=145)
        float soc = handler ? handler->socPercent : 0;
        float pw = handler ? handler->packPowerKW : 0;
        uint16_t socCol = soc > 60 ? GREEN : (soc > 30 ? YELLOW : RED);
        snprintf(buf, sizeof(buf), "%.0f%%  %.1fkW", soc, pw);
        drawStatusValue(3, 145, buf, socCol);

        // 5. Temperature (Row 4, Y=175)
        float tmin = handler ? handler->packTempMin : 0;
        float tmax = handler ? handler->packTempMax : 0;
        snprintf(buf, sizeof(buf), "%.0f~%.0f C", tmin, tmax);
        drawStatusValue(4, 175, buf, CYAN);

        // 6. Wh/km (Row 5, Y=205)
        float whkm = handler ? handler->whPerKm : 0;
        if (whkm > 0)
            snprintf(buf, sizeof(buf), "%.0f Wh/km", whkm);
        else
            snprintf(buf, sizeof(buf), "--");
        drawStatusValue(5, 205, buf, WHITE);

        // 7. Frames + Uptime (Row 6, Y=235)
        uint32_t rx = handler ? handler->frameCount : 0;
        uint32_t up = millis() / 1000;
        snprintf(buf, sizeof(buf), "RX:%lu %02d:%02d", (unsigned long)rx, (int)(up / 3600), (int)((up % 3600) / 60));
        drawStatusValue(6, 235, buf, GRAY);

        // 8. Precondition status (Row 7, Y=265)
        bool prec = handler ? handler->precondActive : false;
        bool allow = handler ? handler->precondAllowed : false;
        const char *ps = prec ? "HEATING" : (allow ? "ALLOWED" : "OFF");
        uint16_t pc = prec ? YELLOW : (allow ? GREEN : GRAY);
        drawStatusValue(7, 265, ps, pc);

        // Bottom performance bar
        drawPerfBar(handler);
    }

    void showMessage(const char *msg, uint16_t color = 0xFFFF)
    {
        msgExpiry_ = millis() + 2500;
        tft_->fillRect(0, 278, LCD_WIDTH, 42, BG_COLOR);
        tft_->drawRect(0, 278, LCD_WIDTH, 42, color);
        tft_->setTextColor(color, BG_COLOR);
        tft_->setTextSize(1);
        int16_t mw = strlen(msg) * 6;
        int16_t mx = (LCD_WIDTH > mw) ? (LCD_WIDTH - mw) / 2 : 8;
        tft_->setCursor(mx, 294);
        tft_->print(msg);
    }

private:
    static constexpr uint16_t BG_COLOR = 0x0821;
    static constexpr uint16_t GREEN    = 0x07E0;
    static constexpr uint16_t YELLOW   = 0xFFE0;
    static constexpr uint16_t CYAN     = 0x07FF;
    static constexpr uint16_t WHITE    = 0xFFFF;
    static constexpr uint16_t GRAY     = 0x4208;
    static constexpr uint16_t RED      = 0xF800;
    static constexpr uint16_t BLUE     = 0x001F;

    Adafruit_ST7789 *tft_ = nullptr;
    unsigned long lastUpdate_ = 0;
    uint32_t lastFrameCount_ = 0;
    unsigned long msgExpiry_ = 0;

    // Cache to eliminate screen flicker (no full row clears)
    struct RowCache {
        char value[32] = "";
        uint16_t color = 0;
    };
    RowCache rowCache_[8];

    char lastSpeed_[16] = "";
    char lastTorque_[16] = "";
    char lastAccel_[32] = "";

    void drawHeader()
    {
        tft_->fillRect(0, 0, LCD_WIDTH, 45, 0x1082);
        tft_->setTextColor(GREEN, 0x1082);
        tft_->setTextSize(2);
        int16_t titleX = (LCD_WIDTH > 96) ? (LCD_WIDTH - 96) / 2 : 20;
        tft_->setCursor(titleX, 8);
        tft_->print("TeslaCAN");
        tft_->setTextColor(GRAY, 0x1082);
        tft_->setTextSize(1);
        const char *sub = "ESP32-C6 FSD Mod";
        int16_t subw = strlen(sub) * 6;
        int16_t subX = (LCD_WIDTH > subw) ? (LCD_WIDTH - subw) / 2 : 12;
        tft_->setCursor(subX, 32);
        tft_->print(sub);
    }

    void drawStaticLayout()
    {
        tft_->setTextSize(1);
        tft_->setTextColor(GRAY, BG_COLOR);

        const char *labels[] = {
            "FSD", "CAN", "Mode", "Batt",
            "Temp", "Energy", "Stats", "Precond"
        };
        const int ys[] = {55, 85, 115, 145, 175, 205, 235, 265};

        for (int i = 0; i < 8; i++)
        {
            tft_->setCursor(8, ys[i] + 4);
            tft_->print(labels[i]);
        }

        initPerfBar();
    }

    void initPerfBar()
    {
        tft_->fillRect(0, 278, LCD_WIDTH, 42, 0x1082);
        tft_->setTextSize(1);
        tft_->setTextColor(GRAY, 0x1082);
        tft_->setCursor(8, 303);
        tft_->print("0-100");
    }

    void drawStatusValue(int rowIdx, int y, const char *value, uint16_t valueColor)
    {
        // Zero flicker: if content and color are identical, do nothing
        if (strcmp(rowCache_[rowIdx].value, value) == 0 && rowCache_[rowIdx].color == valueColor)
            return;

        int16_t oldLen = strlen(rowCache_[rowIdx].value);
        int16_t newLen = strlen(value);
        int16_t oldTw = oldLen * 6;
        int16_t newTw = newLen * 6;

        // If new text is shorter, clear only the leftover area on the left
        if (oldTw > newTw)
        {
            int16_t clearX = LCD_WIDTH - 8 - oldTw;
            int16_t clearW = oldTw - newTw + 2;
            tft_->fillRect(clearX, y + 2, clearW, 12, BG_COLOR);
        }

        // Render opaque text with background color in one pass (no wipe)
        tft_->setTextSize(1);
        tft_->setTextColor(valueColor, BG_COLOR);
        tft_->setCursor(LCD_WIDTH - 8 - newTw, y + 4);
        tft_->print(value);

        strncpy(rowCache_[rowIdx].value, value, sizeof(rowCache_[rowIdx].value) - 1);
        rowCache_[rowIdx].color = valueColor;
    }

    void drawPerfBar(CarManagerBase *handler)
    {
        if (msgExpiry_ != 0)
        {
            if (millis() < msgExpiry_)
                return;
            msgExpiry_ = 0;
            initPerfBar();
            lastSpeed_[0] = '\0';
            lastTorque_[0] = '\0';
            lastAccel_[0] = '\0';
        }

        float spd = handler ? handler->vehicleSpeedKph : 0;
        float trq = handler ? (handler->frontTorqueNm + handler->rearTorqueNm) : 0;
        char buf[32];

        // 1. Speed (left side of Y=284)
        snprintf(buf, sizeof(buf), "%.0f km/h", spd);
        if (strcmp(lastSpeed_, buf) != 0)
        {
            int16_t oldTw = strlen(lastSpeed_) * 6;
            int16_t newTw = strlen(buf) * 6;
            if (oldTw > newTw)
                tft_->fillRect(8 + newTw, 284, oldTw - newTw + 2, 10, 0x1082);

            tft_->setTextSize(1);
            tft_->setTextColor(CYAN, 0x1082);
            tft_->setCursor(8, 284);
            tft_->print(buf);
            strncpy(lastSpeed_, buf, sizeof(lastSpeed_) - 1);
        }

        // 2. Torque (right side of Y=284)
        snprintf(buf, sizeof(buf), "%.0f Nm", trq);
        if (strcmp(lastTorque_, buf) != 0)
        {
            int16_t oldTw = strlen(lastTorque_) * 6;
            int16_t newTw = strlen(buf) * 6;
            if (oldTw > newTw)
            {
                int16_t clearX = LCD_WIDTH - 8 - oldTw;
                tft_->fillRect(clearX, 284, oldTw - newTw + 2, 10, 0x1082);
            }

            tft_->setTextSize(1);
            tft_->setTextColor(WHITE, 0x1082);
            tft_->setCursor(LCD_WIDTH - 8 - newTw, 284);
            tft_->print(buf);
            strncpy(lastTorque_, buf, sizeof(lastTorque_) - 1);
        }

        // 3. 0-100 Accel (right side of Y=303)
        uint32_t last = handler ? handler->accel0to100Ms : 0;
        uint32_t best = handler ? handler->accel0to100BestMs : 0;
        if (last)
            snprintf(buf, sizeof(buf), "%.2fs  best %.2fs", last / 1000.0f, best / 1000.0f);
        else
            snprintf(buf, sizeof(buf), "--");

        if (strcmp(lastAccel_, buf) != 0)
        {
            int16_t oldTw = strlen(lastAccel_) * 6;
            int16_t newTw = strlen(buf) * 6;
            if (oldTw > newTw)
            {
                int16_t clearX = LCD_WIDTH - 8 - oldTw;
                tft_->fillRect(clearX, 303, oldTw - newTw + 2, 10, 0x1082);
            }

            tft_->setTextSize(1);
            tft_->setTextColor(last ? GREEN : GRAY, 0x1082);
            tft_->setCursor(LCD_WIDTH - 8 - newTw, 303);
            tft_->print(buf);
            strncpy(lastAccel_, buf, sizeof(lastAccel_) - 1);
        }
    }
};
