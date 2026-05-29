from PIL import Image, ImageDraw
import socket
import struct

W, H = 800, 480
ESP32_IP = "192.168.1.240"
ESP32_PORT = 8080

# Create your image
img = Image.new("1", (W, H), 1)
draw = ImageDraw.Draw(img)

draw.rectangle([20, 20, 780, 460], outline=0, width=3)
draw.text((40, 40),  "Hello from Python!", fill=0)
draw.text((40, 100), "Line two goes here",  fill=0)

# Save as 1-bit BMP and read bytes
img.save("screen.bmp")
with open("screen.bmp", "rb") as f:
    data = f.read()

print(f"Sending {len(data)} bytes")

# Connect and send: 4-byte length header + BMP data
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((ESP32_IP, ESP32_PORT))
sock.sendall(struct.pack("<I", len(data)))  # little-endian uint32 length
sock.sendall(data)

# Wait for response
response = sock.recv(1024)
print(f"Response: {response.decode()}")
sock.close()