const path = require('node:path');
const { defineConfig } = require('@playwright/test');

const webRoot = path.resolve(__dirname, '..', '..', 'firmware', 'web');

module.exports = defineConfig({
    testDir: __dirname,
    timeout: 30000,
    fullyParallel: true,
    reporter: process.env.CI ? [['github'], ['html', { open: 'never' }]] : 'list',
    use: {
        baseURL: 'http://127.0.0.1:4173',
        trace: 'retain-on-failure'
    },
    webServer: {
        command: `python -m http.server 4173 --bind 127.0.0.1 --directory "${webRoot}"`,
        url: 'http://127.0.0.1:4173',
        reuseExistingServer: !process.env.CI,
        stdout: 'ignore',
        stderr: 'pipe'
    }
});
