import re

with open('wifi_config_html.h', 'r') as f:
    content = f.read()

form_regex = re.compile(r'<form action="/config" method="POST">\n(.*?)<button type="submit" class="submit">Save &amp; Reboot</button>\n.*?<div class="footnote">.*?</div>\n\s*</form>', re.DOTALL)
match = form_regex.search(content)

if match:
    form_content = match.group(1)
    
    # Split by '<div class="section">'
    parts = form_content.split('<div class="section">')
    parts = [p.strip() for p in parts if p.strip()]
    
    new_html = ""
    section_names = ["NETWORK", "MQTT", "WS", "DEV"]
    
    for i, p in enumerate(parts):
        s_name = section_names[i]
        # Re-attach the '<div class="section">' that was removed by split
        section = '<div class="section">\n' + p
        
        new_html += f'''
        <form action="/config" method="POST" style="margin-bottom: 24px; background: rgba(0,0,0,0.1); padding: 16px; border-radius: 8px; border: 1px solid var(--line);">
            {section}
            <input type="hidden" name="config_section" value="{s_name}">
            <button type="submit" class="submit" style="margin-top: 16px;">Save {s_name}</button>
        </form>
'''
    
    new_html += '''
        <div class="footnote" style="text-align: center; font-size: 11px; color: var(--muted); margin-top: 20px;">
            Settings are written to flash and applied on reboot.<br><strong>Antigravity ESP-IDF Manager</strong>
        </div>
'''
    
    content = content[:match.start()] + new_html + content[match.end():]
    
    with open('wifi_config_html.h', 'w') as f:
        f.write(content)
    print("Forms split successfully.")
else:
    print("Could not find form block.")
