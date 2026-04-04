const { test, expect } = require('@playwright/test');

const mockStatus = {
    clock: {
        year: 2026,
        month: 4,
        day: 5,
        hour: 21,
        minute: 37,
        second: 15
    },
    firmware: {
        version: '2.0.0',
        buildRef: 'local-smoke',
        buildDate: '2026-04-05',
        buildTime: '21:37:15',
        idfVersion: 'ESP-IDF mock'
    },
    battery: {
        voltage: 3.98,
        percent: 82
    },
    network: {
        staConnected: true,
        apMode: false,
        staSsid: 'DomowaSiec',
        staLastConnectedEpoch: 1775417820,
        configuredStaSsid: 'DomowaSiec',
        configuredApSsid: 'Aquarium-Backup',
        ip: '192.168.0.57'
    },
    schedule: {
        lightMode: 0,
        dayStartHour: 9,
        dayStartMin: 0,
        dayEndHour: 21,
        dayEndMin: 30,
        airMode: 0,
        airStartHour: 8,
        airStartMin: 30,
        airEndHour: 22,
        airEndMin: 0,
        filterMode: 1,
        filterStartHour: 0,
        filterStartMin: 0,
        filterEndHour: 0,
        filterEndMin: 0,
        heaterMode: 0
    },
    feeding: {
        freq: 1,
        hour: 18,
        minute: 0,
        active: false,
        lastFeedEpoch: 1775409600
    },
    relays: {
        light: true,
        pump: true,
        heater: false,
        aerationPercent: 35
    },
    temperature: {
        current: 25.4,
        target: 25,
        hysteresis: 0.5,
        historyCapacity: 20,
        historyIntervalMinutes: 10,
        history: [
            { value: 24.7, epoch: 1775410800 },
            { value: 24.8, epoch: 1775411400 },
            { value: 24.9, epoch: 1775412000 },
            { value: 25.0, epoch: 1775412600 },
            { value: 25.1, epoch: 1775413200 },
            { value: 25.1, epoch: 1775413800 },
            { value: 25.2, epoch: 1775414400 },
            { value: 25.3, epoch: 1775415000 },
            { value: 25.4, epoch: 1775415600 },
            { value: 25.4, epoch: 1775416200 }
        ]
    }
};

const mockLogs = {
    normal: [
        'HTTP panel ready.',
        'SSE status stream attached.',
        'Dashboard rendered from mocked controller data.'
    ],
    critical: [
        'No critical entries in smoke test.'
    ]
};

const viewports = [
    { name: 'mobile', width: 390, height: 844 },
    { name: 'tablet', width: 768, height: 1024 },
    { name: 'desktop', width: 1280, height: 800 }
];

async function installApiMocks(page) {
    await page.addInitScript(
        ({ status, logs }) => {
            class MockEventSource {
                constructor(url) {
                    this.url = url;
                    this.readyState = 0;
                    this.listeners = new Map();
                    setTimeout(() => {
                        this.readyState = 1;
                        if (typeof this.onopen === 'function') {
                            this.onopen({ type: 'open' });
                        }
                        this.dispatch('ready', '{}');
                        this.dispatch('status', JSON.stringify(status));
                        this.dispatch('logs', JSON.stringify(logs));
                    }, 20);
                }

                addEventListener(type, handler) {
                    const handlers = this.listeners.get(type) || [];
                    handlers.push(handler);
                    this.listeners.set(type, handlers);
                }

                removeEventListener(type, handler) {
                    const handlers = this.listeners.get(type) || [];
                    this.listeners.set(
                        type,
                        handlers.filter((item) => item !== handler)
                    );
                }

                close() {
                    this.readyState = 2;
                }

                dispatch(type, data) {
                    const event = { type, data };
                    const handlers = this.listeners.get(type) || [];
                    handlers.forEach((handler) => handler(event));
                    const inlineHandler = this[`on${type}`];
                    if (typeof inlineHandler === 'function') {
                        inlineHandler(event);
                    }
                }
            }

            window.EventSource = MockEventSource;
        },
        { status: mockStatus, logs: mockLogs }
    );

    await page.route('**/api/status', async (route) => {
        await route.fulfill({
            status: 200,
            contentType: 'application/json; charset=utf-8',
            body: JSON.stringify(mockStatus)
        });
    });

    await page.route('**/api/logs', async (route) => {
        await route.fulfill({
            status: 200,
            contentType: 'application/json; charset=utf-8',
            body: JSON.stringify(mockLogs)
        });
    });

    await page.route('**/api/action', async (route) => {
        await route.fulfill({
            status: 200,
            contentType: 'application/json; charset=utf-8',
            body: JSON.stringify({
                success: true,
                code: 'ok',
                message: 'ok'
            })
        });
    });
}

for (const viewport of viewports) {
    test(`web dashboard smoke: ${viewport.name}`, async ({ page, baseURL }) => {
        const consoleErrors = [];
        const pageErrors = [];
        const externalRequests = [];
        const allowedOrigin = new URL(baseURL).origin;

        page.on('console', (msg) => {
            if (msg.type() === 'error') {
                consoleErrors.push(msg.text());
            }
        });
        page.on('pageerror', (error) => {
            pageErrors.push(error.message);
        });
        page.on('request', (request) => {
            const url = request.url();
            if (!url.startsWith('http')) {
                return;
            }
            if (new URL(url).origin !== allowedOrigin) {
                externalRequests.push(url);
            }
        });

        await installApiMocks(page);
        await page.setViewportSize({ width: viewport.width, height: viewport.height });
        await page.goto('/');

        await expect(page.locator('#dashboard')).toHaveClass(/active/);
        await expect(page.locator('#dashboard-temp-current')).toHaveText('25.4');
        await expect(page.locator('#network-status')).toHaveText('STA ONLINE');
        await expect(page.locator('#sidebar-firmware-version')).toHaveText('v2.0.0');
        await expect(page.locator('#logs-status')).toContainText('ESP32');

        const overflow = await page.evaluate(() => ({
            document: document.documentElement.scrollWidth,
            window: window.innerWidth
        }));
        expect(overflow.document).toBeLessThanOrEqual(overflow.window + 1);

        if (viewport.width <= 960) {
            const toggle = page.locator('#mobile-nav-toggle');
            await expect(toggle).toBeVisible();
            await toggle.click();
            await expect(page.locator('#app-sidebar')).toHaveClass(/mobile-open/);
            await page.locator('.nav-item[data-target="logi"]').click();
            await expect(page.locator('#logi')).toHaveClass(/active/);
            await expect(page.locator('#app-sidebar')).not.toHaveClass(/mobile-open/);
            await toggle.click();
            await expect(page.locator('#app-sidebar')).toHaveClass(/mobile-open/);
        } else {
            await page.locator('.nav-item[data-target="logi"]').click();
            await expect(page.locator('#logi')).toHaveClass(/active/);
        }

        await page.locator('.nav-item[data-target="ustawienia"]').click();
        await expect(page.locator('#ustawienia')).toHaveClass(/active/);
        await expect(page.locator('#settings-network-ip')).toHaveValue('192.168.0.57');

        expect(consoleErrors, `Console errors for ${viewport.name}`).toEqual([]);
        expect(pageErrors, `Page errors for ${viewport.name}`).toEqual([]);
        expect(externalRequests, `External requests for ${viewport.name}`).toEqual([]);
    });
}
