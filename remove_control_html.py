import re

with open('wifi_config_html.h', 'r') as f:
    content = f.read()

# Remove navigation links
content = re.sub(r' *<a href="/control"[^>]*>.*?</a>\n', '', content)

# Remove the control_page string block
content = re.sub(r'const char\* control_page = R"html\(.*?\)html";\n+', '', content, flags=re.DOTALL)

with open('wifi_config_html.h', 'w') as f:
    f.write(content)

print("Done")
