import re

files = ['page_config.h', 'page_log.h', 'page_login.h', 'page_gpio.h', 'page_ota.h']

base_header = """<body>
    <div class="panel">
        <div class="panel__header">
            <svg class="chip-icon" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
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
        </div>

        <div class="panel__nav">"""

for file_name in files:
    with open(file_name, 'r') as f:
        content = f.read()

    # Determine Title and Subtitle based on file
    if file_name == 'page_config.h':
        title = "Device Configuration"
        subtitle = "Set the network and broker parameters."
    elif file_name == 'page_log.h':
        title = "Device System Logs"
        subtitle = "View system runtime logs."
    elif file_name == 'page_login.h':
        title = "Authentication"
        subtitle = "Authenticate to access the device."
    elif file_name == 'page_gpio.h':
        title = "GPIO Mapping"
        subtitle = "Map hardware pins to logic."
    elif file_name == 'page_ota.h':
        title = "Firmware Update"
        subtitle = "Update device firmware via OTA."

    new_header = base_header.replace('{TITLE}', title).replace('{SUBTITLE}', subtitle)

    # Use regex to find everything from <body> to <div class="panel__nav">
    # Note: re.DOTALL is important.
    pattern = re.compile(r'<body>\s*<div class="panel">.*?<div class="panel__nav">', re.DOTALL)
    
    if pattern.search(content):
        content = pattern.sub(new_header, content)
        with open(file_name, 'w') as f:
            f.write(content)
        print(f"Fixed {file_name}")
    else:
        print(f"Could not match pattern in {file_name}")

