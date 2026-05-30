---
name: eink-display
description: Send text and images to an ESP32-connected 800x480 e-ink display over WiFi
version: 1.0.0
command-dispatch: tool
command-tool: send_to_display
disable-model-invocation: false
openclaw:
  emoji: "🖥"
  requires:
    env:
      - ESP32_IP
    bins:
      - python
  primaryEnv: ESP32_IP
---

# E-Ink Display

IMPORTANT: You MUST call the `send_to_display` tool to use this skill. Do NOT attempt to connect to the ESP32 yourself. Do NOT use HTTP, curl, TCP, UDP, or any direct network requests. The `send_to_display` tool handles the entire protocol internally.

## Usage

Call the `send_to_display` tool with one argument:

- **lines** (array of strings): Each string is rendered as one row on the display.

Example:
```json
{
  "name": "send_to_display",
  "arguments": {
    "lines": ["Hello World"]
  }
}
```

## Constraints

- Maximum 7 lines per call.
- Keep each line under 70 characters.
- Black and white only.
- If the tool returns an error, tell the user to verify ESP32_IP and that the device is on.

## What NOT to do

- Do NOT make HTTP requests to the ESP32.
- Do NOT send raw TCP or UDP packets.
- Do NOT try to guess the device protocol.
- ONLY use the `send_to_display` tool.
