#ifndef PAGE_GPIO_H
#define PAGE_GPIO_H

const char* gpio_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 GPIO Mapping</title>
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
            --danger: #ff6b6b;
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
        h2 {
            margin: 0;
            font-size: 18px;
            font-weight: 600;
            color: var(--text);
            flex: 1;
            line-height: 1.2;
        }
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
            letter-spacing: 1px;
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
        
        .container {
            padding: 24px;
        }
        
        .btn {
            padding: 8px 12px;
            font-family: var(--mono);
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 1px;
            text-transform: uppercase;
            color: var(--text);
            background: transparent;
            border: 1px solid var(--line);
            border-radius: 4px;
            cursor: pointer;
            transition: all 0.15s ease;
        }
        .btn:hover { background: rgba(255,255,255,0.05); }
        .btn--primary {
            background: var(--accent);
            color: var(--ink);
            border-color: var(--accent);
        }
        .btn--primary:hover {
            background: #ffc578;
        }
        .btn--danger {
            background: var(--danger);
            color: var(--ink);
            border-color: var(--danger);
            padding: 6px 10px;
        }

        .row {
            display: flex;
            gap: 10px;
            margin-bottom: 15px;
            align-items: center;
        }
        
        select, input {
            padding: 10px;
            font-family: var(--mono);
            font-size: 13px;
            color: var(--text);
            background: rgba(0, 0, 0, 0.35);
            border: 1px solid var(--line);
            border-radius: 5px;
            flex: 1;
            outline: none;
        }
        select:focus { border-color: var(--accent); }
        
        .mapping-item {
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
            padding: 10px;
            background: rgba(255,255,255,0.02);
            border: 1px solid var(--line);
            border-radius: 5px;
            margin-bottom: 10px;
            align-items: center;
        }
        .mapping-item select {
            min-width: 0;
        }
        @media (max-width: 480px) {
            .container { padding: 12px; }
            .mapping-item {
                flex-direction: column;
                align-items: stretch;
            }
            .mapping-item select { flex: unset; width: 100%; }
            .mapping-item .btn--danger { align-self: flex-end; }
            .nav-item { padding: 10px 6px; font-size: 10px; letter-spacing: 0.5px; }
            .row { flex-wrap: wrap; }
        }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <h2>Hardware Settings</h2>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Configuration</a>
            <a href="/log" class="nav-item">System Logs</a>
            <a href="/gpio" class="nav-item active">GPIO Mapping</a>
            <a href="/ota" class="nav-item">Update</a>
        </div>

        <div class="container">
            <div class="row">
                <label style="flex: 0 0 100px; color: var(--muted); font-size: 12px; font-weight: bold;">BOARD TYPE:</label>
                <select id="boardType" onchange="updatePins()">
                    <option value="ESP32">ESP32</option>
                    <option value="ESP32-S3">ESP32-S3</option>
                </select>
            </div>
            
            <div style="margin-top: 20px; margin-bottom: 10px; border-bottom: 1px solid var(--line); padding-bottom: 10px;">
                <span style="font-weight: bold; color: var(--accent);">BUTTON MAPPINGS</span>
            </div>
            
            <div id="mappingContainer">
                <div style="text-align: center; color: var(--muted); padding: 24px 0;">Loading configuration...</div>
            </div>
            
            <div style="margin-top: 15px; display: flex; gap: 10px;">
                <button class="btn" onclick="addMappingRow()">+ Add Button</button>
                <button class="btn" id="syncBtn" onclick="fetchServices(true)">Sync Services</button>
            </div>
            
            <div style="margin-top: 30px;">
                <button class="btn btn--primary" style="width: 100%; padding: 12px;" onclick="saveConfig()">SAVE CONFIGURATION & REBOOT</button>
            </div>
        </div>
    </div>

    <script>
        const devId = "{{DEV_ID}}";
        const devKey = "{{DEV_KEY}}";
        const esp32Pins = [13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33];
        const esp32s3Pins = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 21, 35, 36, 37, 38, 39, 40, 41, 42, 47, 48];
        let services = [];
        let currentMappings = [];
        
        async function init() {
            await fetchServices();
            await fetchConfig();
        }

        async function fetchServices(triggerMqtt = false) {
            try {
                // Step 1: Always try API first (services may already be loaded from MQTT auto-sync)
                let res = await fetch('/api/services');
                services = await res.json();
                
                if (services.length > 0 && !triggerMqtt) {
                    renderAll();
                    return;
                }
                
                if (triggerMqtt) {
                    document.getElementById('syncBtn').innerText = "Syncing...";
                    
                    // Step 2: Send MQTT get_config request
                    const topic = `qms/sender/${devId}/request`;
                    const payload = JSON.stringify({ cmd: "get_config", secret_key: devKey });
                    const params = `topic=${encodeURIComponent(topic)}&payload=${encodeURIComponent(payload)}`;
                    await fetch('/publish', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                        body: params
                    });
                    
                    // Step 3: Poll API + fallback to log parsing (5 attempts, 1s each)
                    for (let i = 0; i < 5; i++) {
                        await new Promise(r => setTimeout(r, 1000));
                        
                        // Try API first
                        res = await fetch('/api/services');
                        services = await res.json();
                        if (services.length > 0) break;
                        
                        // Fallback: parse services from device log (proven method from Kiosk tab)
                        try {
                            const logRes = await fetch('/log_data');
                            const logData = await logRes.text();
                            const regex = /Nhấn phím \d+:\s*(.*?)\s*\(Service ID:\s*(\d+)\)/g;
                            let match;
                            let logServices = [];
                            while ((match = regex.exec(logData)) !== null) {
                                logServices.push({ id: parseInt(match[2]), name: match[1] });
                            }
                            if (logServices.length > 0) {
                                services = logServices;
                                break;
                            }
                        } catch(logErr) { /* ignore log parse errors */ }
                    }
                    
                    document.getElementById('syncBtn').innerText = "Sync Services";
                    
                    if (services.length === 0) {
                        alert("Không lấy được danh sách. Hãy kiểm tra kết nối MQTT!");
                    }
                }
                renderAll();
            } catch (e) {
                console.error("Failed to fetch services", e);
                if (triggerMqtt) document.getElementById('syncBtn').innerText = "Sync Services";
            }
        }
        
        async function fetchConfig() {
            try {
                const res = await fetch('/api/gpio_config');
                const conf = await res.json();
                if (conf.board) document.getElementById('boardType').value = conf.board;
                if (conf.mappings) currentMappings = conf.mappings;
                renderAll();
            } catch (e) {
                console.error("Failed to fetch config", e);
                document.getElementById('mappingContainer').innerHTML = "";
            }
        }

        function getSafePins() {
            return document.getElementById('boardType').value === 'ESP32-S3' ? esp32s3Pins : esp32Pins;
        }
        
        function updatePins() {
            renderAll();
        }

        function addMappingRow() {
            currentMappings.push({ service_id: services.length > 0 ? services[0].id : 0, pin: getSafePins()[0] });
            renderAll();
        }
        
        function removeMapping(index) {
            currentMappings.splice(index, 1);
            renderAll();
        }
        
        function updateMapping(index, field, value) {
            currentMappings[index][field] = parseInt(value);
        }

        function renderAll() {
            const container = document.getElementById('mappingContainer');
            container.innerHTML = '';
            
            if (currentMappings.length === 0) {
                container.innerHTML = '<div style="text-align: center; color: var(--muted); padding: 24px 0;">No buttons mapped.</div>';
                return;
            }
            
            const safePins = getSafePins();
            
            currentMappings.forEach((map, index) => {
                const row = document.createElement('div');
                row.className = 'mapping-item';
                
                let srvSelect = `<select style="flex: 2;" onchange="updateMapping(${index}, 'service_id', this.value)">`;
                if (services.length === 0) {
                    srvSelect += `<option value="${map.service_id}">ID: ${map.service_id}</option>`;
                } else {
                    services.forEach(s => {
                        srvSelect += `<option value="${s.id}" ${s.id === map.service_id ? 'selected' : ''}>[${s.id}] ${s.name}</option>`;
                    });
                }
                srvSelect += `</select>`;
                
                let pinSelect = `<select style="flex: 1;" onchange="updateMapping(${index}, 'pin', this.value)">`;
                safePins.forEach(p => {
                    pinSelect += `<option value="${p}" ${p === map.pin ? 'selected' : ''}>GPIO ${p}</option>`;
                });
                pinSelect += `</select>`;
                
                row.innerHTML = `
                    ${srvSelect}
                    ${pinSelect}
                    <button class="btn btn--danger" onclick="removeMapping(${index})">X</button>
                `;
                container.appendChild(row);
            });
        }
        
        async function saveConfig() {
            // Validate duplicates
            const pins = currentMappings.map(m => m.pin);
            if (new Set(pins).size !== pins.length) {
                alert("Error: Duplicate GPIO pins selected. A pin can only trigger one service.");
                return;
            }
            
            const payload = {
                board: document.getElementById('boardType').value,
                mappings: currentMappings
            };
            
            try {
                const res = await fetch('/api/gpio_config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(payload)
                });
                
                if (res.ok) {
                    alert("Configuration saved! ESP32 will now reboot.");
                    setTimeout(() => window.location.reload(), 3000);
                } else {
                    alert("Failed to save configuration.");
                }
            } catch (e) {
                alert("Error saving configuration.");
            }
        }
        
        init();
    </script>
</body>
</html>
)html";

#endif // PAGE_GPIO_H
