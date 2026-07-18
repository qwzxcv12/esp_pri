#ifndef RECEIPT_TEMPLATE_H
#define RECEIPT_TEMPLATE_H

#include "thermal_printer.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

inline void print_qms_ticket(ThermalPrinter &printer, const char* unitName, const char* serviceName, const char* ticketNum, const char* customerName = nullptr) {
    // 1. Reset cài đặt
    printer.resetSettings();
    
    // 2. In Tiêu đề Đơn vị (Header Style)
    printer.useHeaderStyle();
    printer.println(unitName ? unitName : "HE THONG XEP HANG TU DONG");
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(30);
    printer.println("------------------------------------------");
    
    // 3. In Tên Dịch vụ
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setSize(2);
    printer.setBold(true);
    printer.println(serviceName ? serviceName : "DICH VU");
    
    // 4. In Số thứ tự (Size lớn nhất)
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setSize(4); // Cỡ rất lớn cho số thứ tự (Khổ 80mm)
    printer.setBold(true);
    printer.setLineSpacing(80);
    printer.println(ticketNum ? ticketNum : "000");
    
    // 5. In Tên khách hàng (nếu có)
    if (customerName && strlen(customerName) > 0) {
        printer.resetSettings();
        printer.setAlignment(ThermalPrinter::CENTER);
        printer.setSize(2);
        printer.println(customerName);
    }
    
    printer.resetSettings();
    printer.setAlignment(ThermalPrinter::CENTER);
    printer.setLineSpacing(30);
    printer.println("------------------------------------------");
    
    // 6. In Thời gian
    printer.useBodyStyle();
    printer.setAlignment(ThermalPrinter::CENTER);
    
    // Lấy giờ hệ thống
    time_t now;
    struct tm timeinfo;
    time(&now);
    // Chỉnh timezone về GMT+7
    setenv("TZ", "CST-7", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "Ngay in: %d/%m/%Y %H:%M", &timeinfo);
    
    printer.println(timeStr);
    printer.println("Vui long doi den luot phuc vu.");
    printer.println("Xin cam on!");
    
    // 7. Cắt giấy
    printer.println("\n\n\n\n\n");
    printer.cut();
}

#endif
