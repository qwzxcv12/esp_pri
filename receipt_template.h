#ifndef RECEIPT_TEMPLATE_H
#define RECEIPT_TEMPLATE_H

#include "thermal_printer.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string>
#include <string.h>

struct AccentPair {
    const char* utf8;
    char ascii;
};

static const AccentPair VIETNAMESE_ACCENTS[] = {
    // Lowercase a
    {"à", 'a'}, {"á", 'a'}, {"ả", 'a'}, {"ã", 'a'}, {"ạ", 'a'},
    {"ă", 'a'}, {"ằ", 'a'}, {"ắ", 'a'}, {"ẳ", 'a'}, {"ẵ", 'a'}, {"ặ", 'a'},
    {"â", 'a'}, {"ầ", 'a'}, {"ấ", 'a'}, {"ẩ", 'a'}, {"ẫ", 'a'}, {"ậ", 'a'},
    // Uppercase A
    {"À", 'A'}, {"Á", 'A'}, {"Ả", 'A'}, {"Ã", 'A'}, {"Ạ", 'A'},
    {"Ă", 'A'}, {"Ằ", 'A'}, {"Ắ", 'A'}, {"Ẳ", 'A'}, {"Ẵ", 'A'}, {"Ặ", 'A'},
    {"Â", 'A'}, {"Ầ", 'A'}, {"Ấ", 'A'}, {"Ẩ", 'A'}, {"Ẫ", 'A'}, {"Ậ", 'A'},

    // Lowercase d, đ
    {"đ", 'd'}, {"Đ", 'D'},

    // Lowercase e
    {"è", 'e'}, {"é", 'e'}, {"ẻ", 'e'}, {"ẽ", 'e'}, {"ẹ", 'e'},
    {"ê", 'e'}, {"ề", 'e'}, {"ế", 'e'}, {"ể", 'e'}, {"ễ", 'e'}, {"ệ", 'e'},
    // Uppercase E
    {"È", 'E'}, {"É", 'E'}, {"Ẻ", 'E'}, {"Ẽ", 'E'}, {"Ẹ", 'E'},
    {"Ê", 'E'}, {"Ề", 'E'}, {"Ế", 'E'}, {"Ể", 'E'}, {"Ễ", 'E'}, {"Ệ", 'E'},

    // Lowercase i
    {"ì", 'i'}, {"í", 'i'}, {"ỉ", 'i'}, {"ĩ", 'i'}, {"ị", 'i'},
    // Uppercase I
    {"Ì", 'I'}, {"Í", 'I'}, {"Ỉ", 'I'}, {"Ĩ", 'I'}, {"Ị", 'I'},

    // Lowercase o
    {"ò", 'o'}, {"ó", 'o'}, {"ỏ", 'o'}, {"õ", 'o'}, {"ọ", 'o'},
    {"ô", 'o'}, {"ồ", 'o'}, {"ố", 'o'}, {"ổ", 'o'}, {"ỗ", 'o'}, {"ộ", 'o'},
    {"ơ", 'o'}, {"ờ", 'o'}, {"ớ", 'o'}, {"ở", 'o'}, {"ỡ", 'o'}, {"ợ", 'o'},
    // Uppercase O
    {"Ò", 'O'}, {"Ó", 'O'}, {"Ỏ", 'O'}, {"Õ", 'O'}, {"Ọ", 'O'},
    {"Ô", 'O'}, {"Ồ", 'O'}, {"Ố", 'O'}, {"Ổ", 'O'}, {"Ỗ", 'O'}, {"Ộ", 'O'},
    {"Ơ", 'O'}, {"Ờ", 'O'}, {"Ớ", 'O'}, {"Ở", 'O'}, {"Ỡ", 'O'}, {"Ợ", 'O'},

    // Lowercase u
    {"ù", 'u'}, {"ú", 'u'}, {"ủ", 'u'}, {"ũ", 'u'}, {"ụ", 'u'},
    {"ư", 'u'}, {"ừ", 'u'}, {"ứ", 'u'}, {"ử", 'u'}, {"ữ", 'u'}, {"ự", 'u'},
    // Uppercase U
    {"Ù", 'U'}, {"Ú", 'U'}, {"Ủ", 'U'}, {"Ũ", 'U'}, {"Ụ", 'U'},
    {"Ư", 'U'}, {"Ừ", 'U'}, {"Ứ", 'U'}, {"Ử", 'U'}, {"Ữ", 'U'}, {"Ự", 'U'},

    // Lowercase y
    {"ỳ", 'y'}, {"ý", 'y'}, {"ỷ", 'y'}, {"ỹ", 'y'}, {"ỵ", 'y'},
    // Uppercase Y
    {"Ỳ", 'Y'}, {"Ý", 'Y'}, {"Ỷ", 'Y'}, {"Ỹ", 'Y'}, {"Ỵ", 'Y'}
};

inline std::string remove_vietnamese_accents(const char* str) {
    if (!str) return "";
    std::string result = "";
    size_t i = 0;
    size_t len = strlen(str);
    static const size_t num_pairs = sizeof(VIETNAMESE_ACCENTS) / sizeof(VIETNAMESE_ACCENTS[0]);

    while (i < len) {
        bool matched = false;
        for (size_t k = 0; k < num_pairs; k++) {
            size_t pat_len = strlen(VIETNAMESE_ACCENTS[k].utf8);
            if (i + pat_len <= len && strncmp(str + i, VIETNAMESE_ACCENTS[k].utf8, pat_len) == 0) {
                result += VIETNAMESE_ACCENTS[k].ascii;
                i += pat_len;
                matched = true;
                break;
            }
        }
        if (!matched) {
            result += str[i];
            i++;
        }
    }
    return result;
}

inline void print_qms_ticket(ThermalPrinter &printer, const char* unitName, const char* serviceName, const char* ticketNum, const char* customTime = nullptr, const char* customDate = nullptr) {
    // Convert Vietnamese accented strings to non-accented ASCII
    std::string cleanUnit = remove_vietnamese_accents(unitName && strlen(unitName) > 0 ? unitName : "HE THONG XEP HANG TU DONG");
    std::string cleanService = remove_vietnamese_accents(serviceName && strlen(serviceName) > 0 ? serviceName : "DICH VU");

    // 1. Reset cài đặt
    printer.resetSettings();
    
    // 2. In Tiêu đề Đơn vị (Header Style)
    printer.useHeaderStyle();
    printer.println(cleanUnit.c_str());
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(30);
    printer.println("------------------------------------------");
    
    // 3. In Tên Dịch vụ
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setSize(2);
    printer.setBold(true);
    printer.println(cleanService.c_str());
    
    // 4. In Số thứ tự (Size lớn nhất)
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setSize(4); // Cỡ rất lớn cho số thứ tự (Khổ 80mm)
    printer.setBold(true);
    printer.setLineSpacing(80);
    printer.println(ticketNum ? ticketNum : "000");
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(30);
    printer.println("------------------------------------------");
    
    // 5. In Thời gian
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::CENTER);
    
    char timePrintBuf[64];
    if (customTime && strlen(customTime) > 0 && customDate && strlen(customDate) > 0) {
        snprintf(timePrintBuf, sizeof(timePrintBuf), "Ngay in: %s %s", customDate, customTime);
    } else if (customTime && strlen(customTime) > 0) {
        snprintf(timePrintBuf, sizeof(timePrintBuf), "Gio in: %s", customTime);
    } else {
        // Fallback: System time
        time_t now;
        struct tm timeinfo;
        time(&now);
        setenv("TZ", "CST-7", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        strftime(timePrintBuf, sizeof(timePrintBuf), "Ngay in: %d/%m/%Y %H:%M:%S", &timeinfo);
    }
    
    printer.println(timePrintBuf);
    printer.println("Vui long doi den luot phuc vu.");
    printer.println("Xin cam on!");
    
    // 6. Cắt giấy
    printer.println("\n\n\n\n\n");
    printer.cut();
}

inline void print_startup_test_ticket(ThermalPrinter &printer, const char* dev_id, const char* ip_str, const char* wifi_ssid, const char* mqtt_host, bool mqtt_connected) {
    (void)mqtt_host;

    printer.resetSettings();
    printer.useHeaderStyle();
    printer.setLineSpacing(60);
    printer.println("THIET BI KHOI DONG");
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(45);
    printer.println("------------------------------------------");
    
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::LEFT);
    printer.setSize(1);
    printer.setLineSpacing(55);
    
    char short_id[16] = {0};
    if (dev_id && strlen(dev_id) >= 12) {
        strncpy(short_id, dev_id, 12);
        short_id[12] = '\0';
    } else if (dev_id && strlen(dev_id) > 0) {
        strncpy(short_id, dev_id, sizeof(short_id) - 1);
    } else {
        strcpy(short_id, "N/A");
    }
    
    char buf[128];
    snprintf(buf, sizeof(buf), "Device ID: %s...", short_id);
    printer.println(buf);
    
    snprintf(buf, sizeof(buf), "WiFi SSID: %s", (wifi_ssid && strlen(wifi_ssid) > 0) ? wifi_ssid : "AP Mode");
    printer.println(buf);
    
    snprintf(buf, sizeof(buf), "IP Addr  : %s", (ip_str && strlen(ip_str) > 0) ? ip_str : "N/A");
    printer.println(buf);
    
    snprintf(buf, sizeof(buf), "MQTT Stat: %s", mqtt_connected ? "CONNECTED" : "READY");
    printer.println(buf);
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(45);
    printer.println("------------------------------------------");
    
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(55);
    printer.println("DA SAN SANG KET NOI & HOAT DONG");
    
    printer.println("\n\n\n\n\n");
    printer.cut();
}

#endif
