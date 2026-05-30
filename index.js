import { definePluginEntry } from "openclaw/plugin-sdk/plugin-entry";
import { Type } from "@sinclair/typebox";

export default definePluginEntry({
  id: "eink-display",
  name: "E-Ink Display",
  description: "Send text to an ESP32-connected 800x480 e-ink display over WiFi",
  register(api) {
    api.registerTool({
      name: "send_to_display",
      description:
        "Render text lines onto the 800x480 e-ink display connected via ESP32. Each string in lines is drawn on its own row.",
      parameters: Type.Object({
        lines: Type.Array(Type.String(), {
          description: "Text lines to display, one per row",
        }),
      }),
      async execute(_id, params, { config }) {
        const { spawn } = await import("child_process");
        const ip = config?.esp32Ip ?? process.env.ESP32_IP ?? "192.168.1.240";
        const port = config?.esp32Port ?? process.env.ESP32_PORT ?? "8080";

        return new Promise((resolve, reject) => {
          const proc = spawn("python", ["eink_server.py"], {
            cwd: import.meta.dirname,
            env: { ...process.env, ESP32_IP: ip, ESP32_PORT: String(port) },
            stdio: ["pipe", "pipe", "pipe"],
          });

          const input = JSON.stringify({
            method: "tools/call",
            params: { name: "send_to_display", arguments: { lines: params.lines } },
          });
          proc.stdin.write(input + "\n");
          proc.stdin.end();

          let stdout = "";
          proc.stdout.on("data", (d) => (stdout += d));
          proc.on("close", () => {
            resolve({
              content: [{ type: "text", text: stdout.trim() || "Sent to display" }],
            });
          });
          proc.on("error", (err) => {
            reject(new Error(`Failed to run eink_server.py: ${err.message}`));
          });
        });
      },
    });
  },
});
