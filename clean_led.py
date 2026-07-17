import re

def clean_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
        
    # Remove #include "led_display.h"
    content = re.sub(r'#include\s+"led_display\.h"\s*\n', '', content)
    
    # Remove setup_led_display();
    content = re.sub(r'\s*setup_led_display\(\);\s*', '\n', content)
    
    # Remove show_startup_messages = false;
    content = re.sub(r'\s*show_startup_messages\s*=\s*false;\s*', '\n', content)
    
    # Remove extern void processMessage(const char* command);
    content = re.sub(r'\s*extern\s+void\s+processMessage\(const\s+char\*\s+command\);\s*', '\n', content)
    
    # Remove blocks of:
    # if (show_startup_messages) {
    #     processMessage(...);
    # }
    content = re.sub(r'[ \t]*if\s*\(show_startup_messages\)\s*\{\s*processMessage\([^)]+\);\s*\}[ \t]*\n?', '', content)
    
    # Remove any remaining if (show_startup_messages) { processMessage(buf); } which might have different formatting
    content = re.sub(r'[ \t]*if\s*\(show_startup_messages\)\s*\{[^}]*processMessage[^}]*\}[ \t]*\n?', '', content)
    
    # Remove single line processMessage(...);
    content = re.sub(r'[ \t]*processMessage\([^)]+\);\n?', '', content)
    
    # Remove char disp_msg[128]; and sprintf(disp_msg, ...); in mqtt_handler.h
    content = re.sub(r'[ \t]*char\s+disp_msg\[128\];\n?', '', content)
    content = re.sub(r'[ \t]*sprintf\(disp_msg,\s*"[^"]+",\s*[^)]+\);\n?', '', content)
    content = re.sub(r'[ \t]*sprintf\(disp_msg,\s*"[^"]+"\);\n?', '', content)

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)

clean_file('pri_NOW.cpp')
clean_file('mqtt_handler.h')
print("Cleaned LED code from pri_NOW.cpp and mqtt_handler.h")
