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
          "Render text lines onto the 800x480 e-ink display connected via ESP32. Each string in lines is drawn on its own row.",
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
