import { definePluginEntry } from "openclaw/plugin-sdk/plugin-entry";
import { Type } from "@sinclair/typebox";

export default definePluginEntry({
  id: "eink-display",
  name: "E-Ink Display",
  description:
    "Send text, graphics, icons, and SVGs to an ESP32-connected 800x480 e-ink display over WiFi",
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
      { mcp: "eink-display" },
    );

    api.registerTool(
      {
        name: "draw_on_display",
        description:
          'Draw shapes, text, icons, and graphics on the 800x480 e-ink display. Pass an array of drawing elements. Each element has a "type" and type-specific properties. Types: rect, circle, ellipse, line, polygon, arc, text, icon. The icon type uses FontAwesome 6 (270+ icons) and Weather Icons (120+ icons) fonts with 170+ aliases. Use list_icons to browse available icons by category.',
        parameters: Type.Object({
          elements: Type.Array(Type.Any(), {
            description: "Array of drawing element objects",
          }),
        }),
      },
      { mcp: "eink-display" },
    );

    api.registerTool(
      {
        name: "send_svg_to_display",
        description:
          'Render an SVG string onto the 800x480 e-ink display. Use viewBox="0 0 800 480" for pixel-accurate layout. Dark colours become black, light become white. Requires cairosvg on the server.',
        parameters: Type.Object({
          svg: Type.String({
            description: "SVG markup string",
          }),
        }),
      },
      { mcp: "eink-display" },
    );

    api.registerTool(
      {
        name: "list_icons",
        description:
          "List available icon names for draw_on_display, optionally filtered by category. Categories: weather, wi, nature, arrows, status, ui, shapes, objects, people, tech, communication, charts, files, media, transport, health, commerce, aliases, all.",
        parameters: Type.Object({
          category: Type.Optional(
            Type.String({
              description:
                "Category to filter by (e.g. weather, wi, nature, transport, health). Omit to see a summary of all categories.",
            }),
          ),
        }),
      },
      { mcp: "eink-display" },
    );
  },
});
