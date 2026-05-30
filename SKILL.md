---
name: eink-display
description: Send text and images to an ESP32-connected 800x480 e-ink display over WiFi
version: 1.0.0
openclaw:
  emoji: "🖥"
  requires:
    env:
      - ESP32_IP
    bins:
      - python
  primaryEnv: ESP32_IP
---

# E-Ink Display Skill

## Purpose

Render text onto a 7.5" e-ink display (GDEY075T7, 800x480) connected to an ESP32 over WiFi. Uses the `send_to_display` MCP tool provided by the `eink-display` MCP server.

## Workflow

1. When the user asks to display text, show a message, or update the e-ink screen, use this skill.
2. Call the `send_to_display` MCP tool with a `lines` array — each string becomes one row of text on the display.
3. The tool renders the text as a 1-bit BMP image and sends it to the ESP32 over TCP.
4. Report the response back to the user (byte count and ESP32 acknowledgment).

## Output Format

After sending, confirm with:
- Number of bytes sent
- ESP32 response status

## Rules

- Each line should be short enough to fit within the 800px width (roughly 60-80 characters depending on font).
- The display is black and white only — no grayscale or color.
- The ESP32 must be powered on and connected to the same network.
- If the connection fails, suggest the user check that ESP32_IP is correct and the device is online.
- Do not send more than ~8 lines at once; the default line spacing is 60px and the drawable area is ~420px tall.
