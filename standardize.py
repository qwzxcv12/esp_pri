import re

files = ['page_config.h', 'page_log.h', 'page_login.h', 'page_gpio.h', 'page_ota.h']

# 1. Read base style and header from page_config.h
with open('page_config.h', 'r') as f:
    config_content = f.read()

# Extract common CSS
style_match = re.search(r'(<style>.*?)(form \{|/\* Log Console Control)', config_content, re.DOTALL)
if style_match:
    common_css = style_match.group(1)
else:
    # try another split
    common_css = re.search(r'(<style>.*?)\s+form \{', config_content, re.DOTALL).group(1)

# Base SVG header
base_header = """        <div class="panel__header">
            <svg class="chip-icon" style="width: 40px; height: 40px; border-radius: 10px; background: linear-gradient(135deg, var(--accent), #ff8f00); padding: 8px; color: var(--ink); box-shadow: 0 4px 12px var(--accent-dim); flex: none;" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>{TITLE}</h2>
                <p class="subtitle">{SUBTITLE}</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
        </div>"""

for file_name in files:
    with open(file_name, 'r') as f:
        content = f.read()

    # Extract old title to use in new header
    old_title_match = re.search(r'<div class="panel__header">.*?<h2>(.*?)</h2>', content, re.DOTALL)
    old_title = old_title_match.group(1).strip() if old_title_match else "Settings"
    if file_name == 'page_config.h':
        subtitle = "Set the network and broker parameters."
    elif file_name == 'page_log.h':
        subtitle = "View system runtime logs."
        old_title = "Device System Logs"
    elif file_name == 'page_login.h':
        subtitle = "Authenticate to access the device."
        old_title = "Authentication"
    elif file_name == 'page_gpio.h':
        subtitle = "Map hardware pins to logic."
        old_title = "GPIO Mapping"
    elif file_name == 'page_ota.h':
        subtitle = "Update device firmware via OTA."
        old_title = "Firmware Update"

    new_header = base_header.replace('{TITLE}', old_title).replace('{SUBTITLE}', subtitle)

    # Replace header
    content = re.sub(r'<div class="panel__header">.*?</div>', new_header, content, flags=re.DOTALL, count=1)

    # For CSS, it's tricky because each file has specific CSS. 
    # We replace from <style> to <style end or specific class>
    # In each file, we find <style> and we find the first specific class or end of base classes.
    # Actually, a simpler way is to find <style> to .nav-item.active { ... } and replace it with common_css + .nav-item.active { ... }
    
    # We will just replace everything from <style> down to .nav-item.active { ... }
    # Wait, the common_css ALREADY includes .nav-item.active.
    # So we replace from <style> to .nav-item.active \{.*?\}
    css_regex = re.compile(r'<style>.*?\.nav-item\.active\s*\{.*?\}', re.DOTALL)
    if css_regex.search(content):
        # common_css ends right before `form {` or similar. Let's make sure common_css includes .nav-item.active
        content = css_regex.sub(common_css, content)
    else:
        # Fallback if .nav-item.active not found
        print(f"CSS replacement fallback for {file_name}")
    
    # Write back
    with open(file_name, 'w') as f:
        f.write(content)
    print(f"Standardized {file_name}")

