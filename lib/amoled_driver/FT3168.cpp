#include "FT3168.h"

FT3168::FT3168(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin) 
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
    _addr = 0x38; // Default
}

bool FT3168::begin(uint8_t addr)
{
    _addr = addr;
    // Wire.begin(_sda, _scl); <-- Kann weg, wenn es in setup() steht!
    
    // Reset Sequence (Wichtig für Stabilität)
    if (_rst != -1) {
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, LOW);
        delay(10);
        digitalWrite(_rst, HIGH);
        delay(100);
    }

    // Check ob der Chip antwortet
    Wire.beginTransmission(_addr);
    if (Wire.endTransmission() != 0) {
        return false; 
    }
    return true;
}

bool FT3168::getTouch(uint16_t *x, uint16_t *y, uint8_t *gesture)
{
    // Register 0x02 lesen (Anzahl der Finger)
    Wire.beginTransmission(_addr);
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) return false; 
    
    if (Wire.requestFrom(_addr, (uint8_t)1) == 0) return false;
    uint8_t status = Wire.read();

    if (status > 0 && status < 6) {
        uint8_t buf[4];
        Wire.beginTransmission(_addr);
        Wire.write(0x03); // Start der Koordinaten
        Wire.endTransmission(false);
        
        if (Wire.requestFrom(_addr, (uint8_t)4) == 4) {
            // Wir lesen die Rohwerte OHNE sie im Treiber zu vertauschen
            // damit wir in der main.cpp wissen, was wir tun.
            uint16_t raw_0_1 = ((Wire.read() & 0x0F) << 8) | Wire.read();
            uint16_t raw_2_3 = ((Wire.read() & 0x0F) << 8) | Wire.read();
            
            // Bei diesem Display ist oft:
            // raw_0_1 = Koordinate der langen Seite (0-535)
            // raw_2_3 = Koordinate der kurzen Seite (0-239)
            *x = raw_0_1; 
            *y = raw_2_3;
            return true;
        }
    }
    return false;
}

void FT3168::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t FT3168::readReg(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, (uint8_t)1);
    if(Wire.available()) return Wire.read();
    return 0;
}

void FT3168::readStruct(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_addr, len);
    for(int i=0; i<len; i++) {
        if(Wire.available()) buf[i] = Wire.read();
    }
}