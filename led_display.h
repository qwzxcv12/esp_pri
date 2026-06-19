#ifndef LED_DISPLAY_H
#define LED_DISPLAY_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "Arduino.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeSansBold12pt7b.h> 

// Define the connected PINs between P5 and ESP32
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1  
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

// Define P5 Panel configuration
#define PANEL_RES_X 64  
#define PANEL_RES_Y 32  
#define PANEL_CHAIN 1   

// Global pointers
inline MatrixPanel_I2S_DMA *dma_display = nullptr;
inline U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Text display variables
inline char textToDisplay[250] = "Ready"; 
inline uint16_t textColor = 0xFFFF;    // Default white
inline bool needRedraw = true;
inline bool show_startup_messages = true;

// Scrolling variables
inline bool isScrolling = false;
inline int scrollPosition = 0;
inline unsigned long lastScrollTime = 0;
inline int scrollDelay = 50;  
inline int textWidth = 0;

// Text content type
enum ContentType {
  MIXED_CONTENT,
  ONLY_TEXT,
  ONLY_NUMBERS
};

inline ContentType currentContentType = MIXED_CONTENT;

// Color mapping
struct ColorMapping {
  const char* name;
  uint16_t color;
};

inline const ColorMapping colorDict[] = {
  {"do", 0xF800},      // Red
  {"xanh", 0x07E0},    // Green
  {"lam", 0x001F},     // Blue
  {"vang", 0xFFE0},    // Yellow
  {"tim", 0x780F},     // Purple
  {"cam", 0xFD20},     // Orange
  {"trang", 0xFFFF},   // White
  {"hong", 0xF81F}     // Pink
};

inline const int NUM_COLORS = sizeof(colorDict) / sizeof(colorDict[0]);

struct TextSegment {
  char text[50];  
  bool isNumeric;  
};

// Font selection
inline const uint8_t* VIETNAMESE_FONT = u8g2_font_unifont_t_vietnamese1;

// Analyze content type
inline ContentType analyzeContentType(const char* text) {
  if (strstr(text, "IP:") != nullptr || strstr(text, "WiFi") != nullptr || strstr(text, "AP:") != nullptr) {
    return ONLY_TEXT;
  }
  bool hasDigits = false;
  bool hasNonDigits = false;
  
  for (int i = 0; text[i] != '\0'; i++) {
    if (isdigit((unsigned char)text[i]) || text[i] == '.' || text[i] == '-' || text[i] == '+') {
      hasDigits = true;
    } else if (text[i] != ' ') {
      hasNonDigits = true;
    }
  }
  
  if (hasDigits && !hasNonDigits) return ONLY_NUMBERS;
  if (!hasDigits && hasNonDigits) return ONLY_TEXT;
  return MIXED_CONTENT;
}

// Break text into segments
inline void segmentText(const char* input, TextSegment* segments, int maxSegments, int* segmentCount) {
  *segmentCount = 0;
  int inputLen = strlen(input);
  if (inputLen == 0) return;
  
  int segmentStart = 0;
  bool currentIsNumeric = isdigit((unsigned char)input[0]) || input[0] == '-' || input[0] == '+' || input[0] == '.';
  
  for (int i = 0; i <= inputLen; i++) {
    bool isNumChar = i < inputLen && (isdigit((unsigned char)input[i]) || input[i] == '-' || input[i] == '+' || input[i] == '.');
    
    if (i == inputLen || isNumChar != currentIsNumeric) {
      if (*segmentCount < maxSegments) {
        int segmentLen = i - segmentStart;
        if (segmentLen > 0) {
          int copyLen = segmentLen < 49 ? segmentLen : 49;
          strncpy(segments[*segmentCount].text, input + segmentStart, copyLen);
          segments[*segmentCount].text[copyLen] = '\0';
          segments[*segmentCount].isNumeric = currentIsNumeric;
          (*segmentCount)++;
        }
      }
      segmentStart = i;
      currentIsNumeric = isNumChar;
    }
  }
}

// Calculate mixed text width
inline int calculateMixedTextWidth(const char* text) {
  TextSegment segments[20]; 
  int segmentCount = 0;
  int totalWidth = 0;
  
  segmentText(text, segments, 20, &segmentCount);
  
  for (int i = 0; i < segmentCount; i++) {
    if (segments[i].isNumeric) {
      dma_display->setFont(&FreeSansBold12pt7b);
      int16_t x1, y1;
      uint16_t w, h;
      dma_display->getTextBounds(segments[i].text, 0, 0, &x1, &y1, &w, &h);
      totalWidth += w;
    } else {
      u8g2Fonts.setFont(VIETNAMESE_FONT);
      totalWidth += u8g2Fonts.getUTF8Width(segments[i].text);
    }
  }
  
  return totalWidth;
}

// Draw/Update scrolling text
inline void updateScrollingText() {
  if (!dma_display) return;
  dma_display->clearScreen();
  
  int16_t yPos = PANEL_RES_Y / 2 + 6; 
  
  if (currentContentType == ONLY_NUMBERS) {
    dma_display->setTextColor(textColor);
    dma_display->setFont(&FreeSansBold12pt7b);
    dma_display->setCursor(-scrollPosition, yPos);
    dma_display->print(textToDisplay);
  } 
  else if (currentContentType == ONLY_TEXT) {
    u8g2Fonts.setFont(VIETNAMESE_FONT);
    u8g2Fonts.setForegroundColor(textColor);
    u8g2Fonts.setCursor(-scrollPosition, yPos);
    u8g2Fonts.print(textToDisplay);
  }
  else {
    TextSegment segments[20]; 
    int segmentCount = 0;
    segmentText(textToDisplay, segments, 20, &segmentCount);
    
    int currentX = -scrollPosition;
    for (int i = 0; i < segmentCount; i++) {
      if (segments[i].isNumeric) {
        dma_display->setTextColor(textColor);
        dma_display->setFont(&FreeSansBold12pt7b);
        dma_display->setCursor(currentX, yPos);
        dma_display->print(segments[i].text);
        
        int16_t x1, y1;
        uint16_t w, h;
        dma_display->getTextBounds(segments[i].text, 0, 0, &x1, &y1, &w, &h);
        currentX += w;
      } else {
        u8g2Fonts.setFont(VIETNAMESE_FONT);
        u8g2Fonts.setForegroundColor(textColor);
        u8g2Fonts.setCursor(currentX, yPos);
        u8g2Fonts.print(segments[i].text);
        currentX += u8g2Fonts.getUTF8Width(segments[i].text);
      }
    }
  }
}

// Process received display command
inline void processMessage(const char* command) {
  if (strcmp(command, "clear") == 0 || strcmp(command, "clear_display") == 0 || strlen(command) == 0) {
    strcpy(textToDisplay, "");
    if (dma_display) dma_display->clearScreen();
    needRedraw = true;
    isScrolling = false;
    return;
  }

  char colorName[16] = {0};
  char text[250] = {0}; 
  
  const char* space = strchr(command, ' ');
  
  if (space == NULL) {
    strncpy(text, command, 249);
  } else {
    int colorNameLen = space - command;
    if (colorNameLen < 15) {
      strncpy(colorName, command, colorNameLen);
      colorName[colorNameLen] = '\0';
    }
    strncpy(text, space + 1, 249);
  }
  
  if (strlen(colorName) > 0) {
    bool colorFound = false;
    for (int i = 0; i < NUM_COLORS; i++) {
      if (strcmp(colorName, colorDict[i].name) == 0) {
        textColor = colorDict[i].color;
        colorFound = true;
        break;
      }
    }
    if (!colorFound) {
      // If color not matched, consider the whole command as text and use white
      strncpy(text, command, 249);
      textColor = 0xFFFF;
    }
  } else {
    textColor = 0xFFFF; // Default white
  }
  
  if (strlen(text) > 0) {
    strcpy(textToDisplay, text);
    currentContentType = analyzeContentType(textToDisplay);
    scrollPosition = 0;  
    needRedraw = true;   
  }
}

// Display layout calculation
inline void updateDisplay() {
  if (!dma_display) return;
  dma_display->clearScreen();
  
  currentContentType = analyzeContentType(textToDisplay);
  
  if (currentContentType == ONLY_NUMBERS) {
    dma_display->setTextColor(textColor);
    dma_display->setFont(&FreeSansBold12pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    dma_display->getTextBounds(textToDisplay, 0, 0, &x1, &y1, &w, &h);
    textWidth = w;
  } 
  else if (currentContentType == ONLY_TEXT) {
    u8g2Fonts.setFont(VIETNAMESE_FONT);
    textWidth = u8g2Fonts.getUTF8Width(textToDisplay);
  }
  else {
    textWidth = calculateMixedTextWidth(textToDisplay);
  }
  
  int16_t yPos = PANEL_RES_Y / 2 + 6; 
  
  if (strlen(textToDisplay) <= 4 && currentContentType != MIXED_CONTENT) {
    isScrolling = false;
    int16_t xPos = (PANEL_RES_X - textWidth) / 2; 
    
    if (currentContentType == ONLY_NUMBERS) {
      dma_display->setTextColor(textColor);
      dma_display->setFont(&FreeSansBold12pt7b);
      dma_display->setCursor(xPos, yPos);
      dma_display->print(textToDisplay);
    } 
    else if (currentContentType == ONLY_TEXT) {
      u8g2Fonts.setFont(VIETNAMESE_FONT);
      u8g2Fonts.setForegroundColor(textColor);
      u8g2Fonts.setCursor(xPos, yPos);
      u8g2Fonts.print(textToDisplay);
    }
  }
  else if (textWidth <= PANEL_RES_X) {
    isScrolling = false;
    int16_t xPos = (PANEL_RES_X - textWidth) / 2; 
    
    if (currentContentType == ONLY_NUMBERS) {
      dma_display->setTextColor(textColor);
      dma_display->setFont(&FreeSansBold12pt7b);
      dma_display->setCursor(xPos, yPos);
      dma_display->print(textToDisplay);
    } 
    else if (currentContentType == ONLY_TEXT) {
      u8g2Fonts.setFont(VIETNAMESE_FONT);
      u8g2Fonts.setForegroundColor(textColor);
      u8g2Fonts.setCursor(xPos, yPos);
      u8g2Fonts.print(textToDisplay);
    }
    else {
      TextSegment segments[20]; 
      int segmentCount = 0;
      segmentText(textToDisplay, segments, 20, &segmentCount);
      
      int currentX = xPos;
      for (int i = 0; i < segmentCount; i++) {
        if (segments[i].isNumeric) {
          dma_display->setTextColor(textColor);
          dma_display->setFont(&FreeSansBold12pt7b);
          dma_display->setCursor(currentX, yPos);
          dma_display->print(segments[i].text);
          
          int16_t x1, y1;
          uint16_t w, h;
          dma_display->getTextBounds(segments[i].text, 0, 0, &x1, &y1, &w, &h);
          currentX += w;
        } else {
          u8g2Fonts.setFont(VIETNAMESE_FONT);
          u8g2Fonts.setForegroundColor(textColor);
          u8g2Fonts.setCursor(currentX, yPos);
          u8g2Fonts.print(segments[i].text);
          currentX += u8g2Fonts.getUTF8Width(segments[i].text);
        }
      }
    }
  } 
  else if (currentContentType == ONLY_TEXT && textWidth <= PANEL_RES_X * 2 && strchr(textToDisplay, ' ') != NULL) {
    isScrolling = false;
    int halfLength = strlen(textToDisplay) / 2;
    int breakPos = -1;
    
    for (int i = halfLength - 5; i <= halfLength + 5 && i < (int)strlen(textToDisplay); i++) {
      if (i > 0 && textToDisplay[i] == ' ') {
        breakPos = i;
        break;
      }
    }
    
    bool splitSuccess = false;
    if (breakPos > 0) {
      char firstLine[125]; 
      strncpy(firstLine, textToDisplay, breakPos);
      firstLine[breakPos] = '\0';
      
      u8g2Fonts.setFont(VIETNAMESE_FONT);
      int width1 = u8g2Fonts.getUTF8Width(firstLine);
      int width2 = u8g2Fonts.getUTF8Width(&textToDisplay[breakPos+1]);
      
      if (width1 <= PANEL_RES_X && width2 <= PANEL_RES_X) {
        u8g2Fonts.setForegroundColor(textColor);
        u8g2Fonts.setCursor((PANEL_RES_X - width1) / 2, PANEL_RES_Y / 2 - 5);
        u8g2Fonts.print(firstLine);
        
        u8g2Fonts.setCursor((PANEL_RES_X - width2) / 2, PANEL_RES_Y / 2 + 10);
        u8g2Fonts.print(&textToDisplay[breakPos+1]);
        splitSuccess = true;
      }
    }
    
    if (!splitSuccess) {
      isScrolling = true;
      scrollPosition = -PANEL_RES_X;
      updateScrollingText();
    }
  } 
  else {
    isScrolling = true;
    scrollPosition = -PANEL_RES_X;  
    updateScrollingText();
  }
}

// Scrolling handler
inline void handleScrolling() {
  if (isScrolling) {
    unsigned long currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (currentTime - lastScrollTime > (unsigned long)scrollDelay) {
      lastScrollTime = currentTime;
      scrollPosition++;
      if (scrollPosition > textWidth + PANEL_RES_X) {
        scrollPosition = -PANEL_RES_X;
      }
      updateScrollingText();
    }
  }
}

// Task loop for LED scanning and scrolling
inline void led_display_task(void *pvParameters) {
  while (1) {
    if (needRedraw) {
      updateDisplay();
      needRedraw = false;
    }
    handleScrolling();
    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}

// Task to read commands from Serial/UART
inline void serial_command_task(void *pvParameters) {
  char buffer[256];
  int idx = 0;
  while (1) {
    int c = getchar();
    if (c != EOF && c != 0xFF && c != 0) {
      if (c == '\n' || c == '\r') {
        if (idx > 0) {
          buffer[idx] = '\0';
          // Declare add_device_log extern since it is in mqtt_handler.h
          extern void add_device_log(const char* format, ...);
          add_device_log("Serial Command: %s", buffer);
          processMessage(buffer);
          idx = 0;
        }
      } else if (idx < sizeof(buffer) - 1) {
        buffer[idx++] = (char)c;
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// Main initialization
inline void setup_led_display() {
  HUB75_I2S_CFG::i2s_pins pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
  
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   
    PANEL_RES_Y,   
    PANEL_CHAIN,   
    pins           
  );
  
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(100);  
  
  u8g2Fonts.begin(*dma_display);
  dma_display->clearScreen();
  
  u8g2Fonts.setFont(VIETNAMESE_FONT); 
  u8g2Fonts.setForegroundColor(dma_display->color565(255, 180, 84));
  u8g2Fonts.setCursor(5, 20);
  u8g2Fonts.print("LED Ready");
  
  xTaskCreate(led_display_task, "led_display_task", 4096, NULL, 5, NULL);
  // serial_command_task disabled - use web UI for testing instead
  // xTaskCreate(serial_command_task, "serial_command_task", 4096, NULL, 5, NULL);
}

#endif // LED_DISPLAY_H
