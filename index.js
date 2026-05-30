module.exports = function activate(api) {
  api.registerTool("send_to_display", {
    description: "Render text lines onto the 800x480 e-ink display connected via ESP32",
    inputSchema: {
      type: "object",
      properties: {
        lines: {
          type: "array",
          items: { type: "string" },
          description: "Text lines to display, one per row"
        }
      },
      required: ["lines"]
    },
    mcp: {
      server: "eink-display",
      tool: "send_to_display"
    }
  });
};
