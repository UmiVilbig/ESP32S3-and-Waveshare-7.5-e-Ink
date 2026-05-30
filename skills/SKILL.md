---
name: eink-display
description: Send text and images to an ESP32-connected 800x480 e-ink display over WiFi
version: 1.0.0
command-dispatch: tool
command-tool: send_to_display
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

You MUST use the `send_to_display` MCP tool to interact with the e-ink display. Do NOT attempt to connect to the ESP32 directly via HTTP, TCP, curl, or any other method. The MCP server handles the connection protocol internally.

## How to use

Call the `send_to_display` tool with a `lines` parameter — an array of strings. Each string is rendered as one row of text on the 800x480 e-ink screen.

Example tool call:
```json
{
  "name": "send_to_display",
  "arguments": {
    "lines": ["Hello World", "Line two goes here"]
  }
}
```

## Rules

- Always use the `send_to_display` MCP tool. Never make direct network requests to the ESP32.
- Keep lines under 70 characters to fit the 800px width.
- Send at most 7 lines per call (drawable area is ~420px tall at 60px spacing).
- The display is black and white only.
- If the tool returns an error, tell the user to check that ESP32_IP is correct and the device is powered on.
