import { definePluginEntry } from "openclaw/plugin-sdk/plugin-entry";
import { Type } from "@sinclair/typebox";

export default definePluginEntry({
  id: "eink-display",
  name: "E-Ink Display",
  description: "Send text to an ESP32-connected 800x480 e-ink display over WiFi",
  register(api) {
    api.registerTool(
      {
        name: "send_to_display",
        description:
          "Send text to the 800x480 e-ink display. Pass an array of strings — each is rendered on its own row. This tool handles all rendering and network communication automatically; do NOT make direct HTTP or TCP requests to the ESP32.",
        parameters: Type.Object({
          lines: Type.Array(Type.String(), {
            description: "Text lines to display, one per row",
          }),
        }),
      },
      { mcp: "eink-display" }
    );
  },
});
