import re

files = ['page_config.h', 'page_log.h', 'page_login.h', 'page_gpio.h', 'page_ota.h']

media_query = """
        /* Mobile Responsive Adjustments */
        @media (max-width: 480px) {
            body { padding: 0; }
            .panel {
                max-width: 100%;
                border-radius: 0;
                border-left: none;
                border-right: none;
                box-shadow: none;
                min-height: 100vh;
            }
            .panel__header {
                padding: 0 16px;
                height: 75px;
            }
            .chip-icon { display: none; }
            .panel__nav {
                overflow-x: auto;
                -webkit-overflow-scrolling: touch;
            }
            .panel__nav::-webkit-scrollbar { display: none; }
            .nav-item {
                padding: 12px 16px;
                font-size: 10px;
                white-space: nowrap;
                flex: none;
            }
            form, .log-container, .container {
                padding: 16px !important;
            }
            .field label { font-size: 10px; }
            input[type="text"], input[type="password"], input[type="number"], .dropdown-multi__selected, select {
                font-size: 14px !important; /* Prevent iOS zoom */
                padding: 12px !important;
            }
            .submit, .btn {
                padding: 14px !important;
                font-size: 14px !important;
            }
        }
"""

for file_name in files:
    with open(file_name, 'r') as f:
        content = f.read()

    if "/* Mobile Responsive Adjustments */" not in content:
        # We find `.nav-item.active { ... }` and append the media query after it
        content = re.sub(
            r'(\.nav-item\.active\s*\{[^}]*\})',
            r'\1\n' + media_query,
            content
        )
        
        with open(file_name, 'w') as f:
            f.write(content)
        print(f"Added responsive styles to {file_name}")
    else:
        print(f"Already responsive: {file_name}")

