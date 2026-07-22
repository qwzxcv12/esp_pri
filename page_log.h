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
        
        .log-header-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 16px;
            background: #0d1117;
            border-bottom: 1px solid var(--line);
            flex-wrap: wrap;
            gap: 8px;
        }
        .log-title {
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 1px;
            color: var(--accent);
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .log-badge {
            background: rgba(255, 180, 84, 0.15);
            color: var(--accent);
            padding: 2px 8px;
            border-radius: 4px;
            font-size: 10.5px;
        }
        .log-tools {
            display: flex;
            align-items: center;
            gap: 6px;
        }
        .log-tools select {
            padding: 5px 8px;
            font-family: var(--mono);
            font-size: 11px;
            color: var(--text);
            background: var(--ink);
            border: 1px solid var(--line);
            border-radius: 4px;
            outline: none;
            cursor: pointer;
        }
        .btn-sm {
            font-family: var(--mono);
            font-size: 10.5px;
            font-weight: 700;
            padding: 5px 10px;
            border-radius: 4px;
            border: 1px solid var(--line);
            background: rgba(255, 255, 255, 0.05);
            color: var(--text);
            cursor: pointer;
            transition: all 0.15s;
        }
        .btn-sm:hover {
            background: rgba(255, 255, 255, 0.12);
            border-color: var(--muted);
        }
        .btn-accent {
            background: var(--accent-dim);
            color: var(--accent);
            border-color: var(--accent);
        }
        .btn-accent:hover {
            background: var(--accent);
            color: var(--ink);
        }
        .log-container {
            height: 380px;
            max-height: 50vh;
            padding: 14px 16px;
            overflow-y: auto;
            background: #070a0e;
            font-family: var(--mono);
            font-size: 12px;
            line-height: 1.6;
            color: #ffb454;
            white-space: pre-wrap;
            border-bottom: 1px solid var(--line);
        }
        .log-container::-webkit-scrollbar {
            width: 6px;
        }
        .log-container::-webkit-scrollbar-track {
            background: #070a0e;
        }
        .log-container::-webkit-scrollbar-thumb {
            background: #232b35;
            border-radius: 3px;
        }
        .log-container::-webkit-scrollbar-thumb:hover {
            background: var(--accent);
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
            <svg class="logo chip-icon" style="flex: none;" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>Device System Logs</h2>
                <p class="subtitle">View system runtime logs.</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/log" class="nav-item active">System Logs</a>
            <a href="/gpio" class="nav-item">GPIO Mapping</a>
            <a href="/ota" class="nav-item">Update</a>
        </div>

        <!-- Log Header Controls -->
        <div class="log-header-bar">
            <div class="log-title">
                <span>📟</span> LOG CONSOLE
                <span id="logCountBadge" class="log-badge">0 dòng</span>
            </div>
            <div class="log-tools">
                <select id="logLimitSelect" onchange="renderLogs()">
                    <option value="30" selected>30 dòng mới nhất</option>
                    <option value="50">50 dòng mới nhất</option>
                    <option value="100">100 dòng (Tất cả)</option>
                </select>
                <button type="button" class="btn-sm" id="pauseBtn" onclick="togglePause()">⏸ Tạm dừng</button>
                <button type="button" class="btn-sm" onclick="clearDisplay()">🧹 Xóa</button>
                <button type="button" class="btn-sm btn-accent" onclick="copyLogs()">📋 Copy Log</button>
            </div>
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

        let rawLogData = "";
        let isPaused = false;

        function togglePause() {
            isPaused = !isPaused;
            const btn = document.getElementById('pauseBtn');
            if (isPaused) {
                btn.textContent = "▶ Tiếp tục";
                btn.style.color = "var(--accent)";
            } else {
                btn.textContent = "⏸ Tạm dừng";
                btn.style.color = "var(--text)";
                fetchLogs();
            }
        }

        function clearDisplay() {
            rawLogData = "";
            const logBox = document.getElementById('logBox');
            if (logBox) logBox.textContent = "Console cleared.";
            const badge = document.getElementById('logCountBadge');
            if (badge) badge.textContent = "0 dòng";
        }

        function renderLogs() {
            const logBox = document.getElementById('logBox');
            if (!logBox) return;

            if (!rawLogData || rawLogData.trim() === "") {
                logBox.textContent = "No logs available.";
                const badge = document.getElementById('logCountBadge');
                if (badge) badge.textContent = "0 dòng";
                return;
            }

            let lines = rawLogData.trim().split('\n');
            const limitSelect = document.getElementById('logLimitSelect');
            const limit = limitSelect ? parseInt(limitSelect.value) : 30;

            const totalLines = lines.length;
            if (lines.length > limit) {
                lines = lines.slice(lines.length - limit);
            }

            const badge = document.getElementById('logCountBadge');
            if (badge) {
                badge.textContent = lines.length < totalLines ? `${lines.length}/${totalLines} dòng` : `${lines.length} dòng`;
            }

            const isAtBottom = logBox.scrollHeight - logBox.scrollTop <= logBox.clientHeight + 60;

            logBox.textContent = lines.join('\n');

            if (isAtBottom) {
                logBox.scrollTop = logBox.scrollHeight;
            }
        }

        function copyLogs() {
            const logBox = document.getElementById('logBox');
            if (!logBox) return;
            const textToCopy = logBox.innerText;
            if (navigator.clipboard && navigator.clipboard.writeText) {
                navigator.clipboard.writeText(textToCopy).then(() => {
                    alert("Đã copy log đang hiển thị!");
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
                    alert("Đã copy log đang hiển thị!");
                } catch (err) {
                    alert("Lỗi khi copy: " + err);
                }
                document.body.removeChild(textArea);
            }
        }

        function fetchLogs() {
            if (isPaused) return;
            fetch('/log_data')
                .then(response => response.text())
                .then(data => {
                    rawLogData = data ? data : "";
                    renderLogs();
                    parseServicesFromLog(rawLogData);
                })
                .catch(err => {
                    const logBox = document.getElementById('logBox');
                    if (logBox) logBox.textContent = "Error loading logs from device: " + err;
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
