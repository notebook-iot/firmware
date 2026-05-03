import machine
import utime
import json
import random

i2c = machine.I2C(0, sda=machine.Pin(0), scl=machine.Pin(1), freq=100000)
ESP32_ADDR = 0x12

print("Starting Pico I2C Master...")
print("Scanning for ESP32 at 0x12...")

while True:
    devices = i2c.scan()
    if ESP32_ADDR in devices:
        print("Found ESP32! Sending data...")

        data = {
            "temp": round(random.uniform(20.0, 30.0), 2),
            "humidity": random.randint(40, 60),
            "status": "active"
        }

        payload = json.dumps(data) + '\0'

        try:
            i2c.writeto(ESP32_ADDR, payload)
            print(f"Sent: {payload[:-1]}")
        except Exception as e:
            print(f"Error sending data: {e}")

    else:
        print(f"ESP32 (0x12) not found on I2C bus. Devices found: {devices}")

    utime.sleep(5)
