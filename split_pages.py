import re

with open('wifi_config_html.h', 'r') as f:
    content = f.read()

# Pattern to extract pages
pattern = re.compile(r'const char\*\s+(\w+)\s*=\s*R"html\((.*?)\)html";', re.DOTALL)
matches = pattern.findall(content)

for name, html_content in matches:
    # name: html_page, log_page, login_page, gpio_page
    if name == 'html_page':
        file_name = 'page_config.h'
        guard = 'PAGE_CONFIG_H'
    else:
        file_name = f"page_{name.replace('_page', '')}.h"
        guard = f"PAGE_{name.replace('_page', '').upper()}_H"
        
    with open(file_name, 'w') as f:
        f.write(f'#ifndef {guard}\n#define {guard}\n\n')
        f.write(f'const char* {name} = R"html({html_content})html";\n')
        f.write(f'\n#endif // {guard}\n')
    print(f"Created {file_name}")

# Generate new wifi_config_html.h
new_h = """#ifndef WIFI_CONFIG_HTML_H
#define WIFI_CONFIG_HTML_H

"""
for name, _ in matches:
    if name == 'html_page':
        file_name = 'page_config.h'
    else:
        file_name = f"page_{name.replace('_page', '')}.h"
    new_h += f'#include "{file_name}"\n'
new_h += "\n#endif // WIFI_CONFIG_HTML_H\n"

with open('wifi_config_html.h', 'w') as f:
    f.write(new_h)
print("Updated wifi_config_html.h")
