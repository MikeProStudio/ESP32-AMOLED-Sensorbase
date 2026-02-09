#ifndef _FT3168_H
#define _FT3168_H

#include <Arduino.h>
#include <Wire.h>

// Entferne das harte Define
// #define I2C_ADDR_FT3168 0x38

enum GESTURE
{
    None = 0x00, SlideDown = 0x08, SlideUp = 0x04, SlideLeft = 0x01, 
    SlideRight = 0x02, SingleTap = 0x00, DoubleTap = 0x10, LongPress = 0x00
};

class FT3168
{
public:
    FT3168(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin);

    // begin() gibt jetzt zurück, ob der Chip gefunden wurde!
    bool begin(uint8_t addr = 0x38); 
    
    bool getTouch(uint16_t *x, uint16_t *y, uint8_t *gesture);

private:
    int8_t _sda, _scl, _rst, _int;
    uint8_t _addr; // Wir speichern die Adresse hier

    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
    void readStruct(uint8_t reg, uint8_t *buf, uint8_t len);
};

#endif