#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "handlers.h"

// Waveshare ESP32-C6-LCD-1.47: ST7789V, 172x320
// SPI: MOSI=6, SCLK=2, CS=14, DC=15, RST=21, BL=22

class LCDDisplay
{
public:
    void init()
    {
        SPI.begin(7 /*SCK*/, -1 /*MISO*/, 6 /*MOSI*/, 14 /*CS*/);
        tft_ = new Adafruit_ST7789(&SPI, 14 /*CS*/, 15 /*DC*/, 21 /*RST*/);
        tft_->init(172, 320);
        tft_->setRotation(0);
        tft_->fillScreen(BG_COLOR);

        // Backlight on
        pinMode(22, OUTPUT);
        digitalWrite(22, HIGH);

        drawHeader();
    }

    void update(CarManagerBase *handler)
    {
        unsigned long now = millis();
        if (now - lastUpdate_ < 300)
            return;
        lastUpdate_ = now;

        bool fsd = handler ? handler->fsdEnabled : false;
        drawStatusRow(55, "FSD", fsd ? "ACTIVE" : "OFF", fsd ? GREEN : GRAY);

        bool canOK = handler && handler->frameCount > lastFrameCount_;
        lastFrameCount_ = handler ? handler->frameCount : 0;
        drawStatusRow(85, "CAN", canOK ? "RUNNING" : "WAITING", canOK ? GREEN : YELLOW);

        const char *profiles[] = {"Chill", "Normal", "Sport", "P3", "P4"};
        int prof = handler ? handler->speedProfile : 0;
        if (prof < 0 || prof > 4) prof = 0;
        drawStatusRow(115, "Mode", profiles[prof], CYAN);

        char buf[32];

        // Battery SoC + Power
        float soc = handler ? handler->socPercent : 0;
        float pw = handler ? handler->packPowerKW : 0;
        uint16_t socCol = soc > 60 ? GREEN : (soc > 30 ? YELLOW : RED);
        snprintf(buf, sizeof(buf), "%.0f%%  %.1fkW", soc, pw);
        drawStatusRow(145, "Batt", buf, socCol);

        // Temperature
        float tmin = handler ? handler->packTempMin : 0;
        float tmax = handler ? handler->packTempMax : 0;
        snprintf(buf, sizeof(buf), "%.0f~%.0f C", tmin, tmax);
        drawStatusRow(175, "Temp", buf, CYAN);

        // Wh/km
        float whkm = handler ? handler->whPerKm : 0;
        if (whkm > 0)
            snprintf(buf, sizeof(buf), "%.0f Wh/km", whkm);
        else
            snprintf(buf, sizeof(buf), "--");
        drawStatusRow(205, "Energy", buf, WHITE);

        // Frames + Uptime compact
        uint32_t rx = handler ? handler->frameCount : 0;
        uint32_t up = millis() / 1000;
        snprintf(buf, sizeof(buf), "RX:%lu %02d:%02d", (unsigned long)rx, (int)(up / 3600), (int)((up % 3600) / 60));
        drawStatusRow(235, "Stats", buf, GRAY);

        // Precondition status (active / BMS allowed)
        bool prec = handler ? handler->precondActive : false;
        bool allow = handler ? handler->precondAllowed : false;
        const char *ps = prec ? "HEATING" : (allow ? "ALLOWED" : "OFF");
        uint16_t pc = prec ? YELLOW : (allow ? GREEN : GRAY);
        drawStatusRow(265, "Precond", ps, pc);
    }

    void showMessage(const char *msg, uint16_t color = 0xFFFF)
    {
        tft_->fillRect(0, 280, 172, 40, BG_COLOR);
        tft_->setTextColor(color);
        tft_->setCursor(10, 295);
        tft_->setTextSize(1);
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

    void drawHeader()
    {
        tft_->fillRect(0, 0, 172, 45, 0x1082);
        tft_->setTextColor(GREEN);
        tft_->setTextSize(2);
        tft_->setCursor(20, 8);
        tft_->print("TeslaCAN");
        tft_->setTextColor(GRAY);
        tft_->setTextSize(1);
        tft_->setCursor(12, 32);
        tft_->print("ESP32-C6 FSD Mod");
    }

    void drawStatusRow(int y, const char *label, const char *value, uint16_t valueColor)
    {
        tft_->fillRect(0, y, 172, 24, BG_COLOR);
        tft_->setTextSize(1);
        tft_->setTextColor(GRAY);
        tft_->setCursor(8, y + 4);
        tft_->print(label);
        tft_->setTextColor(valueColor);
        int16_t tw = strlen(value) * 6;
        tft_->setCursor(164 - tw, y + 4);
        tft_->print(value);
    }
};
