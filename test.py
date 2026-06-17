import paho.mqtt.client as mqtt
import json
import time
import threading

# --- CẤU HÌNH (Sử dụng ID và KEY của thiết bị HIỂN THỊ trong DB) ---
# Chú ý: DEVICE_ID này phải là ID bạn đã gán cho Quầy (Counter) trong Admin
DEVICE_ID = "31e46c59a1cf3c8a5aa1602a6dfac5098334ba5a13839088446f889d613741aa"
SECRET_KEY = "24f9738d029af6ef1e361043dd0dfe09e2ba992f55b6e1fda35eacf22bce3cc9"

MQTT_HOST, MQTT_PORT = "qms2.camdvr.org", 1993
MQTT_USER, MQTT_PASS = "thom", "301258"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        # Lắng nghe topic của Display
        topic = f"qms/display/{DEVICE_ID}/command"
        client.subscribe(topic)
        print(f"✅ [DISPLAY] Đã kết nối. Đang đợi nhân viên GỌI SỐ tại topic: {topic}")
    else:
        print(f"❌ Kết nối thất bại: {rc}")

def on_message(client, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        cmd = data.get("cmd")
        ticket_info = data.get("data", {})

        if cmd == "display_ticket":
            print("\n" + "🔔" + "═"*35 + "🔔")
            print(f"║ 📢 ĐANG GỌI SỐ: {ticket_info.get('ticket', 'N/A'):<17} ║")
            print(f"║ 🖥️ Quầy: {ticket_info.get('counter', 'N/A'):<25} ║")
            print(f"║ 📂 Dịch vụ: {ticket_info.get('service', 'N/A')[:20]:<20} ║")
            if ticket_info.get('cust_name'):
                print(f"║ 👤 Khách: {ticket_info.get('cust_name', ''):<25} ║")
            print("╚" + "═"*35 + "╝")
            
        elif cmd == "clear_display":
            print("\n🧹 [Lệnh] Xóa màn hình hiển thị (Quầy tạm nghỉ).")
    except Exception as e:
        print(f"⚠️ Lỗi xử lý: {e}")

client = mqtt.Client()
client.username_pw_set(MQTT_USER, MQTT_PASS)
client.on_connect, client.on_message = on_connect, on_message
client.connect(MQTT_HOST, MQTT_PORT, 60)

# Heartbeat để server thấy thiết bị đang Online
threading.Thread(target=lambda: [ (client.publish(f"qms/heartbeat/{DEVICE_ID}", json.dumps({"secret_key": SECRET_KEY})), time.sleep(20)) for _ in iter(int, 1) ], daemon=True).start()

print(f"🚀 Simulator Display {DEVICE_ID[:8]} đang chạy...")
client.loop_forever()