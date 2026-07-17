#ifndef WIFI_CONFIG_HTML_H
#define WIFI_CONFIG_HTML_H

const char* html_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Device Setup</title>
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
            padding: 24px 0;
        }
        .panel {
            width: 440px;
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
            align-items: flex-start;
            gap: 12px;
            padding: 22px 24px 18px;
            border-bottom: 1px solid var(--line);
        }
        .chip-icon {
            flex: none;
            width: 30px;
            height: 30px;
            color: var(--accent);
            margin-top: 2px;
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

        form {
            padding: 4px 24px 24px;
        }
        .section {
            padding-top: 20px;
        }
        .section__title {
            display: flex;
            align-items: baseline;
            gap: 8px;
            margin-bottom: 14px;
        }
        .section__index {
            font-family: var(--mono);
            font-size: 11px;
            color: var(--accent);
        }
        .section__label {
            flex: none;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 1.5px;
            text-transform: uppercase;
            color: var(--text);
        }
        .section__title::after {
            content: '';
            flex: 1;
            height: 1px;
            background: var(--line);
        }
        .field {
            margin-bottom: 12px;
        }
        .field label {
            display: block;
            margin-bottom: 5px;
            font-family: var(--mono);
            font-size: 11px;
            letter-spacing: 0.5px;
            color: var(--muted);
        }
        input[type="text"],
        input[type="password"],
        input[type="number"] {
            width: 100%;
            padding: 10px 12px;
            font-family: var(--mono);
            font-size: 13.5px;
            color: var(--text);
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            border-radius: 5px;
            outline: none;
            transition: border-color 0.15s ease, box-shadow 0.15s ease;
        }
        input::placeholder {
            color: #45505c;
        }
        input:focus {
            border-color: var(--accent);
            box-shadow: 0 0 0 3px var(--accent-dim);
        }

        /* Dropdown Multi-select styles */
        .dropdown-multi {
            position: relative;
            width: 100%;
        }
        .dropdown-multi__selected {
            width: 100%;
            padding: 10px 12px;
            font-family: var(--mono);
            font-size: 13px;
            color: var(--text);
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            border-radius: 5px;
            cursor: pointer;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .dropdown-multi__selected:focus-within {
            border-color: var(--accent);
        }
        .arrow-icon {
            width: 16px;
            height: 16px;
            transition: transform 0.2s;
            color: var(--muted);
        }
        .dropdown-multi.open .arrow-icon {
            transform: rotate(180deg);
        }
        .dropdown-multi__options {
            display: none;
            position: absolute;
            top: 100%;
            left: 0;
            right: 0;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: none;
            border-radius: 0 0 5px 5px;
            z-index: 10;
            box-shadow: 0 10px 25px rgba(0,0,0,0.5);
            padding: 8px;
        }
        .dropdown-multi.open .dropdown-multi__options {
            display: block;
        }
        .dropdown-multi__list {
            max-height: 150px;
            overflow-y: auto;
            margin-bottom: 8px;
        }
        .dropdown-option {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 6px 8px;
            cursor: pointer;
            font-family: var(--mono);
            font-size: 12px;
            border-radius: 4px;
            transition: background 0.1s;
        }
        .dropdown-option:hover {
            background: rgba(255, 255, 255, 0.05);
        }
        .dropdown-option input[type="checkbox"] {
            cursor: pointer;
            accent-color: var(--accent);
        }
        .dropdown-multi__add {
            display: flex;
            gap: 6px;
            border-top: 1px solid var(--line);
            padding-top: 8px;
        }
        .dropdown-multi__add input {
            flex: 1;
            padding: 6px 8px;
            font-family: var(--mono);
            font-size: 11px;
            background: rgba(0, 0, 0, 0.5);
            border: 1px solid var(--line);
            border-radius: 4px;
            color: var(--text);
            outline: none;
        }
        .dropdown-multi__add button {
            padding: 6px 12px;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: bold;
            background: var(--accent);
            color: var(--ink);
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        .dropdown-multi__add button:hover {
            background: #ffc578;
        }

        .submit {
            width: 100%;
            margin-top: 22px;
            padding: 13px;
            font-family: var(--mono);
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 1.5px;
            text-transform: uppercase;
            color: var(--ink);
            background: var(--accent);
            border: none;
            border-radius: 6px;
            cursor: pointer;
            transition: background 0.15s ease, transform 0.05s ease;
        }
        .submit:hover { background: #ffc578; }
        .submit:active { transform: translateY(1px); }
        .footnote {
            margin-top: 14px;
            text-align: center;
            font-family: var(--mono);
            font-size: 10.5px;
            color: var(--muted);
            line-height: 1.6;
        }
        .footnote strong {
            color: #8a96a3;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <svg class="chip-icon" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>Device Configuration</h2>
                <p class="subtitle">Set the network and broker parameters.</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item active">Configuration</a>
            <a href="/log" class="nav-item">System Logs</a>
        </div>

        <form action="/config" method="POST">
            <div class="section">
                <div class="section__title">
                    <span class="section__index">01</span>
                    <span class="section__label">Network</span>
                </div>
                <div class="field">
                    <label for="ssid">Wi-Fi network name (SSID)</label>
                    <input type="text" id="ssid" name="ssid" required placeholder="Enter Wi-Fi SSID" value="{{SSID}}">
                </div>
                <div class="field">
                    <label for="password">Wi-Fi password</label>
                    <input type="password" id="password" name="password" placeholder="Enter Wi-Fi password" value="{{PASSWORD}}">
                </div>
            </div>

            <div class="section">
                <div class="section__title">
                    <span class="section__index">02</span>
                    <span class="section__label">MQTT Broker</span>
                </div>
                <div class="field">
                    <label for="mqtt_server">Broker host</label>
                    <input type="text" id="mqtt_server" name="mqtt_server" placeholder="qms1.camdvr.org" value="{{MQTT_SERVER}}">
                </div>
                <div class="field">
                    <label for="mqtt_port">Broker port</label>
                    <input type="number" id="mqtt_port" name="mqtt_port" placeholder="1993" value="{{MQTT_PORT}}">
                </div>
                <div class="field">
                    <label for="mqtt_user">Broker username</label>
                    <input type="text" id="mqtt_user" name="mqtt_user" placeholder="thom" value="{{MQTT_USER}}">
                </div>
                <div class="field">
                    <label for="mqtt_pass">Broker password</label>
                    <input type="password" id="mqtt_pass" name="mqtt_pass" placeholder="301258" value="{{MQTT_PASS}}">
                </div>
                
                <!-- Dropdown Đa Chọn Topic -->
                <div class="field">
                    <label>Topics (Select one or more)</label>
                    <div class="dropdown-multi">
                        <div class="dropdown-multi__selected" onclick="toggleDropdown(event)">
                            <span id="selectedTopicsText">Select Topics...</span>
                            <svg class="arrow-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg>
                        </div>
                        <div class="dropdown-multi__options" id="dropdownOptions">
                            <div class="dropdown-multi__list" id="topicsList">
                                <!-- Option Rendered via JS -->
                            </div>
                            <div class="dropdown-multi__add" onclick="event.stopPropagation()">
                                <input type="text" id="newTopicInput" placeholder="Add custom topic...">
                                <button type="button" onclick="addCustomTopic(event)">Add</button>
                            </div>
                        </div>
                    </div>
                    <!-- Hidden input gửi lên Server -->
                    <input type="hidden" id="mqtt_topic" name="mqtt_topic" value="{{MQTT_TOPIC}}">
                </div>
            </div>

            <div class="section">
                <div class="section__title">
                    <span class="section__index">03</span>
                    <span class="section__label">WebSocket</span>
                </div>
                <div class="field">
                    <label for="ws_url">Socket URL</label>
                    <input type="text" id="ws_url" name="ws_url" placeholder="e.g. ws://192.168.1.100:81" value="{{WS_URL}}">
                </div>
            </div>

            <div class="section">
                <div class="section__title">
                    <span class="section__index">04</span>
                    <span class="section__label">Device Credentials</span>
                </div>
                <div class="field">
                    <label for="dev_id">Device ID</label>
                    <input type="text" id="dev_id" name="dev_id" placeholder="Device ID" value="{{DEV_ID}}">
                </div>
                <div class="field">
                    <label for="dev_key">Device KEY</label>
                    <input type="text" id="dev_key" name="dev_key" placeholder="Device KEY" value="{{DEV_KEY}}">
                </div>
            </div>

            <button type="submit" class="submit">Save &amp; Reboot</button>
            <div class="footnote">Settings are written to flash and applied on reboot.<br><strong>Antigravity ESP-IDF Manager</strong></div>
        </form>
    </div>

    <script>
        let selectedTopics = [];
        const defaultTopics = ["qms/sender", "qms/display"];

        // Đọc giá trị từ hidden input backend
        const initialVal = document.getElementById('mqtt_topic').value;
        if (initialVal) {
            selectedTopics = initialVal.split(',').map(t => t.trim()).filter(t => t.length > 0);
        }

        // Tạo tập các topic hiển thị
        let allTopics = [...new Set([...defaultTopics, ...selectedTopics])];

        function renderTopics() {
            const list = document.getElementById('topicsList');
            list.innerHTML = '';
            allTopics.forEach(topic => {
                const checked = selectedTopics.includes(topic) ? 'checked' : '';
                const option = document.createElement('div');
                option.className = 'dropdown-option';
                option.onclick = (e) => {
                    const chk = document.getElementById('chk_' + topic);
                    if (e.target !== chk) {
                        chk.checked = !chk.checked;
                        onTopicChange(chk);
                    }
                };
                option.innerHTML = `
                    <input type="checkbox" id="chk_${topic}" value="${topic}" ${checked} onclick="event.stopPropagation(); onTopicChange(this)">
                    <label style="flex:1; margin:0; cursor:pointer;">${topic}</label>
                `;
                list.appendChild(option);
            });
            updateSelectedText();
        }

        function onTopicChange(checkbox) {
            const val = checkbox.value;
            if (checkbox.checked) {
                if (!selectedTopics.includes(val)) {
                    selectedTopics.push(val);
                }
            } else {
                selectedTopics = selectedTopics.filter(t => t !== val);
            }
            document.getElementById('mqtt_topic').value = selectedTopics.join(',');
            updateSelectedText();
        }

        function updateSelectedText() {
            const textEl = document.getElementById('selectedTopicsText');
            if (selectedTopics.length === 0) {
                textEl.textContent = 'Select Topics...';
                textEl.style.color = 'var(--muted)';
            } else {
                textEl.textContent = selectedTopics.join(', ');
                textEl.style.color = 'var(--text)';
            }
        }

        function toggleDropdown(event) {
            event.stopPropagation();
            const wrapper = document.querySelector('.dropdown-multi');
            wrapper.classList.toggle('open');
        }

        function addCustomTopic(event) {
            event.stopPropagation();
            const input = document.getElementById('newTopicInput');
            const newTopic = input.value.trim();
            if (newTopic && !allTopics.includes(newTopic)) {
                allTopics.push(newTopic);
                selectedTopics.push(newTopic);
                document.getElementById('mqtt_topic').value = selectedTopics.join(',');
                renderTopics();
                input.value = '';
            }
        }

        document.addEventListener('click', function(e) {
            const dropdown = document.querySelector('.dropdown-multi');
            if (dropdown && !dropdown.contains(e.target)) {
                dropdown.classList.remove('open');
            }
        });

        renderTopics();

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
            padding: 20px;
        }
        .panel {
            width: 700px;
            max-width: 100%;
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
            padding: 18px 24px;
            border-bottom: 1px solid var(--line);
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        h2 {
            margin: 0;
            font-size: 18px;
            font-weight: 600;
            color: var(--text);
        }
        
        /* Navigation Bar */
        .panel__nav {
            display: flex;
            border-bottom: 1px solid var(--line);
            background: rgba(0, 0, 0, 0.25);
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
            padding: 20px;
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
                <button class="btn btn--primary" onclick="fetchLogs()">Refresh</button>
            </div>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/log" class="nav-item active">System Logs</a>
        </div>

        <div class="log-container" id="logBox">Loading device logs...</div>

        <!-- Ô nhập lệnh gửi trực tiếp -->
        <div class="log-control">
            <div style="display: flex; border-bottom: 1px solid var(--line); margin-bottom: 6px; gap: 8px;">
                <button type="button" class="tab-btn active" id="tabKioskBtn" onclick="switchControlTab('kiosk')">Kiosk Sim</button>
                <button type="button" class="tab-btn" id="tabManualBtn" onclick="switchControlTab('manual')">Manual Publish</button>
                <span id="kioskStatus" style="margin-left: auto; font-family: var(--mono); font-size: 11px; align-self: center; color: var(--muted);">Ready</span>
            </div>

            <!-- Tab 1: Kiosk Simulator -->
            <div id="kioskControlPanel" style="display: flex; flex-direction: column; gap: 10px;">
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

            <!-- Tab 2: Manual Control -->
            <div id="manualControlPanel" style="display: none; flex-direction: column; gap: 10px;">
                <div class="log-control__row">
                    <input type="text" id="cmdTopic" placeholder="Topic to publish..." style="flex: 1;">
                    <select id="templateSelect" onchange="applyTemplate()">
                        <option value="">-- Template --</option>
                        <option value="call">Call Ticket (BH-003)</option>
                        <option value="clear">Clear Screen</option>
                    </select>
                </div>
                <div class="log-control__row">
                    <input type="text" id="cmdPayload" placeholder='JSON Payload e.g. {"cmd":"clear_display"}'>
                    <button class="btn btn--primary" onclick="sendCmd()" style="padding: 10px 20px; border-radius: 4px;">Send</button>
                </div>
            </div>
        </div>

        <div class="panel__nav">
            <!-- Secondary nav fallback inside body if needed, otherwise ignore -->
        </div>
        <div class="panel__footer">
            <span class="status">Realtime logs (auto-refresh 5s)</span>
            <span class="status" style="color: var(--ok);">Active</span>
        </div>
    </div>

    <script>
        const devId = "{{DEV_ID}}";
        const devKey = "{{DEV_KEY}}";
        let servicesList = [];

        if (devId) {
            document.getElementById('cmdTopic').value = `qms/display/${devId}/command`;
        } else {
            document.getElementById('cmdTopic').value = "qms/display/command";
        }

        function switchControlTab(tab) {
            const kioskPanel = document.getElementById('kioskControlPanel');
            const manualPanel = document.getElementById('manualControlPanel');
            const kioskBtn = document.getElementById('tabKioskBtn');
            const manualBtn = document.getElementById('tabManualBtn');
            
            if (tab === 'kiosk') {
                kioskPanel.style.display = 'flex';
                manualPanel.style.display = 'none';
                kioskBtn.classList.add('active');
                manualBtn.classList.remove('active');
            } else {
                kioskPanel.style.display = 'none';
                manualPanel.style.display = 'flex';
                kioskBtn.classList.remove('active');
                manualBtn.classList.add('active');
            }
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
            const regex = /Payload:\s*(\{.*"cmd"\s*:\s*"init_config".*\})/g;
            let match;
            let lastJson = null;
            
            while ((match = regex.exec(logData)) !== null) {
                try {
                    lastJson = JSON.parse(match[1]);
                } catch(e) {}
            }
            
            if (lastJson && lastJson.services) {
                updateServiceDropdown(lastJson.services);
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

        function applyTemplate() {
            const select = document.getElementById('templateSelect');
            const payloadInput = document.getElementById('cmdPayload');
            if (select.value === 'call') {
                payloadInput.value = JSON.stringify({
                    cmd: "display_ticket",
                    data: {
                        ticket: "BH-003",
                        service: "Cấp thẻ Bảo hiểm y tế",
                        counter: "Quầy số 03",
                        status: "CALLING",
                        cust_name: "Nguyễn Văn A"
                    }
                });
            } else if (select.value === 'clear') {
                payloadInput.value = JSON.stringify({ cmd: "clear_display" });
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
        
        function sendCmd() {
            const topic = document.getElementById('cmdTopic').value.trim();
            const payload = document.getElementById('cmdPayload').value.trim();
            if (!topic || !payload) {
                alert("Please enter both topic and payload.");
                return;
            }
            sendMqtt(topic, payload, "Publishing manually...");
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

const char* login_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Device Login</title>
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
            padding: 20px;
        }
        .panel {
            width: 360px;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: 2px solid var(--accent);
            border-radius: 8px;
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
            overflow: hidden;
            padding: 24px;
        }
        .panel__header {
            text-align: center;
            margin-bottom: 24px;
        }
        .chip-icon {
            width: 40px;
            height: 40px;
            color: var(--accent);
            margin: 0 auto 10px;
        }
        h2 {
            margin: 0 0 6px;
            font-size: 18px;
            font-weight: 600;
        }
        .subtitle {
            font-size: 12px;
            color: var(--muted);
            margin: 0;
        }
        .field {
            margin-bottom: 16px;
        }
        .field label {
            display: block;
            margin-bottom: 6px;
            font-family: var(--mono);
            font-size: 11px;
            color: var(--muted);
        }
        input[type="password"] {
            width: 100%;
            padding: 10px 12px;
            font-family: var(--mono);
            font-size: 13.5px;
            color: var(--text);
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            border-radius: 5px;
            outline: none;
            transition: border-color 0.15s ease;
        }
        input:focus {
            border-color: var(--accent);
        }
        .error-msg {
            color: #ff6b6b;
            background: rgba(255, 107, 107, 0.1);
            border: 1px solid #ff6b6b;
            border-radius: 5px;
            padding: 8px 12px;
            font-family: var(--mono);
            font-size: 11px;
            margin-bottom: 16px;
            text-align: center;
            {{ERROR_STYLE}}
        }
        .submit {
            width: 100%;
            padding: 12px;
            font-family: var(--mono);
            font-size: 12.5px;
            font-weight: 700;
            letter-spacing: 1px;
            text-transform: uppercase;
            color: var(--ink);
            background: var(--accent);
            border: none;
            border-radius: 5px;
            cursor: pointer;
            transition: background 0.15s ease;
        }
        .submit:hover {
            background: #ffc578;
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <svg class="chip-icon" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <h2>Device Login</h2>
            <p class="subtitle">Enter password to manage device settings.</p>
        </div>

        <form action="/login" method="POST">
            <div class="error-msg">Incorrect password!</div>
            <div class="field">
                <label for="password">Password</label>
                <input type="password" id="password" name="password" required placeholder="••••••••" autofocus>
            </div>
            <button type="submit" class="submit">Login</button>
        </form>
    </div>
</body>
</html>
)html";

#endif // WIFI_CONFIG_HTML_H
