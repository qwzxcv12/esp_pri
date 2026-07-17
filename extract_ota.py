import re

with open('wifi_config_html_old.h', 'r') as f:
    content = f.read()

pattern = re.compile(r'const char\*\s+ota_page\s*=\s*R"=====\((.*?)\)=====";', re.DOTALL)
match = pattern.search(content)

if match:
    html_content = match.group(1)
    with open('page_ota.h', 'w') as f:
        f.write('#ifndef PAGE_OTA_H\n#define PAGE_OTA_H\n\n')
        f.write(f'const char* ota_page = R"=====({html_content})=====";\n')
        f.write('\n#endif // PAGE_OTA_H\n')
    print("Created page_ota.h")
else:
    print("Could not find ota_page")

# Add include to wifi_config_html.h
with open('wifi_config_html.h', 'r') as f:
    main_h = f.read()

if '#include "page_ota.h"' not in main_h:
    main_h = main_h.replace('#endif // WIFI_CONFIG_HTML_H', '#include "page_ota.h"\n\n#endif // WIFI_CONFIG_HTML_H')
    with open('wifi_config_html.h', 'w') as f:
        f.write(main_h)
    print("Updated wifi_config_html.h")

