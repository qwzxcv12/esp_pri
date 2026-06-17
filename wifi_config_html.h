#ifndef WIFI_CONFIG_HTML_H
#define WIFI_CONFIG_HTML_H

const char* html_page = R"html(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Device Configuration</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #12121e 0%, #08080c 100%);
            color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px 0;
            box-sizing: border-box;
        }
        .container {
            background: rgba(255, 255, 255, 0.03);
            backdrop-filter: blur(12px);
            -webkit-backdrop-filter: blur(12px);
            border: 1px solid rgba(255, 255, 255, 0.08);
            padding: 30px;
            border-radius: 16px;
            box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.5);
            width: 380px;
            max-width: 90%;
            box-sizing: border-box;
        }
        h2 {
            margin-top: 0;
            margin-bottom: 5px;
            color: #00adb5;
            text-align: center;
            font-size: 24px;
        }
        .subtitle {
            color: #888888;
            font-size: 13px;
            text-align: center;
            margin-bottom: 25px;
        }
        .section-title {
            font-size: 14px;
            font-weight: bold;
            color: #00adb5;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin: 20px 0 10px 0;
            padding-bottom: 5px;
            border-bottom: 1px solid rgba(0, 173, 181, 0.2);
        }
        .input-group {
            margin-bottom: 15px;
            text-align: left;
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-size: 13px;
            color: #aaaaaa;
        }
        input[type="text"], input[type="password"], input[type="number"] {
            width: 100%;
            padding: 10px 12px;
            box-sizing: border-box;
            border: 1px solid rgba(255, 255, 255, 0.15);
            background: rgba(0, 0, 0, 0.3);
            color: #ffffff;
            border-radius: 6px;
            outline: none;
            transition: all 0.3s ease;
            font-size: 14px;
        }
        input:focus {
            border-color: #00adb5;
            box-shadow: 0 0 8px rgba(0, 173, 181, 0.4);
            background: rgba(0, 0, 0, 0.5);
        }
        button {
            width: 100%;
            padding: 12px;
            background: linear-gradient(90deg, #00adb5, #00bfa5);
            border: none;
            color: white;
            font-size: 16px;
            font-weight: bold;
            border-radius: 6px;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-top: 20px;
            box-shadow: 0 4px 15px rgba(0, 173, 181, 0.3);
        }
        button:hover {
            background: linear-gradient(90deg, #00bfa5, #00d2b4);
            box-shadow: 0 6px 20px rgba(0, 191, 165, 0.5);
            transform: translateY(-1px);
        }
        button:active {
            transform: translateY(1px);
        }
        .footer {
            margin-top: 25px;
            font-size: 11px;
            color: #555555;
            text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h2>ESP32 Device Config</h2>
        <div class="subtitle">Configure WiFi, MQTT, and WebSocket Settings</div>
        
        <form action="/config" method="POST">
            <div class="section-title">WiFi Settings</div>
            <div class="input-group">
                <label for="ssid">WiFi SSID</label>
                <input type="text" id="ssid" name="ssid" required placeholder="Enter WiFi SSID" value="{{SSID}}">
            </div>
            <div class="input-group">
                <label for="password">WiFi Password</label>
                <input type="password" id="password" name="password" placeholder="Enter WiFi Password" value="{{PASSWORD}}">
            </div>
            
            <div class="section-title">MQTT Settings</div>
            <div class="input-group">
                <label for="mqtt_server">MQTT Broker Host</label>
                <input type="text" id="mqtt_server" name="mqtt_server" placeholder="e.g. broker.hivemq.com" value="{{MQTT_SERVER}}">
            </div>
            <div class="input-group">
                <label for="mqtt_port">MQTT Port</label>
                <input type="number" id="mqtt_port" name="mqtt_port" placeholder="1883" value="{{MQTT_PORT}}">
            </div>
            <div class="input-group">
                <label for="mqtt_user">MQTT Username</label>
                <input type="text" id="mqtt_user" name="mqtt_user" placeholder="Optional" value="{{MQTT_USER}}">
            </div>
            <div class="input-group">
                <label for="mqtt_pass">MQTT Password</label>
                <input type="password" id="mqtt_pass" name="mqtt_pass" placeholder="Optional" value="{{MQTT_PASS}}">
            </div>
            <div class="input-group">
                <label for="mqtt_topic">MQTT Topic</label>
                <input type="text" id="mqtt_topic" name="mqtt_topic" placeholder="e.g. esp32/sensor" value="{{MQTT_TOPIC}}">
            </div>
            
            <div class="section-title">WebSocket Settings</div>
            <div class="input-group">
                <label for="ws_url">WebSocket URL</label>
                <input type="text" id="ws_url" name="ws_url" placeholder="e.g. ws://192.168.1.100:81" value="{{WS_URL}}">
            </div>
            
            <button type="submit">Save & Restart</button>
        </form>
        <div class="footer">Antigravity ESP-IDF Manager</div>
    </div>
</body>
</html>
)html";

#endif // WIFI_CONFIG_HTML_H
