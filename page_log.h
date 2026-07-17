#ifndef PAGE_LOG_H
#define PAGE_LOG_H

const char* log_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device System Logs</title>
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
            display: flex;
            flex-direction: column;
            height: 90vh;
        }
        .panel__header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            height: 85px;
            flex: none;
            padding: 0 24px;
            border-bottom: 1px solid var(--line);
        }
        h2 {
            margin: 0;
            font-size: 18px;
            font-weight: 600;
            color: var(--text);
            line-height: 1.2;
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

        .btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 8px 16px;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1px;
            border-radius: 4px;
            cursor: pointer;
            text-decoration: none;
            transition: all 0.15s ease;
        }
        .btn--primary {
            background: var(--accent);
            color: var(--ink);
            border: none;
        }
        .btn--primary:hover { background: #ffc578; }
        
        .log-container {
            flex: 1;
            padding: 24px 16px;
            overflow-y: scroll;
            overflow-y: auto;
            background: #070a0e;
            font-family: var(--mono);
            font-size: 12.5px;
            line-height: 1.6;
            color: #ffb454;
            white-space: pre-wrap;
            border-bottom: 1px solid var(--line);
        }
        
        /* Log Console Control */
        .log-control {
            padding: 16px 20px;
            background: rgba(0, 0, 0, 0.25);
            flex: none;
            border-top: 1px solid var(--line);
            display: flex;
            flex-direction: column;
            gap: 12px;
        }
        .log-control__title {
            font-family: var(--mono);
            font-size: 10px;
            letter-spacing: 1px;
            text-transform: uppercase;
            color: var(--muted);
        }
        .log-control__row {
            display: flex;
            gap: 10px;
            align-items: center;
        }
        .log-control__row input[type="text"] {
            flex: 1;
            padding: 9px 12px;
            font-family: var(--mono);
            font-size: 12px;
            color: var(--text);
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            border-radius: 4px;
            outline: none;
        }
        .log-control__row input[type="text"]:focus {
            border-color: var(--accent);
        }
        .log-control__row select {
            padding: 9px 12px;
            font-family: var(--mono);
            font-size: 12px;
            color: var(--text);
            background: var(--ink);
            border: 1px solid var(--line);
            border-radius: 4px;
            outline: none;
            cursor: pointer;
        }
        .log-control__row select:focus {
            border-color: var(--accent);
        }

        .tab-btn {
            background: transparent;
            border: none;
            color: var(--muted);
            font-family: var(--mono);
            font-size: 10.5px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 1px;
            padding: 6px 12px;
            cursor: pointer;
            border-bottom: 2px solid transparent;
            transition: all 0.15s ease;
            outline: none;
        }
        .tab-btn:hover {
            color: var(--text);
        }
        .tab-btn.active {
            color: var(--accent);
            border-bottom-color: var(--accent);
        }

        .panel__footer {
            padding: 12px 24px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: rgba(0,0,0,0.15);
        }
        .status {
            font-family: var(--mono);
            font-size: 11px;
            color: var(--muted);
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <h2>Device Logs &amp; Diagnostics</h2>
            <div style="display: flex; gap: 8px;">
                <button class="btn btn--primary" onclick="copyLogs()" style="background-color: var(--ok); color: var(--ink);">Copy</button>
                <button class="btn btn--primary" onclick="fetchLogs()">Refresh</button>
            </div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/log" class="nav-item active">System Logs</a>
            <a href="/gpio" class="nav-item">GPIO Mapping</a>
            <a href="/ota" class="nav-item">Update</a>
        </div>

        <div class="log-container" id="logBox">Loading device logs...</div>

        <!-- Ô nhập lệnh gửi trực tiếp -->
        <div class="log-control">
            <div style="display: flex; border-bottom: 1px solid var(--line); margin-bottom: 6px; gap: 8px;">
                <button type="button" class="tab-btn active" id="tabKioskBtn" onclick="switchControlTab('kiosk')">Kiosk Sim</button>
                <span id="kioskStatus" style="margin-left: auto; font-family: var(--mono); font-size: 11px; align-self: center; color: var(--muted);">Ready</span>
            </div>

            <!-- Tab 1: Kiosk Simulator -->
            <div id="kioskControlPanel" style="display: flex; flex-direction: column; gap: 10px;">
                <div class="log-control__row">
                    <span id="kioskUnitName" style="font-weight: bold; color: var(--accent); padding-bottom: 5px; width: 100%; border-bottom: 1px solid var(--line);">Đơn vị: Đang chờ đồng bộ...</span>
                </div>
                <div class="log-control__row">
                    <button class="btn btn--primary" onclick="syncServices()" style="padding: 10px 14px; background-color: var(--accent); color: var(--ink);">1. Sync Services</button>
                    <select id="kioskServiceSelect" style="flex: 1;">
                        <option value="">-- Please click Sync Services --</option>
                    </select>
                </div>
                <div class="log-control__row">
                    <input type="text" id="kioskCustName" placeholder="Customer Name (Default: Guest)" style="flex: 1;">
                    <button class="btn btn--primary" onclick="getTicket()" style="padding: 10px 20px; background-color: var(--ok); color: var(--ink);">2. Get Ticket</button>
                </div>
            </div>

            </div>
        </div>

    </div>

    <script>
        const devId = "{{DEV_ID}}";
        const devKey = "{{DEV_KEY}}";
        let servicesList = [];



        function switchControlTab(tab) {
            // Only kiosk tab exists now
        }

        function syncServices() {
            if (!devId || !devKey) {
                alert("Error: Device ID or Device KEY not configured!");
                return;
            }
            const topic = `qms/sender/${devId}/request`;
            const payload = JSON.stringify({
                cmd: "get_config",
                secret_key: devKey
            });
            sendMqtt(topic, payload, "Syncing services...");
        }

        function getTicket() {
            const select = document.getElementById('kioskServiceSelect');
            const serviceId = select.value;
            if (!serviceId) {
                alert("Please select or sync services first!");
                return;
            }
            
            const custName = document.getElementById('kioskCustName').value.trim() || "Guest";
            const topic = `qms/sender/${devId}/ticket_request`;
            const payload = JSON.stringify({
                service_id: parseInt(serviceId),
                secret_key: devKey,
                customer_name: custName
            });
            sendMqtt(topic, payload, `Requesting ticket for service ID ${serviceId}...`);
        }

        function sendMqtt(topic, payload, pendingMsg) {
            const statusBox = document.getElementById('kioskStatus');
            statusBox.textContent = pendingMsg;
            statusBox.style.color = 'var(--accent)';
            
            const params = `topic=${encodeURIComponent(topic)}&payload=${encodeURIComponent(payload)}`;
            fetch('/publish', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: params
            })
            .then(res => {
                if (res.ok) {
                    statusBox.textContent = "Sent successfully!";
                    statusBox.style.color = 'var(--ok)';
                    setTimeout(fetchLogs, 500);
                } else {
                    res.text().then(err => {
                        statusBox.textContent = "Send error: " + err;
                        statusBox.style.color = '#ff6b6b';
                    });
                }
            })
            .catch(err => {
                statusBox.textContent = "Connection error: " + err;
                statusBox.style.color = '#ff6b6b';
            });
        }

        function parseServicesFromLog(logData) {
            const regexUnit = /Unit:\s*(.*)/g;
            let matchUnit;
            let unitName = "";
            while ((matchUnit = regexUnit.exec(logData)) !== null) {
                unitName = matchUnit[1];
            }
            if (unitName) {
                const el = document.getElementById('kioskUnitName');
                if (el) el.textContent = "Đơn vị: " + unitName;
            }

            const regex = /Nhấn phím \d+:\s*(.*?)\s*\(Service ID:\s*(\d+)\)/g;
            let match;
            let serviceMap = new Map();
            
            while ((match = regex.exec(logData)) !== null) {
                serviceMap.set(parseInt(match[2]), match[1]);
            }
            
            let services = [];
            serviceMap.forEach((name, id) => {
                services.push({ name: name, id: id });
            });
            
            if (services.length > 0) {
                updateServiceDropdown(services);
            }
        }
        
        function updateServiceDropdown(services) {
            const select = document.getElementById('kioskServiceSelect');
            const newKeys = services.map(s => `${s.id}:${s.name}`).join('|');
            const oldKeys = servicesList.map(s => `${s.id}:${s.name}`).join('|');
            if (newKeys === oldKeys) return;
            
            servicesList = services;
            const currentValue = select.value;
            
            select.innerHTML = '<option value="">-- Select Service --</option>';
            services.forEach(s => {
                const opt = document.createElement('option');
                opt.value = s.id;
                opt.textContent = `${s.name} (ID: ${s.id})`;
                select.appendChild(opt);
            });
            
            if (currentValue) {
                select.value = currentValue;
            }
            
            const statusBox = document.getElementById('kioskStatus');
            statusBox.textContent = `Synced ${services.length} services!`;
            statusBox.style.color = 'var(--ok)';
        }

        function copyLogs() {
            const logBox = document.getElementById('logBox');
            if (!logBox) return;
            const textToCopy = logBox.innerText;
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(textToCopy).then(() => {
                    alert("Đã copy toàn bộ log!");
                }).catch(err => {
                    alert("Lỗi khi copy: " + err);
                });
            } else {
                const textArea = document.createElement("textarea");
                textArea.value = textToCopy;
                document.body.appendChild(textArea);
                textArea.select();
                try {
                    document.execCommand('copy');
                    alert("Đã copy toàn bộ log!");
                } catch (err) {
                    alert("Lỗi khi copy: " + err);
                }
                document.body.removeChild(textArea);
            }
        }


        function fetchLogs() {
            const logBox = document.getElementById('logBox');
            fetch('/log_data')
                .then(response => response.text())
                .then(data => {
                    logBox.textContent = data ? data : "No logs available.";
                    logBox.scrollTop = logBox.scrollHeight;
                    parseServicesFromLog(data);
                })
                .catch(err => {
                    logBox.textContent = "Error loading logs from device: " + err;
                });
        }
        

        
        fetchLogs();
        setInterval(fetchLogs, 5000);

        // 15-minute Inactivity Auto Logout
        (function() {
            let timeout;
            const idleTime = 15 * 60 * 1000;
            function logout() {
                document.cookie = "passwd=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;";
                window.location.href = "/login";
            }
            function resetTimer() {
                clearTimeout(timeout);
                timeout = setTimeout(logout, idleTime);
            }
            window.onload = resetTimer;
            document.onmousemove = resetTimer;
            document.onkeydown = resetTimer;
            document.onclick = resetTimer;
            document.onscroll = resetTimer;
        })();
    </script>
</body>
</html>
)html";

#endif // PAGE_LOG_H
