#ifndef THERMAL_PRINTER_H
#define THERMAL_PRINTER_H

#include "driver/uart.h"
#include <string.h>

class ThermalPrinter {
private:
    uart_port_t _uart_num;
    
    static const uint8_t ESC = 0x1B;
    static const uint8_t GS = 0x1D;
    
    struct TextSettings {
        uint8_t size = 1;
        bool bold = false;
        bool underline = false;
        uint8_t align = 0;
        uint8_t lineSpacing = 30;
        uint8_t charSpacing = 0;
    } _currentSettings;
    
    void sendByte(uint8_t data) {
        uart_write_bytes(_uart_num, &data, 1);
    }

    void updatePrintMode() {
        uint8_t mode = 0;
        if (_currentSettings.bold) mode |= 0x08;
        if (_currentSettings.underline) mode |= 0x80;
        
        uint8_t widthMultiplier = (_currentSettings.size - 1) & 0x07;
        uint8_t heightMultiplier = ((_currentSettings.size - 1) & 0x07) << 4;
        uint8_t sizeCmd = widthMultiplier | heightMultiplier;

        sendByte(ESC);
        sendByte(0x21);
        sendByte(mode);
        
        sendByte(GS);
        sendByte('!');
        sendByte(sizeCmd);
    }

    uint8_t clamp_val(uint8_t val, uint8_t min_val, uint8_t max_val) {
        if (val < min_val) return min_val;
        if (val > max_val) return max_val;
        return val;
    }

public:
    enum Alignment {
        LEFT = 0,
        CENTER = 1,
        RIGHT = 2
    };
    
    ThermalPrinter(uart_port_t uart_num) : _uart_num(uart_num) {}
    
    void setSize(uint8_t size) {
        _currentSettings.size = clamp_val(size, 1, 8);
        updatePrintMode();
    }
    
    void setBold(bool enable) {
        _currentSettings.bold = enable;
        updatePrintMode();
    }
    
    void setUnderline(bool enable) {
        _currentSettings.underline = enable;
        updatePrintMode();
    }
    
    void setAlignment(Alignment align) {
        _currentSettings.align = align;
        sendByte(ESC);
        sendByte(0x61);
        sendByte(align);
    }
    
    void setLineSpacing(uint8_t spacing) {
        _currentSettings.lineSpacing = spacing;
        sendByte(ESC);
        sendByte(0x33);
        sendByte(spacing);
    }
    
    void setCharSpacing(uint8_t spacing) {
        _currentSettings.charSpacing = clamp_val(spacing, 0, 13);
        sendByte(ESC);
        sendByte(0x52);
        sendByte(_currentSettings.charSpacing);
    }
    
    void flush() {
        uart_wait_tx_done(_uart_num, pdMS_TO_TICKS(1000));
    }

    void cut(bool partial = true) {
        sendByte(GS);
        sendByte(0x56);
        sendByte(66);
        sendByte(partial ? 1 : 0);
        flush();
    }
    
    void print(const char* str) {
        uart_write_bytes(_uart_num, str, strlen(str));
    }
    
    void println(const char* str) {
        print(str);
        sendByte('\r');
        sendByte('\n');
    }
    
    void useHeaderStyle() {
        setSize(2);
        setBold(true);
        setAlignment(CENTER);
        setLineSpacing(50);
    }
    
    void useBodyStyle() {
        setSize(1);
        setBold(false);
        setAlignment(LEFT);
        setLineSpacing(30);
        setCharSpacing(0);
    }
    
    void resetSettings() {
        _currentSettings = TextSettings();
        updatePrintMode();
        setAlignment(LEFT);
        setLineSpacing(30);
        setCharSpacing(0);
    }
};

#endif
