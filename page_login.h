#ifndef PAGE_LOGIN_H
#define PAGE_LOGIN_H

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
            padding: 24px 16px;
            overflow-y: scroll;
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
            <svg class="logo chip-icon" style="flex: none;" style="width: 40px; height: 40px; border-radius: 10px; background: linear-gradient(135deg, var(--accent), #ff8f00); padding: 8px; color: var(--ink); box-shadow: 0 4px 12px var(--accent-dim); flex: none;" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg">
                <rect x="9" y="9" width="14" height="14" rx="1.5" stroke="currentColor" stroke-width="1.6"/>
                <rect x="13" y="13" width="6" height="6" rx="0.5" stroke="currentColor" stroke-width="1.4"/>
                <path d="M9 13H4M9 19H4M28 13H23M28 19H23M13 9V4M19 9V4M13 28V23M19 28V23" stroke="currentColor" stroke-width="1.6" stroke-linecap="round"/>
            </svg>
            <div class="panel__heading">
                <div class="device-id">ESP32 &middot; SoC</div>
                <h2>Authentication</h2>
                <p class="subtitle">Authenticate to access the device.</p>
            </div>
            <div class="status"><span class="status-dot"></span>Active</div>
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

#endif // PAGE_LOGIN_H
