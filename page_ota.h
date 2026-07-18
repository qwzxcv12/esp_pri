#ifndef PAGE_OTA_H
#define PAGE_OTA_H

const char* ota_page = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Firmware Update - PRI_QMS</title>
    <style>
        :root {
            --ink: #0b0f14;
            --panel: #11161d;
            --line: #232b35;
            --text: #e6edf3;
            --muted: #6b7785;
            --accent: #ffb454;
            --accent-dim: rgba(255, 180, 84, 0.16);
            --ok: #5ec98f;
            --mono: ui-monospace, 'SF Mono', 'Cascadia Code', 'Consolas', 'Courier New', monospace;
            --sans: -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        }
        * { box-sizing: border-box; }
        body {
            font-family: var(--sans);
            background-color: var(--ink);
            background-image:
                linear-gradient(90deg, rgba(255, 180, 84, 0.04) 1px, transparent 1px),
                linear-gradient(rgba(255, 180, 84, 0.04) 1px, transparent 1px),
                radial-gradient(circle, rgba(255, 180, 84, 0.12) 1.4px, transparent 1.4px);
            background-size: 56px 56px, 56px 56px, 56px 56px;
            background-position: 0 0, 0 0, 28px 28px;
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 24px 16px;
            overflow-y: scroll;
        }
        .panel {
            width: 540px;
            max-width: 95%;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: 2px solid var(--accent);
            border-radius: 8px;
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
            overflow: hidden;
        }
        .panel__header {
            display: flex;
            align-items: center;
            gap: 12px;
            height: 85px;
            flex: none;
            padding: 0 24px;
            border-bottom: 1px solid var(--line);
        }
        .logo {
            width: 40px;
            height: 40px;
            border-radius: 10px;
            background: linear-gradient(135deg, var(--accent), #ff8f00);
            padding: 8px;
            color: var(--ink);
            box-shadow: 0 4px 12px var(--accent-dim);
        }
        .title-wrap {
            display: flex;
            flex-direction: column;
            justify-content: center;
        }
        h2 {
            margin: 0 0 4px;
            font-size: 18px;
            font-weight: 600;
            color: var(--text);
            line-height: 1.2;
        }
        .panel__heading {
            flex: 1;
            min-width: 0;
        }
        .device-id {
            font-family: var(--mono);
            font-size: 11px;
            letter-spacing: 1.5px;
            color: var(--muted);
            text-transform: uppercase;
        }
        h2 {
            margin: 2px 0 6px;
            font-size: 19px;
            font-weight: 600;
            color: var(--text);
        }
        .subtitle {
            font-size: 12.5px;
            line-height: 1.5;
            color: var(--muted);
            margin: 0;
        }
        .status {
            flex: none;
            display: flex;
            align-items: center;
            gap: 6px;
            font-family: var(--mono);
            font-size: 10.5px;
            letter-spacing: 1px;
            color: var(--ok);
            white-space: nowrap;
            padding-top: 3px;
        }
        .status-dot {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background: var(--ok);
            box-shadow: 0 0 6px rgba(94, 201, 143, 0.8);
            animation: pulse 2s ease-in-out infinite;
        }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.35; }
        }
        
        /* Navigation Bar */
        .panel__nav {
            display: flex;
            border-bottom: 1px solid var(--line);
            background: rgba(0, 0, 0, 0.25);
            flex: none;
        }
        .nav-item {
            flex: 1;
            text-align: center;
            padding: 14px;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            color: var(--muted);
            text-decoration: none;
            transition: all 0.15s ease;
            border-bottom: 2px solid transparent;
        }
        .nav-item:hover {
            color: var(--text);
            background: rgba(255, 255, 255, 0.02);
        }
        .nav-item.active {
            color: var(--accent);
            border-bottom-color: var(--accent);
            background: rgba(255, 255, 255, 0.04);
        }

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


        
        .container { padding: 32px 24px; text-align: center; }
        .btn {
            background: var(--accent);
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 6px;
            font-family: var(--mono);
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
            margin-top: 20px;
        }
        .btn:disabled { background: var(--muted); cursor: not-allowed; }
        #status {
            margin-top: 16px;
            font-family: var(--mono);
            font-size: 13px;
            color: var(--accent);
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <svg class="logo chip-icon" style="flex: none;" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>Firmware Update</h2>
                <p class="subtitle">Update device firmware via OTA.</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/log" class="nav-item">System Logs</a>
            <a href="/gpio" class="nav-item">GPIO Mapping</a>
            <a href="/ota" class="nav-item active">Update</a>
        </div>
        <div class="container">
            <h3 style="margin-top:0">Firmware Update</h3>
            <p id="info" style="color:var(--muted); font-size:14px; line-height:1.5;">Checking for updates...</p>
            <button id="updateBtn" class="btn" onclick="startUpdate()" disabled>Install Update</button>
            <div id="status"></div>
        </div>
    </div>
    <script>
        async function checkUpdate() {
            try {
                let res = await fetch('/api/ota_check');
                let data = await res.json();
                if (data.url && data.url.length > 5) {
                    document.getElementById('info').innerHTML = "Update available!<br><br>URL: " + data.url;
                    document.getElementById('updateBtn').disabled = false;
                } else {
                    document.getElementById('info').innerHTML = "No pending updates from MQTT.<br>Send 'update_firmware' command to this device via MQTT first.";
                }
            } catch (e) {
                document.getElementById('info').innerHTML = "Failed to check update status.";
            }
        }
        async function startUpdate() {
            document.getElementById('updateBtn').disabled = true;
            document.getElementById('status').innerText = "Starting update... Please wait, device will reboot automatically.";
            try {
                let res = await fetch('/api/ota_start', { method: 'POST' });
                if (res.ok) {
                    document.getElementById('status').innerText = "Downloading and flashing... DO NOT turn off power!";
                } else {
                    document.getElementById('status').innerText = "Failed to start update.";
                    document.getElementById('updateBtn').disabled = false;
                }
            } catch (e) {
                document.getElementById('status').innerText = "Network error while starting update.";
                document.getElementById('updateBtn').disabled = false;
            }
        }
        checkUpdate();
    </script>
</body>
</html>
)=====";

#endif // PAGE_OTA_H
