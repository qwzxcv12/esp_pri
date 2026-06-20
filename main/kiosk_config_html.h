#ifndef KIOSK_CONFIG_HTML_H
#define KIOSK_CONFIG_HTML_H

const char* kiosk_html_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kiosk Setup</title>
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
            --danger: #f85149;
            --mono: ui-monospace, 'SF Mono', 'Cascadia Code', 'Consolas', 'Courier New', monospace;
            --sans: -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        }
        * { box-sizing: border-box; }
        body {
            font-family: var(--sans);
            background-color: var(--ink);
            color: var(--text);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 24px 0;
        }
        .panel {
            width: 480px;
            max-width: 95%;
            background: var(--panel);
            border: 1px solid var(--line);
            border-top: 2px solid var(--accent);
            border-radius: 8px;
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.45);
        }
        .panel__header {
            padding: 22px 24px 18px;
            border-bottom: 1px solid var(--line);
        }
        h2 { margin: 0 0 6px; font-size: 19px; color: var(--text); }
        .subtitle { font-size: 12.5px; color: var(--muted); margin: 0; }
        
        .panel__nav {
            display: flex; border-bottom: 1px solid var(--line); background: rgba(0, 0, 0, 0.25);
        }
        .nav-item {
            flex: 1; text-align: center; padding: 14px; font-family: var(--mono);
            font-size: 11px; font-weight: 700; text-transform: uppercase;
            color: var(--muted); text-decoration: none; border-bottom: 2px solid transparent;
        }
        .nav-item.active { color: var(--accent); border-bottom-color: var(--accent); background: rgba(255, 255, 255, 0.04); }
        .nav-item:hover:not(.active) { color: var(--text); }

        form { padding: 4px 24px 24px; }
        .section { padding-top: 20px; }
        .section__title {
            display: flex; align-items: baseline; gap: 8px; margin-bottom: 14px;
        }
        .section__index { font-family: var(--mono); font-size: 11px; color: var(--accent); }
        .section__label { font-family: var(--mono); font-size: 11px; font-weight: 600; text-transform: uppercase; }
        .section__title::after { content: ''; flex: 1; height: 1px; background: var(--line); }
        
        .field { margin-bottom: 12px; }
        .field label { display: block; margin-bottom: 5px; font-family: var(--mono); font-size: 11px; color: var(--muted); }
        input[type="text"], input[type="number"] {
            width: 100%; padding: 10px 12px; font-family: var(--mono); font-size: 13.5px;
            color: var(--text); background: rgba(0, 0, 0, 0.35); border: 1px solid var(--line);
            border-radius: 5px; outline: none;
        }
        input:focus { border-color: var(--accent); }

        .btn-row {
            display: flex; gap: 8px; margin-bottom: 8px; align-items: center;
            background: rgba(0, 0, 0, 0.2); padding: 8px; border: 1px solid var(--line); border-radius: 5px;
        }
        .btn-row input { margin-bottom: 0; }
        .btn-row .del-btn {
            background: transparent; color: var(--danger); border: 1px solid var(--danger);
            border-radius: 4px; padding: 6px 10px; cursor: pointer; font-weight: bold;
        }
        .btn-row .del-btn:hover { background: rgba(248, 81, 73, 0.1); }
        .add-btn {
            width: 100%; padding: 10px; background: transparent; border: 1px dashed var(--line);
            color: var(--accent); border-radius: 5px; cursor: pointer; font-family: var(--mono);
        }
        .add-btn:hover { border-color: var(--accent); }

        .submit {
            width: 100%; margin-top: 22px; padding: 13px; font-family: var(--mono);
            font-size: 13px; font-weight: 700; color: var(--ink); background: var(--accent);
            border: none; border-radius: 6px; cursor: pointer;
        }
        .submit:hover { background: #ffc578; }
    </style>
</head>
<body>
    <div class="panel">
        <div class="panel__header">
            <h2>Kiosk & Printer Setup</h2>
            <p class="subtitle">Configure buttons, service mapping, and thermal printer pins.</p>
        </div>

        <div class="panel__nav">
            <a href="/" class="nav-item">Network</a>
            <a href="/kiosk" class="nav-item active">Kiosk</a>
            <a href="/log" class="nav-item">Logs</a>
        </div>

        <form action="/kiosk" method="POST" id="kioskForm">
            <div class="section">
                <div class="section__title">
                    <span class="section__index">01</span>
                    <span class="section__label">Printer Config</span>
                </div>
                <div style="display: flex; gap: 12px;">
                    <div class="field" style="flex: 1;">
                        <label for="ptr_tx">TX Pin</label>
                        <input type="number" id="ptr_tx" name="ptr_tx" value="{{PTR_TX}}" required>
                    </div>
                    <div class="field" style="flex: 1;">
                        <label for="ptr_rx">RX Pin</label>
                        <input type="number" id="ptr_rx" name="ptr_rx" value="{{PTR_RX}}" required>
                    </div>
                </div>
            </div>

            <div class="section">
                <div class="section__title">
                    <span class="section__index">02</span>
                    <span class="section__label">Button & Service Mapping</span>
                </div>
                <div id="btnContainer">
                    <!-- Javascript will render rows here -->
                </div>
                <button type="button" class="add-btn" onclick="addBtnRow('', '')">+ Add Button Mapping</button>
            </div>
            
            <input type="hidden" id="btn_cfg" name="btn_cfg" value="">
            <button type="button" class="submit" onclick="saveForm()">Save &amp; Reboot</button>
        </form>
    </div>

    <script>
        const initialCfg = `{{BTN_CFG}}`; 
        const container = document.getElementById('btnContainer');

        function addBtnRow(pinVal, svcVal) {
            const row = document.createElement('div');
            row.className = 'btn-row';
            row.innerHTML = `
                <input type="number" placeholder="GPIO" value="${pinVal}" style="width: 80px;" class="pin-in">
                <input type="text" placeholder="Service Name (e.g. Khám nội)" value="${svcVal}" style="flex: 1;" class="svc-in">
                <button type="button" class="del-btn" onclick="this.parentElement.remove()">X</button>
            `;
            container.appendChild(row);
        }

        function renderInitial() {
            try {
                // Remove potential quotes or parse properly
                const cfg = JSON.parse(initialCfg || '[]');
                cfg.forEach(c => addBtnRow(c.pin, c.svc));
            } catch(e) {
                // If empty or invalid, add one empty row
                addBtnRow('', '');
            }
        }

        function saveForm() {
            const rows = document.querySelectorAll('.btn-row');
            const data = [];
            rows.forEach(r => {
                const pin = r.querySelector('.pin-in').value.trim();
                const svc = r.querySelector('.svc-in').value.trim();
                if (pin && svc) {
                    data.push({ pin: parseInt(pin), svc: svc });
                }
            });
            document.getElementById('btn_cfg').value = JSON.stringify(data);
            document.getElementById('kioskForm').submit();
        }

        renderInitial();
    </script>
</body>
</html>
)html";

#endif
