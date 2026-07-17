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
            --bg: #0f172a;
            --panel: #1e293b;
            --text: #f8fafc;
            --muted: #94a3b8;
            --accent: #3b82f6;
            --success: #10b981;
            --danger: #ef4444;
            --line: #334155;
            --input: #0f172a;
            --mono: 'JetBrains Mono', 'Fira Code', monospace;
        }
        * { box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 24px 16px;
        }
        .panel {
            width: 540px;
            max-width: 95%;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: 2px solid var(--accent);
            border-radius: 8px;
            box-shadow: 0 20px 50px rgba(0,0,0,0.45);
            overflow: hidden;
            display: flex;
            flex-direction: column;
        }
        .panel__header {
            display: flex;
            align-items: center;
            height: 85px;
            flex: none;
            padding: 0 24px;
            border-bottom: 1px solid var(--line);
        }
        h2 { margin: 0; font-size: 18px; font-weight: 600; color: var(--text); }
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
            color: var(--muted);
            text-decoration: none;
            border-bottom: 2px solid transparent;
        }
        .nav-item.active {
            color: var(--accent);
            border-bottom-color: var(--accent);
            background: rgba(255, 255, 255, 0.04);
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
            <h2>PRI_QMS System</h2>
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
