import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";

const proxyTarget = process.env.VITE_API_PROXY_TARGET;

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: proxyTarget
    ? {
        proxy: {
          "/api": {
            target: proxyTarget,
            changeOrigin: true,
          },
          "/settime": {
            target: proxyTarget,
            changeOrigin: true,
          },
          "/update": {
            target: proxyTarget,
            changeOrigin: true,
          },
        },
      }
    : undefined,
});
