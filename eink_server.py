from mcp.server.fastmcp import FastMCP
from PIL import Image, ImageDraw, ImageFont
import socket
import struct
import os

mcp = FastMCP("eink-display")

W, H = 800, 480
ESP32_IP = "192.168.1.240"
ESP32_PORT = 8080
BMP_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "screen.bmp")


def render_image(lines: list[str]) -> bytes:
    img = Image.new("1", (W, H), 1)
    draw = ImageDraw.Draw(img)
    draw.rectangle([20, 20, 780, 460], outline=0, width=3)
    y = 40
    for line in lines:
        draw.text((40, y), line, fill=0)
        y += 60
    img.save(BMP_PATH)
    with open(BMP_PATH, "rb") as f:
        return f.read()


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
    data = render_image(lines)
    response = send_to_esp32(data)
    return f"Sent {len(data)} bytes. Response: {response}"


if __name__ == "__main__":
    mcp.run()
