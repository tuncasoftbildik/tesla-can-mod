#pragma once

#include <Arduino.h>
#include <Wire.h>

#ifdef TOUCH_ENABLE

#ifndef TOUCH_ROTATION
// Rotation 2 corresponds to 180° flip (invert X and Y), which matches
// Adafruit_ST7789 rotation 0 on the Waveshare ESP32-S3-Touch-LCD-2 board.
#define TOUCH_ROTATION 2
#endif

#ifndef LCD_WIDTH
#define LCD_WIDTH 240
#endif
#ifndef LCD_HEIGHT
#define LCD_HEIGHT 320
#endif

// CST816D Capacitive Touch Controller (I2C address 0x15)
// Default on Waveshare ESP32-S3-Touch-LCD-2: SDA=48, SCL=47
class TouchDriver
{
public:
    static constexpr uint8_t CST816_ADDR          = 0x15;
    static constexpr uint8_t CST816_REG_ID        = 0xA7;
    static constexpr uint8_t CST816_REG_NUM       = 0x02;
    static constexpr uint8_t CST816_REG_XH        = 0x03;
    static constexpr uint8_t CST816_REG_XL        = 0x04;
    static constexpr uint8_t CST816_REG_YH        = 0x05;
    static constexpr uint8_t CST816_REG_YL        = 0x06;

    bool init(int sdaPin = TOUCH_SDA_PIN, int sclPin = TOUCH_SCL_PIN, int intPin = TOUCH_INT_PIN)
    {
        sdaPin_ = sdaPin;
        sclPin_ = sclPin;
        intPin_ = intPin;

        Wire.begin(sdaPin_, sclPin_);
        Wire.setClock(400000);

        uint8_t chipId = 0;
        if (readReg(CST816_REG_ID, &chipId, 1))
        {
            available_ = true;
            return true;
        }
        return false;
    }

    bool getTouch(uint16_t &x, uint16_t &y, uint16_t *rawX = nullptr, uint16_t *rawY = nullptr)
    {
        if (!available_)
            return false;

        uint8_t touchNum = 0;
        if (!readReg(CST816_REG_NUM, &touchNum, 1) || touchNum == 0)
            return false;

        uint8_t raw[4];
        if (!readReg(CST816_REG_XH, raw, 4))
            return false;

        uint16_t rx = static_cast<uint16_t>(((raw[0] & 0x0F) << 8) | raw[1]);
        uint16_t ry = static_cast<uint16_t>(((raw[2] & 0x0F) << 8) | raw[3]);

        if (rawX) *rawX = rx;
        if (rawY) *rawY = ry;

        mapCoordinates(rx, ry, x, y, TOUCH_ROTATION);
        return true;
    }

    bool isAvailable() const { return available_; }

    static void mapCoordinates(uint16_t rx, uint16_t ry, uint16_t &x, uint16_t &y, uint8_t rotation)
    {
        switch (rotation)
        {
        case 1: // 90° clockwise
            x = ry;
            y = (rx < LCD_WIDTH) ? (LCD_WIDTH - 1 - rx) : 0;
            break;
        case 2: // 180° (Invert both X and Y — matches Adafruit_ST7789 portrait rotation 0)
            x = (rx < LCD_WIDTH) ? (LCD_WIDTH - 1 - rx) : 0;
            y = (ry < LCD_HEIGHT) ? (LCD_HEIGHT - 1 - ry) : 0;
            break;
        case 3: // 270° clockwise
            x = (ry < LCD_HEIGHT) ? (LCD_HEIGHT - 1 - ry) : 0;
            y = rx;
            break;
        case 4: // Invert Y only
            x = rx;
            y = (ry < LCD_HEIGHT) ? (LCD_HEIGHT - 1 - ry) : 0;
            break;
        case 5: // Invert X only
            x = (rx < LCD_WIDTH) ? (LCD_WIDTH - 1 - rx) : 0;
            y = ry;
            break;
        default: // Raw 0: no transform
            x = rx;
            y = ry;
            break;
        }
    }

private:
    bool readReg(uint8_t reg, uint8_t *data, uint8_t len)
    {
        Wire.beginTransmission(CST816_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(true) != 0)
            return false;

        if (Wire.requestFrom(static_cast<uint8_t>(CST816_ADDR), len) != len)
            return false;

        for (uint8_t i = 0; i < len; i++)
            data[i] = Wire.read();

        return true;
    }

    int sdaPin_ = -1;
    int sclPin_ = -1;
    int intPin_ = -1;
    bool available_ = false;
};

#endif // TOUCH_ENABLE
