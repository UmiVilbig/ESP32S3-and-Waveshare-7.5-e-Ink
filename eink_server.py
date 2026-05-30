from mcp.server.fastmcp import FastMCP
from PIL import Image, ImageDraw
import socket
import struct
import os
import tempfile
import io

mcp = FastMCP("eink-display")

W, H = 800, 480
ESP32_IP = os.environ.get("ESP32_IP", "192.168.1.240")
ESP32_PORT = int(os.environ.get("ESP32_PORT", "8080"))


def render_image(lines: list[str]) -> bytes:
    img = Image.new("1", (W, H), 1)
    draw = ImageDraw.Draw(img)
    draw.rectangle([20, 20, 780, 460], outline=0, width=3)
    y = 40
    for line in lines:
        draw.text((40, y), line, fill=0)
        y += 60
    buf = io.BytesIO()
    img.save(buf, format="BMP")
    return buf.getvalue()


def send_to_esp32(data: bytes) -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    try:
        sock.connect((ESP32_IP, ESP32_PORT))
        sock.sendall(struct.pack("<I", len(data)))
        sock.sendall(data)
        response = sock.recv(1024)
        return response.decode()
    finally:
        sock.close()


@mcp.tool()
def send_to_display(lines: list[str]) -> str:
    """Render text lines onto the 800x480 e-ink display connected via ESP32.
    Each string in `lines` is drawn on its own row."""
    try:
        data = render_image(lines)
        response = send_to_esp32(data)
        return f"Sent {len(data)} bytes to {ESP32_IP}:{ESP32_PORT}. ESP32 response: {response}"
    except ConnectionRefusedError:
        return f"ERROR: Connection refused at {ESP32_IP}:{ESP32_PORT}. Is the ESP32 powered on and connected to WiFi?"
    except socket.timeout:
        return f"ERROR: Connection timed out reaching {ESP32_IP}:{ESP32_PORT}. Check the IP address and network."
    except Exception as e:
        return f"ERROR: {type(e).__name__}: {e}"


if __name__ == "__main__":
    mcp.run()
