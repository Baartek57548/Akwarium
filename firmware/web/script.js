const API_STATUS = '/api/status';
const API_ACTION = '/api/action';
const API_LOGS = '/api/logs';
const API_EVENTS = '/api/events';
const API_OTA = '/update';

let backendConnected = false;
let activeLogType = 'normal';
let cachedLogs = { normal: [], critical: [] };
let deviceClockBaseDate = null;
let deviceClockSyncedAtMs = 0;
let lastStatusData = null;
const OLED_SAVE_ACTIONS = new Set(['save_schedule', 'save_network', 'set_light', 'set_filter']);
const OLED_SAVE_TITLES = {
    save_schedule: 'Zapisywanie harmonogramow',
    save_network: 'Zapisywanie ustawien WiFi',
    set_light: 'Zapisywanie stanu swiatla',
    set_filter: 'Zapisywanie stanu filtra'
};
let oledSaveOverlayActiveCount = 0;
let oledSaveOverlayShownAtMs = 0;
let oledSaveOverlayHideTimer = null;
let eventStream = null;
let eventStreamConnected = false;
let statusPollingTimer = null;
let logsPollingTimer = null;

function makeLocalIcon(paths) {
    return `<svg viewBox="0 0 24 24" aria-hidden="true" focusable="false">${paths}</svg>`;
}

const LOCAL_ICON_SVGS = {
    'fa-water': makeLocalIcon(`
        <path fill="none" d="M3 7c2 2 4 2 6 0s4-2 6 0 4 2 6 0"/>
        <path fill="none" d="M3 12c2 2 4 2 6 0s4-2 6 0 4 2 6 0"/>
        <path fill="none" d="M3 17c2 2 4 2 6 0s4-2 6 0 4 2 6 0"/>
    `),
    'fa-gauge-high': makeLocalIcon(`
        <path fill="none" d="M4 14a8 8 0 1 1 16 0"/>
        <path fill="none" d="M12 14l4-4"/>
        <circle cx="12" cy="14" r="1" fill="currentColor" stroke="none"/>
    `),
    'fa-calendar-days': makeLocalIcon(`
        <rect x="3" y="5" width="18" height="16" rx="2" fill="none"/>
        <path fill="none" d="M8 3v4M16 3v4M3 10h18"/>
        <circle cx="8" cy="14" r="0.9" fill="currentColor" stroke="none"/>
        <circle cx="12" cy="14" r="0.9" fill="currentColor" stroke="none"/>
        <circle cx="16" cy="14" r="0.9" fill="currentColor" stroke="none"/>
        <circle cx="8" cy="18" r="0.9" fill="currentColor" stroke="none"/>
        <circle cx="12" cy="18" r="0.9" fill="currentColor" stroke="none"/>
        <circle cx="16" cy="18" r="0.9" fill="currentColor" stroke="none"/>
    `),
    'fa-terminal': makeLocalIcon(`
        <path fill="none" d="M4 7l4 4-4 4"/>
        <path fill="none" d="M12 15h8"/>
        <path fill="none" d="M3 20h18"/>
    `),
    'fa-cloud-arrow-up': makeLocalIcon(`
        <path fill="none" d="M7 18a4 4 0 0 1 .9-7.9A5 5 0 0 1 17.5 12H18a3 3 0 0 1 0 6H7z"/>
        <path fill="none" d="M12 16V9"/>
        <path fill="none" d="m9.5 11.5 2.5-2.5 2.5 2.5"/>
    `),
    'fa-gear': makeLocalIcon(`
        <circle cx="12" cy="12" r="3.5" fill="none"/>
        <path fill="none" d="M12 2v3M12 19v3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M2 12h3M19 12h3M4.9 19.1 7 17M17 7l2.1-2.1"/>
    `),
    'fa-battery-three-quarters': makeLocalIcon(`
        <rect x="2.5" y="7" width="18" height="10" rx="2" fill="none"/>
        <path fill="none" d="M22 10v4"/>
        <path fill="none" d="M6 10v4M10 10v4M14 10v4"/>
    `),
    'fa-lightbulb': makeLocalIcon(`
        <path fill="none" d="M8 14a5 5 0 1 1 8 0c-.7.7-1.2 1.5-1.5 2.5h-5C9.2 15.5 8.7 14.7 8 14z"/>
        <path fill="none" d="M9 18h6M10 21h4"/>
    `),
    'fa-wind': makeLocalIcon(`
        <path fill="none" d="M4 9h10a2.5 2.5 0 1 0-2.2-3.7"/>
        <path fill="none" d="M3 13h15a2.5 2.5 0 1 1-2.2 3.7"/>
        <path fill="none" d="M5 17h8"/>
    `),
    'fa-filter': makeLocalIcon(`
        <path fill="none" d="M4 5h16l-6 7v5l-4 2v-7L4 5z"/>
    `),
    'fa-fish': makeLocalIcon(`
        <path fill="none" d="M3 12c3-4 7-5 11-4l4-3v4l3 3-3 3v4l-4-3c-4 1-8 0-11-4z"/>
        <circle cx="9" cy="10.5" r="1" fill="currentColor" stroke="none"/>
    `),
    'fa-download': makeLocalIcon(`
        <path fill="none" d="M12 4v11"/>
        <path fill="none" d="m7 11 5 5 5-5"/>
        <path fill="none" d="M4 20h16"/>
    `),
    'fa-file-arrow-up': makeLocalIcon(`
        <path fill="none" d="M14 2H7a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V7z"/>
        <path fill="none" d="M14 2v5h5"/>
        <path fill="none" d="M12 17V10"/>
        <path fill="none" d="m9.5 12.5 2.5-2.5 2.5 2.5"/>
    `),
    'fa-circle-notch': makeLocalIcon(`
        <path fill="none" d="M20 12a8 8 0 1 1-3.2-6.4"/>
    `),
    'fa-triangle-exclamation': makeLocalIcon(`
        <path fill="none" d="M12 4 3.5 19h17L12 4z"/>
        <path fill="none" d="M12 9.5v4.5"/>
        <circle cx="12" cy="17" r="1" fill="currentColor" stroke="none"/>
    `),
    'fa-microchip': makeLocalIcon(`
        <rect x="7" y="7" width="10" height="10" rx="1.5" fill="none"/>
        <path fill="none" d="M9 2v3M15 2v3M9 19v3M15 19v3M2 9h3M2 15h3M19 9h3M19 15h3"/>
    `),
    'fa-wifi': makeLocalIcon(`
        <path fill="none" d="M5 10a12 12 0 0 1 14 0"/>
        <path fill="none" d="M8 13a7 7 0 0 1 8 0"/>
        <path fill="none" d="M11 16a3 3 0 0 1 2 0"/>
        <circle cx="12" cy="19" r="1" fill="currentColor" stroke="none"/>
    `),
    'fa-satellite-dish': makeLocalIcon(`
        <path fill="none" d="M4 20a10 10 0 0 1 10-10"/>
        <path fill="none" d="M7 17a6 6 0 0 1 6-6"/>
        <circle cx="18" cy="6" r="2" fill="none"/>
        <path fill="none" d="M11 20l2-6 6-2"/>
    `),
    'fa-bluetooth-b': makeLocalIcon(`
        <path fill="none" d="M9 4v16l7-6-5-4 5-4-7-6z"/>
        <path fill="none" d="M9 12 16 6"/>
        <path fill="none" d="M9 12 16 18"/>
    `),
    'fa-temperature-half': makeLocalIcon(`
        <path fill="none" d="M14 14.8V5a2 2 0 0 0-4 0v9.8a4 4 0 1 0 4 0Z"/>
        <path fill="none" d="M12 11v5"/>
    `),
    'fa-floppy-disk': makeLocalIcon(`
        <path fill="none" d="M5 3h11l3 3v15H5z"/>
        <path fill="none" d="M8 3v6h8V3"/>
        <path fill="none" d="M9 18h6"/>
    `),
    'fa-circle-info': makeLocalIcon(`
        <circle cx="12" cy="12" r="9" fill="none"/>
        <path fill="none" d="M12 11.5v4.5"/>
        <circle cx="12" cy="8" r="1" fill="currentColor" stroke="none"/>
    `),
    'fa-clock': makeLocalIcon(`
        <circle cx="12" cy="12" r="9" fill="none"/>
        <path fill="none" d="M12 7v5l3 2"/>
    `),
    'fa-arrows-rotate': makeLocalIcon(`
        <path fill="none" d="M4 12a8 8 0 0 1 13.7-5.7"/>
        <path fill="none" d="M18 3v5h-5"/>
        <path fill="none" d="M20 12a8 8 0 0 1-13.7 5.7"/>
        <path fill="none" d="M6 21v-5h5"/>
    `),
    'fa-laptop': makeLocalIcon(`
        <rect x="5" y="5" width="14" height="10" rx="1.5" fill="none"/>
        <path fill="none" d="M3 19h18"/>
    `),
    'fa-rotate-right': makeLocalIcon(`
        <path fill="none" d="M20 12a8 8 0 1 1-2.3-5.7"/>
        <path fill="none" d="M20 4v6h-6"/>
    `),
    'fa-spinner': makeLocalIcon(`
        <path fill="none" d="M20 12a8 8 0 1 1-3.2-6.4"/>
    `),
    'fa-check-circle': makeLocalIcon(`
        <circle cx="12" cy="12" r="9" fill="none"/>
        <path fill="none" d="m8.5 12.5 2.2 2.2 4.8-5.2"/>
    `)
};

function renderLocalIcon(el) {
    if (!el) return;
    const iconClass = Array.from(el.classList).find((cls) => LOCAL_ICON_SVGS[cls]);
    if (!iconClass) return;
    el.innerHTML = LOCAL_ICON_SVGS[iconClass];
    el.setAttribute('aria-hidden', 'true');
}

function initLocalIcons() {
    document.querySelectorAll('i[class*="fa-"]').forEach(renderLocalIcon);
}

function getLocalIconMarkup(iconClass, extraClass = '') {
    const svg = LOCAL_ICON_SVGS[iconClass] || '';
    const className = extraClass ? `inline-icon ${extraClass}` : 'inline-icon';
    return `<span class="${className}" data-icon-name="${iconClass}" aria-hidden="true">${svg}</span>`;
}

function escapeHtml(value) {
    return String(value ?? '')
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#39;');
}

function toFiniteNumber(value) {
    const num = Number(value);
    return Number.isFinite(num) ? num : null;
}

function isValidTemperature(value) {
    const num = toFiniteNumber(value);
    return num !== null && num > -50 && num < 100;
}

function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
}

function formatTwoDigits(value) {
    const num = Math.max(0, Math.trunc(Number(value) || 0));
    return String(num).padStart(2, '0');
}

function formatTime(hour, minute) {
    return `${formatTwoDigits(hour)}:${formatTwoDigits(minute)}`;
}

function formatRange(startHour, startMinute, endHour, endMinute) {
    return `${formatTime(startHour, startMinute)} - ${formatTime(endHour, endMinute)}`;
}

function formatTemperature(value, digits = 1, fallback = '--.-°C') {
    if (!isValidTemperature(value)) {
        return fallback;
    }
    return `${Number(value).toFixed(digits)}°C`;
}

function formatEpoch(epoch, options = {}) {
    const { fallback = 'Brak historii', includeDate = true, includeSeconds = false } = options;
    const num = toFiniteNumber(epoch);
    if (num === null || num < 946684800) {
        return fallback;
    }

    const date = new Date(num * 1000);
    if (Number.isNaN(date.getTime())) {
        return fallback;
    }

    const datePart = includeDate
        ? date.toLocaleDateString('pl-PL', { day: '2-digit', month: '2-digit', year: 'numeric' })
        : '';
    const timePart = date.toLocaleTimeString('pl-PL', {
        hour: '2-digit',
        minute: '2-digit',
        second: includeSeconds ? '2-digit' : undefined
    });

    return includeDate ? `${datePart} ${timePart}` : timePart;
}

function formatFeedFrequency(freq) {
    switch (Number(freq)) {
        case 1:
            return 'Codziennie';
        case 2:
            return 'Co 2 dni';
        case 3:
            return 'Co 3 dni';
        default:
            return 'Wyłączone';
    }
}

function syncClockFromController(clock) {
    if (!clock) return;

    const year = Number(clock.year);
    const month = Number(clock.month);
    const day = Number(clock.day);
    const hour = Number(clock.hour);
    const minute = Number(clock.minute);
    const second = Number(clock.second);

    if (
        !Number.isInteger(year) || !Number.isInteger(month) || !Number.isInteger(day) ||
        !Number.isInteger(hour) || !Number.isInteger(minute) || !Number.isInteger(second) ||
        year < 2024 || month < 1 || month > 12 || day < 1 || day > 31
    ) {
        return;
    }

    deviceClockBaseDate = new Date(year, month - 1, day, hour, minute, second, 0);
    deviceClockSyncedAtMs = Date.now();
}

function getCurrentClockDate() {
    if (deviceClockBaseDate) {
        return new Date(deviceClockBaseDate.getTime() + (Date.now() - deviceClockSyncedAtMs));
    }
    return new Date();
}

function formatControllerClock(clock) {
    if (!clock) return '';

    const year = Number(clock.year);
    const month = Number(clock.month);
    const day = Number(clock.day);
    const hour = Number(clock.hour);
    const minute = Number(clock.minute);
    const second = Number(clock.second);

    if (
        !Number.isInteger(year) || !Number.isInteger(month) || !Number.isInteger(day) ||
        !Number.isInteger(hour) || !Number.isInteger(minute) || !Number.isInteger(second) ||
        year < 2024 || month < 1 || month > 12 || day < 1 || day > 31
    ) {
        return '';
    }

    const date = new Date(year, month - 1, day, hour, minute, second, 0);
    if (Number.isNaN(date.getTime())) {
        return '';
    }

    return `${date.toLocaleDateString('pl-PL', { day: '2-digit', month: 'short', year: 'numeric' })}, ${date.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit', second: '2-digit' })}`;
}

function renderSettingsClockPanel(clock) {
    const input = document.getElementById('settings-device-datetime');
    const status = document.getElementById('settings-clock-status');
    if (!input || !status) return;

    const formatted = formatControllerClock(clock);
    if (formatted) {
        input.value = formatted;
        status.textContent = 'Czas urzadzenia pobrany automatycznie ze sterownika.';
        status.style.color = 'var(--accent-cyan)';
    } else {
        input.value = 'Brak poprawnego czasu z urzadzenia.';
        status.textContent = 'Sterownik nie udostepnil jeszcze poprawnego czasu.';
        status.style.color = 'var(--text-muted)';
    }
}

function normalizeFirmwareVersion(version) {
    const raw = (version || '').trim();
    if (!raw) return 'v--';
    return raw.startsWith('v') || raw.startsWith('V') ? raw : `v${raw}`;
}

function renderFirmwareInfo(firmware) {
    const versionText = normalizeFirmwareVersion(firmware?.version);
    const tooltipParts = [
        firmware?.buildRef ? `Build: ${firmware.buildRef}` : '',
        firmware?.buildDate && firmware?.buildTime ? `Kompilacja: ${firmware.buildDate} ${firmware.buildTime}` : '',
        firmware?.idfVersion ? `IDF: ${firmware.idfVersion}` : ''
    ].filter(Boolean);
    const tooltip = tooltipParts.join(' | ');

    ['sidebar-firmware-version', 'system-firmware-version'].forEach((id) => {
        const el = document.getElementById(id);
        if (!el) return;
        el.textContent = versionText;
        el.title = tooltip;
    });
}

function setText(id, value) {
    const el = document.getElementById(id);
    if (el) {
        el.textContent = value;
    }
}

function createSvgEl(tag, attrs = {}) {
    const el = document.createElementNS('http://www.w3.org/2000/svg', tag);
    Object.entries(attrs).forEach(([key, value]) => {
        if (value !== undefined && value !== null) {
            el.setAttribute(key, String(value));
        }
    });
    return el;
}

// Clock Logic
function updateClock() {
    const now = getCurrentClockDate();
    
    const timeEl = document.getElementById('current-time');
    const dateEl = document.getElementById('current-date');
    
    if (timeEl && dateEl) {
        timeEl.textContent = now.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        dateEl.textContent = now.toLocaleDateString('pl-PL', { day: '2-digit', month: 'short', year: 'numeric' });
    }
}

function setBackendState(isConnected) {
    backendConnected = isConnected;
    const statusEl = document.getElementById('logs-status');
    if (statusEl) {
        statusEl.textContent = isConnected ? 'Połączono z backendem ESP32.' : 'Brak odpowiedzi sterownika.';
    }
}

function createLogRow(level, text) {
    return `
        <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 14px 20px; display: flex; align-items: center; font-size: 13px;">
            <span style="color: ${level === 'CRITICAL' ? '#ef4444' : 'var(--accent-cyan)'}; font-weight: 600; width: 80px;">${level}</span>
            <span style="color: var(--text-muted); width: 100px;">${new Date().toLocaleTimeString('pl-PL')}</span>
            <span style="color: var(--text-main);">${text}</span>
        </div>`;
}

function renderLogs() {
    const list = document.getElementById('logs-list');
    const infoCount = document.getElementById('info-count');
    const criticalCount = document.getElementById('critical-count');
    const searchInput = document.getElementById('logs-search');
    if (!list) return;

    const query = (searchInput?.value || '').trim().toLowerCase();
    const source = activeLogType === 'critical' ? cachedLogs.critical : cachedLogs.normal;
    const filtered = source.filter(item => item.toLowerCase().includes(query));

    if (infoCount) infoCount.textContent = String(cachedLogs.normal.length);
    if (criticalCount) criticalCount.textContent = String(cachedLogs.critical.length);

    if (filtered.length === 0) {
        list.innerHTML = createLogRow('INFO', 'Brak logów dla wybranego filtra.');
        return;
    }

    list.innerHTML = filtered
        .map(item => createLogRow(activeLogType === 'critical' ? 'CRITICAL' : 'INFO', item))
        .join('');
}

function buildActiveBadge(iconClass, label, tone) {
    return `
        <div class="status-badge status-badge-${tone}">
            ${getLocalIconMarkup(iconClass)}
            <span>${escapeHtml(label)}</span>
        </div>`;
}

function buildModuleBadge(iconClass, label, active, activeTone) {
    return buildActiveBadge(iconClass, label, active ? activeTone : 'muted');
}

function renderTopbarActiveModules(data) {
    const container = document.getElementById('topbar-active-list');
    if (!container) return;

    const network = data.network || {};
    const bleActive = !!network.bleActive || !!network.bleAdvertising || !!network.bleConnected;

    container.innerHTML = [
        buildModuleBadge('fa-satellite-dish', 'AP', !!network.apMode, 'success'),
        buildModuleBadge('fa-wifi', 'STA', !!network.staConnected, 'success'),
        buildModuleBadge('fa-bluetooth-b', 'BLE', bleActive, 'blue')
    ].join('');
}

function renderTemperatureCard(temperature) {
    const currentValue = isValidTemperature(temperature?.current)
        ? Number(temperature.current).toFixed(1)
        : '--.-';

    setText('dashboard-temp-current', currentValue);
    setText('dashboard-temp-target', formatTemperature(temperature?.target));
    setText('dashboard-temp-hysteresis', formatTemperature(temperature?.hysteresis));
}

function renderBatteryWidgets(battery) {
    const voltage = toFiniteNumber(battery?.voltage);
    const percentRaw = toFiniteNumber(battery?.percent);
    const percent = percentRaw === null ? null : clamp(Math.round(percentRaw), 0, 100);
    const voltageText = voltage === null ? '--.--V' : `${voltage.toFixed(2)}V`;

    setText('rtc-battery', voltageText);
    setText('dashboard-battery-voltage', voltageText);
    setText('dashboard-battery-percent', percent === null ? '--' : String(percent));

    const fill = document.getElementById('dashboard-battery-fill');
    if (fill) {
        fill.style.width = `${percent === null ? 0 : percent}%`;
        if (percent !== null && percent <= 15) {
            fill.style.background = 'linear-gradient(90deg, #dc2626, #f87171)';
        } else if (percent !== null && percent <= 35) {
            fill.style.background = 'linear-gradient(90deg, #f59e0b, #fbbf24)';
        } else {
            fill.style.background = 'linear-gradient(90deg, #10b981, #34d399)';
        }
    }

    let stateLabel = 'Brak pomiaru';
    if (percent !== null) {
        if (percent <= 15) {
            stateLabel = 'Niski poziom';
        } else if (percent <= 35) {
            stateLabel = 'Warto obserwować';
        } else {
            stateLabel = 'Poziom stabilny';
        }
    }
    setText('dashboard-battery-state', stateLabel);
}

function renderNetworkCard(network) {
    const card = document.getElementById('network-card');
    if (!card) return;

    const staConnected = !!network?.staConnected;
    const apMode = !!network?.apMode;
    const statusText = staConnected && apMode ? 'AP + STA' : (staConnected ? 'STA ONLINE' : (apMode ? 'TYLKO AP' : 'OFFLINE'));

    setText('network-status', statusText);
    setText('network-ssid', network?.staSsid || '-');
    setText('network-last-seen', formatEpoch(network?.staLastConnectedEpoch, { fallback: 'Brak historii' }));

    card.classList.remove('network-online', 'network-aponly', 'network-offline');
    card.classList.add(staConnected ? 'network-online' : (apMode ? 'network-aponly' : 'network-offline'));
}

function setSettingsNetworkStatus(message, tone = 'muted') {
    const el = document.getElementById('settings-network-status');
    if (!el) return;

    el.textContent = message;
    if (tone === 'success') {
        el.style.color = 'var(--accent-cyan)';
    } else if (tone === 'error') {
        el.style.color = '#ef4444';
    } else {
        el.style.color = 'var(--text-muted)';
    }
}

function setScheduleStatus(message, tone = 'muted') {
    const el = document.getElementById('schedule-save-status');
    if (!el) return;

    el.textContent = message;
    if (tone === 'success') {
        el.style.color = 'var(--accent-cyan)';
    } else if (tone === 'error') {
        el.style.color = '#ef4444';
    } else {
        el.style.color = 'var(--text-muted)';
    }
}

function scheduleModeToUi(value) {
    switch (Number(value)) {
        case 1:
            return 'zawsze_wlaczone';
        case 2:
            return 'zawsze_wylaczone';
        default:
            return 'harmonogram';
    }
}

function scheduleModeToApi(value) {
    switch (value) {
        case 'zawsze_wlaczone':
            return 1;
        case 'zawsze_wylaczone':
            return 2;
        default:
            return 0;
    }
}

function feedFrequencyToUi(value) {
    switch (Number(value)) {
        case 1:
            return 'codziennie';
        case 2:
            return 'co_2_dni';
        case 3:
            return 'co_3_dni';
        default:
            return 'wylaczone';
    }
}

function feedFrequencyToApi(value) {
    switch (value) {
        case 'codziennie':
            return 1;
        case 'co_2_dni':
            return 2;
        case 'co_3_dni':
            return 3;
        default:
            return 0;
    }
}

function clearScheduleDirtyState() {
    document.querySelectorAll('#harmonogramy [data-dirty="1"]').forEach((el) => {
        el.dataset.dirty = '0';
    });
}

function markScheduleDirty() {
    setScheduleStatus('Masz niezapisane zmiany harmonogramow.', 'muted');
}

function setInputValueIfClean(id, value) {
    const el = document.getElementById(id);
    if (!el) return;
    if (document.activeElement === el || el.dataset.dirty === '1') return;
    el.value = value || '';
}

function renderScheduleEditor(data) {
    const schedule = data.schedule || {};
    const feeding = data.feeding || {};

    setInputValueIfClean('schedule-light-mode', scheduleModeToUi(schedule.lightMode));
    setInputValueIfClean('schedule-light-start', formatTime(schedule.dayStartHour, schedule.dayStartMin));
    setInputValueIfClean('schedule-light-end', formatTime(schedule.dayEndHour, schedule.dayEndMin));

    setInputValueIfClean('schedule-air-mode', scheduleModeToUi(schedule.airMode));
    setInputValueIfClean('schedule-air-start', formatTime(schedule.airStartHour, schedule.airStartMin));
    setInputValueIfClean('schedule-air-end', formatTime(schedule.airEndHour, schedule.airEndMin));

    setInputValueIfClean('schedule-filter-mode', scheduleModeToUi(schedule.filterMode));
    setInputValueIfClean('schedule-filter-start', formatTime(schedule.filterStartHour, schedule.filterStartMin));
    setInputValueIfClean('schedule-filter-end', formatTime(schedule.filterEndHour, schedule.filterEndMin));

    setInputValueIfClean('schedule-feed-mode', feedFrequencyToUi(feeding.freq));
    setInputValueIfClean('schedule-feed-time', formatTime(feeding.hour, feeding.minute));

    document.querySelectorAll('#harmonogramy .schedule-item').forEach((item) => {
        const kind = item.getAttribute('data-schedule-kind') || 'point';
        if (kind === 'range') {
            updateRangeScheduleItem(item);
        } else {
            updatePointScheduleItem(item);
        }
    });

    if (!document.querySelector('#harmonogramy [data-dirty="1"]')) {
        setScheduleStatus('Harmonogramy zsynchronizowane ze sterownikiem.', 'success');
    }
}

async function saveScheduleSettings() {
    const button = document.getElementById('save-schedule-btn');
    if (!button) return;

    const payload = {
        lightMode: scheduleModeToApi(document.getElementById('schedule-light-mode')?.value),
        dayStart: document.getElementById('schedule-light-start')?.value || '00:00',
        dayEnd: document.getElementById('schedule-light-end')?.value || '00:00',
        aerationMode: scheduleModeToApi(document.getElementById('schedule-air-mode')?.value),
        airOn: document.getElementById('schedule-air-start')?.value || '00:00',
        airOff: document.getElementById('schedule-air-end')?.value || '00:00',
        filterMode: scheduleModeToApi(document.getElementById('schedule-filter-mode')?.value),
        filterOn: document.getElementById('schedule-filter-start')?.value || '00:00',
        filterOff: document.getElementById('schedule-filter-end')?.value || '00:00',
        feedFreq: feedFrequencyToApi(document.getElementById('schedule-feed-mode')?.value),
        feedTime: document.getElementById('schedule-feed-time')?.value || '00:00'
    };

    button.disabled = true;
    setScheduleStatus('Zapisywanie harmonogramow...', 'muted');

    try {
        const response = await sendAction('save_schedule', payload);
        clearScheduleDirtyState();
        setScheduleStatus(
            response?.code === 'settings_partial'
                ? 'Zapisano harmonogramy po korekcie niektorych wartosci.'
                : 'Zapisano harmonogramy.',
            response?.code === 'settings_partial' ? 'warning' : 'success'
        );
        await fetchStatus(true);
    } catch (error) {
        setScheduleStatus(`Blad zapisu harmonogramow: ${error?.message || 'nieznany blad'}`, 'error');
    } finally {
        button.disabled = false;
    }
}

function renderSettingsNetworkPanel(network) {
    setInputValueIfClean('settings-active-sta-ssid', network?.staSsid || '-');
    setInputValueIfClean('settings-network-ip', network?.ip || '-');
    setInputValueIfClean('settings-sta-ssid', network?.configuredStaSsid || '');
    setInputValueIfClean('settings-ap-ssid', network?.configuredApSsid || '');
}

async function saveNetworkSettings() {
    const staSsidInput = document.getElementById('settings-sta-ssid');
    const staPasswordInput = document.getElementById('settings-sta-password');
    const apSsidInput = document.getElementById('settings-ap-ssid');
    const apPasswordInput = document.getElementById('settings-ap-password');
    const button = document.getElementById('save-network-btn');
    if (!staSsidInput || !staPasswordInput || !apSsidInput || !apPasswordInput || !button) return;

    const payload = {
        staSsid: (staSsidInput.value || '').trim(),
        apSsid: (apSsidInput.value || '').trim()
    };

    const staPassword = (staPasswordInput.value || '').trim();
    const apPassword = (apPasswordInput.value || '').trim();
    if (staPassword) payload.staPassword = staPassword;
    if (apPassword) payload.apPassword = apPassword;

    button.disabled = true;
    setSettingsNetworkStatus('Zapisywanie ustawien WiFi...', 'muted');

    try {
        await sendAction('save_network', payload);
        ['settings-sta-ssid', 'settings-sta-password', 'settings-ap-ssid', 'settings-ap-password'].forEach((id) => {
            const el = document.getElementById(id);
            if (el) el.dataset.dirty = '0';
        });
        staPasswordInput.value = '';
        apPasswordInput.value = '';
        setSettingsNetworkStatus('Zapisano. Nowe SSID/hasla beda uzyte przy kolejnej sesji WiFi.', 'success');
        await fetchStatus(true);
    } catch (error) {
        setSettingsNetworkStatus(`Blad zapisu WiFi: ${error?.message || 'nieznany blad'}`, 'error');
    } finally {
        button.disabled = false;
    }
}

function modeValue(value) {
    const num = Number(value);
    return Number.isFinite(num) ? num : 0;
}

function setRelayCard(relayId, state, meta) {
    const card = document.getElementById(`relay-${relayId}`);
    const stateEl = document.getElementById(`relay-${relayId}-state`);
    const metaEl = document.getElementById(`relay-${relayId}-meta`);
    if (!card || !stateEl || !metaEl) return;

    card.classList.remove('relay-on', 'relay-off', 'relay-standby');
    card.classList.add(`relay-${state}`);
    stateEl.textContent = state.toUpperCase();
    metaEl.textContent = meta;
}

function renderRelays(data) {
    const schedule = data.schedule || {};
    const relays = data.relays || {};
    const aerationPercent = clamp(Number(relays.aerationPercent || 0), 0, 100);

    const lightMode = modeValue(schedule.lightMode);
    const filterMode = modeValue(schedule.filterMode);
    const airMode = modeValue(schedule.airMode);
    const heaterMode = modeValue(schedule.heaterMode);

    const lightState = relays.light ? 'on' : (lightMode === 2 ? 'off' : 'standby');
    const filterState = relays.pump ? 'on' : (filterMode === 2 ? 'off' : 'standby');
    const heaterState = relays.heater ? 'on' : (heaterMode === 1 ? 'off' : 'standby');
    const aerationState = aerationPercent > 0 ? 'on' : (airMode === 2 ? 'off' : 'standby');

    setRelayCard(
        'light',
        lightState,
        lightMode === 1 ? 'Zawsze włączone' : (lightMode === 2 ? 'Ręcznie wyłączone' : formatRange(schedule.dayStartHour, schedule.dayStartMin, schedule.dayEndHour, schedule.dayEndMin))
    );
    setRelayCard(
        'filter',
        filterState,
        filterMode === 1 ? 'Zawsze włączony' : (filterMode === 2 ? 'Ręcznie wyłączony' : formatRange(schedule.filterStartHour, schedule.filterStartMin, schedule.filterEndHour, schedule.filterEndMin))
    );
    setRelayCard(
        'heater',
        heaterState,
        heaterState === 'on' ? 'Dogrzewanie aktywne' : (heaterMode === 1 ? 'Tryb OFF' : `Cel ${formatTemperature(data.temperature?.target)}`)
    );
    setRelayCard(
        'aeration',
        aerationState,
        aerationState === 'on' ? `Otwarcie ${aerationPercent}%` : (airMode === 1 ? 'Zawsze aktywne' : (airMode === 2 ? 'Ręcznie zamknięte' : formatRange(schedule.airStartHour, schedule.airStartMin, schedule.airEndHour, schedule.airEndMin)))
    );

    const activeCount = [lightState, filterState, heaterState, aerationState].filter((state) => state === 'on').length;
    setText('relay-count', `${activeCount} / 4 aktywne`);
}

function describeFeedSchedule(feeding) {
    const freqLabel = formatFeedFrequency(feeding?.freq);
    if (freqLabel === 'Wyłączone') {
        return 'Automatyczne karmienie wyłączone';
    }
    return `${freqLabel} ${formatTime(feeding?.hour, feeding?.minute)}`;
}

function renderTodaySchedule(data) {
    const list = document.getElementById('today-schedule-list');
    if (!list) return;

    const schedule = data.schedule || {};
    const feeding = data.feeding || {};
    const items = [
        {
            label: 'Światło',
            value: modeValue(schedule.lightMode) === 1
                ? 'Zawsze włączone'
                : (modeValue(schedule.lightMode) === 2
                    ? 'Wyłączone'
                    : formatRange(schedule.dayStartHour, schedule.dayStartMin, schedule.dayEndHour, schedule.dayEndMin))
        },
        {
            label: 'Filtr',
            value: modeValue(schedule.filterMode) === 1
                ? 'Zawsze włączony'
                : (modeValue(schedule.filterMode) === 2
                    ? 'Wyłączony'
                    : formatRange(schedule.filterStartHour, schedule.filterStartMin, schedule.filterEndHour, schedule.filterEndMin))
        },
        {
            label: 'Napowietrzanie',
            value: modeValue(schedule.airMode) === 1
                ? 'Zawsze aktywne'
                : (modeValue(schedule.airMode) === 2
                    ? 'Wyłączone'
                    : formatRange(schedule.airStartHour, schedule.airStartMin, schedule.airEndHour, schedule.airEndMin))
        },
        {
            label: 'Karmienie',
            value: describeFeedSchedule(feeding)
        }
    ];

    list.innerHTML = items.map((item) => `
        <div class="schedule-summary-item">
            <span>${escapeHtml(item.label)}</span>
            <strong>${escapeHtml(item.value)}</strong>
        </div>`).join('');
}

function renderFeederCard(data) {
    const feeding = data.feeding || {};
    setText('feed-next-label', describeFeedSchedule(feeding));

    const lastFeedText = formatEpoch(feeding.lastFeedEpoch, {
        fallback: 'brak danych',
        includeDate: true,
        includeSeconds: false
    });
    setText('feed-last-label', `Ostatnie karmienie: ${lastFeedText}`);

    const button = document.getElementById('feed-now-btn');
    if (button) {
        button.disabled = !!feeding.active;
        button.textContent = feeding.active ? 'Trwa...' : 'Karm teraz';
    }
}

function showChartTooltip(clientX, clientY, point) {
    const tooltip = document.getElementById('temperature-chart-tooltip');
    const shell = document.querySelector('.temp-chart-shell');
    if (!tooltip || !shell) return;

    tooltip.innerHTML = `<strong>${escapeHtml(point.valueLabel)}</strong><span>${escapeHtml(point.timeLabel)}</span>`;
    tooltip.hidden = false;

    const rect = shell.getBoundingClientRect();
    const width = tooltip.offsetWidth || 148;
    const height = tooltip.offsetHeight || 56;
    let left = clientX - rect.left - width / 2;
    let top = clientY - rect.top - height - 14;

    left = clamp(left, 10, rect.width - width - 10);
    top = clamp(top, 10, rect.height - height - 10);

    tooltip.style.left = `${left}px`;
    tooltip.style.top = `${top}px`;
}

function hideChartTooltip() {
    const tooltip = document.getElementById('temperature-chart-tooltip');
    if (tooltip) {
        tooltip.hidden = true;
    }
}

function renderTemperatureChart(temperature) {
    const svg = document.getElementById('temperature-chart-svg');
    const empty = document.getElementById('temperature-chart-empty');
    if (!svg || !empty) return;

    const rawHistory = Array.isArray(temperature?.history) ? temperature.history : [];
    const historyCapacity = Math.max(1, Math.round(toFiniteNumber(temperature?.historyCapacity) ?? 20));
    const historyIntervalMinutes = Math.max(1, Math.round(toFiniteNumber(temperature?.historyIntervalMinutes) ?? 10));
    const points = rawHistory
        .map((item, index) => ({
            value: toFiniteNumber(item?.value),
            epoch: toFiniteNumber(item?.epoch),
            index
        }))
        .filter((item) => isValidTemperature(item.value))
        .slice(-historyCapacity)
        .map((item, index, arr) => ({
            value: item.value,
            epoch: item.epoch,
            valueLabel: `${Number(item.value).toFixed(2)}°C`,
            timeLabel: formatEpoch(item.epoch, {
                fallback: `Pomiar ${index + 1} z ${arr.length}`,
                includeDate: false,
                includeSeconds: false
            })
        }));

    setText('temperature-chart-meta', `${points.length || 0} / ${historyCapacity} pomiarow co ${historyIntervalMinutes} min`);
    setText('temperature-chart-start', points.length ? points[0].timeLabel : 'Najstarszy pomiar');
    setText('temperature-chart-end', points.length ? points[points.length - 1].timeLabel : 'Teraz');
    svg.innerHTML = '';
    hideChartTooltip();

    if (points.length === 0) {
        empty.hidden = false;
        return;
    }

    empty.hidden = true;

    const width = 960;
    const height = 320;
    const padding = { top: 20, right: 96, bottom: 34, left: 52 };
    const plotWidth = width - padding.left - padding.right;
    const plotHeight = height - padding.top - padding.bottom;
    const target = toFiniteNumber(temperature?.target);
    const hysteresis = Math.abs(toFiniteNumber(temperature?.hysteresis) ?? 0);

    const rangeValues = points.map((point) => point.value);
    if (target !== null) {
        rangeValues.push(target);
        if (hysteresis > 0) {
            rangeValues.push(target + hysteresis, target - hysteresis);
        }
    }

    let minValue = Math.min(...rangeValues);
    let maxValue = Math.max(...rangeValues);
    if (!Number.isFinite(minValue) || !Number.isFinite(maxValue)) {
        minValue = 20;
        maxValue = 30;
    }
    if (Math.abs(maxValue - minValue) < 0.8) {
        maxValue += 0.4;
        minValue -= 0.4;
    }
    const pad = Math.max(0.2, (maxValue - minValue) * 0.12);
    maxValue += pad;
    minValue -= pad;

    const xFor = (index) => padding.left + (points.length === 1 ? plotWidth / 2 : (index / (points.length - 1)) * plotWidth);
    const yFor = (value) => padding.top + ((maxValue - value) / (maxValue - minValue)) * plotHeight;

    const gridSteps = 4;
    for (let i = 0; i <= gridSteps; i += 1) {
        const ratio = i / gridSteps;
        const y = padding.top + ratio * plotHeight;
        const value = maxValue - ratio * (maxValue - minValue);
        svg.appendChild(createSvgEl('line', {
            x1: padding.left,
            y1: y,
            x2: padding.left + plotWidth,
            y2: y,
            class: 'chart-grid-line'
        }));
        const label = createSvgEl('text', {
            x: 6,
            y: y + 4,
            class: 'chart-grid-label'
        });
        label.textContent = `${value.toFixed(1)}°C`;
        svg.appendChild(label);
    }

    if (target !== null) {
        const targetY = yFor(target);
        svg.appendChild(createSvgEl('line', {
            x1: padding.left,
            y1: targetY,
            x2: padding.left + plotWidth,
            y2: targetY,
            class: 'chart-target-line'
        }));
        const targetLabel = createSvgEl('text', {
            x: padding.left + plotWidth + 10,
            y: targetY + 4,
            class: 'chart-guide-label'
        });
        targetLabel.textContent = `Cel ${target.toFixed(1)}°C`;
        svg.appendChild(targetLabel);

        if (hysteresis > 0) {
            const upper = target + hysteresis;
            const lower = target - hysteresis;
            [upper, lower].forEach((value, index) => {
                const lineY = yFor(value);
                svg.appendChild(createSvgEl('line', {
                    x1: padding.left,
                    y1: lineY,
                    x2: padding.left + plotWidth,
                    y2: lineY,
                    class: 'chart-hysteresis-line'
                }));
                const guide = createSvgEl('text', {
                    x: padding.left + plotWidth + 10,
                    y: lineY + 4,
                    class: 'chart-guide-label hysteresis'
                });
                guide.textContent = `${index === 0 ? 'H +' : 'H -'} ${value.toFixed(1)}°C`;
                svg.appendChild(guide);
            });
        }
    }

    const defs = createSvgEl('defs');
    const gradient = createSvgEl('linearGradient', {
        id: 'chart-area-gradient',
        x1: '0%',
        y1: '0%',
        x2: '0%',
        y2: '100%'
    });
    const startStop = createSvgEl('stop', { offset: '0%', 'stop-color': '#22d3ee', 'stop-opacity': '0.28' });
    const endStop = createSvgEl('stop', { offset: '100%', 'stop-color': '#22d3ee', 'stop-opacity': '0.02' });
    gradient.appendChild(startStop);
    gradient.appendChild(endStop);
    defs.appendChild(gradient);
    svg.appendChild(defs);

    const pathData = points.map((point, index) => {
        const x = xFor(index);
        const y = yFor(point.value);
        return `${index === 0 ? 'M' : 'L'} ${x.toFixed(2)} ${y.toFixed(2)}`;
    }).join(' ');
    const areaData = `${pathData} L ${xFor(points.length - 1).toFixed(2)} ${(padding.top + plotHeight).toFixed(2)} L ${xFor(0).toFixed(2)} ${(padding.top + plotHeight).toFixed(2)} Z`;
    svg.appendChild(createSvgEl('path', { d: areaData, class: 'chart-area' }));
    svg.appendChild(createSvgEl('path', { d: pathData, class: 'chart-line' }));

    points.forEach((point, index) => {
        const x = xFor(index);
        const y = yFor(point.value);

        const circle = createSvgEl('circle', {
            cx: x,
            cy: y,
            r: 5,
            class: 'chart-point'
        });
        const title = createSvgEl('title');
        title.textContent = `${point.valueLabel} - ${point.timeLabel}`;
        circle.appendChild(title);
        svg.appendChild(circle);

        const hit = createSvgEl('circle', {
            cx: x,
            cy: y,
            r: 13,
            tabindex: 0,
            class: 'chart-point-hit'
        });
        hit.addEventListener('mouseenter', (event) => showChartTooltip(event.clientX, event.clientY, point));
        hit.addEventListener('mousemove', (event) => showChartTooltip(event.clientX, event.clientY, point));
        hit.addEventListener('mouseleave', hideChartTooltip);
        hit.addEventListener('focus', () => {
            const rect = svg.getBoundingClientRect();
            showChartTooltip(rect.left + x, rect.top + y, point);
        });
        hit.addEventListener('blur', hideChartTooltip);
        svg.appendChild(hit);
    });
}

function renderDashboard(data) {
    lastStatusData = data;
    renderTopbarActiveModules(data);
    renderTemperatureCard(data.temperature || {});
    renderBatteryWidgets(data.battery || {});
    renderFirmwareInfo(data.firmware || {});
    renderNetworkCard(data.network || {});
    renderSettingsNetworkPanel(data.network || {});
    renderSettingsClockPanel(data.clock || {});
    renderScheduleEditor(data);
    renderRelays(data);
    renderTodaySchedule(data);
    renderFeederCard(data);
    renderTemperatureChart(data.temperature || {});
}

function applyStatusPayload(data) {
    if (!data || typeof data !== 'object') {
        return;
    }
    setBackendState(true);
    syncClockFromController(data.clock);
    renderDashboard(data);
}

function applyLogsPayload(logs) {
    cachedLogs.normal = Array.isArray(logs?.normal) ? logs.normal : [];
    cachedLogs.critical = Array.isArray(logs?.critical) ? logs.critical : [];
    renderLogs();
}

async function fetchStatus(force = false) {
    if (eventStreamConnected && !force) {
        return;
    }

    try {
        const response = await fetch(API_STATUS, { cache: 'no-store' });
        if (!response.ok) throw new Error('status http');
        const data = await response.json();
        applyStatusPayload(data);
    } catch (e) {
        if (!eventStreamConnected) {
            setBackendState(false);
        }
    }
}

async function fetchLogs(force = false) {
    if (eventStreamConnected && !force) {
        return;
    }

    try {
        const response = await fetch(API_LOGS, { cache: 'no-store' });
        if (!response.ok) throw new Error('logs http');
        const logs = await response.json();
        applyLogsPayload(logs);
    } catch (e) {
        // keep last logs
    }
}

function stopPollingFallback() {
    if (statusPollingTimer) {
        clearInterval(statusPollingTimer);
        statusPollingTimer = null;
    }
    if (logsPollingTimer) {
        clearInterval(logsPollingTimer);
        logsPollingTimer = null;
    }
}

function startPollingFallback() {
    if (!statusPollingTimer) {
        statusPollingTimer = setInterval(() => fetchStatus(false), 3000);
    }
    if (!logsPollingTimer) {
        logsPollingTimer = setInterval(() => fetchLogs(false), 5000);
    }
}

function startEventStream() {
    if (eventStream || !('EventSource' in window)) {
        startPollingFallback();
        return;
    }

    eventStream = new EventSource(API_EVENTS);

    eventStream.onopen = () => {
        eventStreamConnected = true;
        setBackendState(true);
        stopPollingFallback();
    };

    eventStream.addEventListener('ready', () => {
        eventStreamConnected = true;
        setBackendState(true);
        stopPollingFallback();
    });

    eventStream.addEventListener('status', (event) => {
        try {
            const payload = JSON.parse(event.data);
            eventStreamConnected = true;
            applyStatusPayload(payload);
            stopPollingFallback();
        } catch (error) {
            console.warn('Nie udalo sie sparsowac zdarzenia status SSE.', error);
        }
    });

    eventStream.addEventListener('logs', (event) => {
        try {
            const payload = JSON.parse(event.data);
            eventStreamConnected = true;
            applyLogsPayload(payload);
            stopPollingFallback();
        } catch (error) {
            console.warn('Nie udalo sie sparsowac zdarzenia logs SSE.', error);
        }
    });

    eventStream.onerror = () => {
        eventStreamConnected = false;
        startPollingFallback();
    };
}

function shouldShowOledSaveAnimation(action) {
    return OLED_SAVE_ACTIONS.has(action);
}

function showOledSaveAnimation(action) {
    const overlay = document.getElementById('oled-save-overlay');
    const title = document.getElementById('oled-save-title');
    const subtext = document.getElementById('oled-save-subtext');
    if (!overlay || !title || !subtext) {
        return false;
    }

    if (oledSaveOverlayHideTimer) {
        clearTimeout(oledSaveOverlayHideTimer);
        oledSaveOverlayHideTimer = null;
    }

    oledSaveOverlayActiveCount += 1;
    title.textContent = OLED_SAVE_TITLES[action] || 'Zapisywanie danych';
    subtext.textContent = 'Sterownik synchronizuje konfiguracje i pokazuje zapis na OLED.';

    if (oledSaveOverlayActiveCount === 1) {
        overlay.style.display = 'flex';
        overlay.setAttribute('aria-hidden', 'false');
        oledSaveOverlayShownAtMs = Date.now();
    }

    return true;
}

function hideOledSaveAnimation() {
    const overlay = document.getElementById('oled-save-overlay');
    if (oledSaveOverlayActiveCount > 0) {
        oledSaveOverlayActiveCount -= 1;
    }

    if (oledSaveOverlayActiveCount > 0 || !overlay) {
        return;
    }

    const elapsedMs = Date.now() - oledSaveOverlayShownAtMs;
    const delayMs = Math.max(0, 900 - elapsedMs);
    oledSaveOverlayHideTimer = setTimeout(() => {
        overlay.style.display = 'none';
        overlay.setAttribute('aria-hidden', 'true');
        oledSaveOverlayHideTimer = null;
    }, delayMs);
}

async function sendAction(action, payload = {}, options = {}) {
    const showSaveAnimation = options.showSaveAnimation ?? shouldShowOledSaveAnimation(action);
    if (showSaveAnimation) {
        showOledSaveAnimation(action);
    }

    try {
        const params = new URLSearchParams({ action, ...payload });
        const response = await fetch(API_ACTION, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: params.toString()
        });

        const contentType = response.headers.get('content-type') || '';
        let responsePayload;

        if (contentType.includes('application/json')) {
            responsePayload = await response.json();
        } else {
            const responseText = await response.text();
            responsePayload = {
                success: response.ok,
                code: responseText || (response.ok ? 'ok' : 'request_failed'),
                message: responseText || ''
            };
        }

        if (!response.ok || responsePayload?.success === false) {
            throw new Error(responsePayload?.message || responsePayload?.code || 'request_failed');
        }

        return responsePayload;
    } finally {
        if (showSaveAnimation) {
            hideOledSaveAnimation();
        }
    }
}

function timeToMinutes(time) {
    const [hours, minutes] = (time || '00:00').split(':').map(Number);
    return ((Number.isFinite(hours) ? hours : 0) * 60) + (Number.isFinite(minutes) ? minutes : 0);
}

function minutesToPercent(totalMinutes) {
    return Math.max(0, Math.min(100, (totalMinutes / (24 * 60)) * 100));
}

function updateRangeScheduleItem(item) {
    const modeSelect = item.querySelector('.schedule-mode-select');
    const startInput = item.querySelector('.schedule-time-start');
    const endInput = item.querySelector('.schedule-time-end');
    const bar = item.querySelector('.schedule-bar');
    if (!modeSelect || !startInput || !endInput || !bar) return;

    const mode = modeSelect.value;
    const startMinutes = timeToMinutes(startInput.value);
    let endMinutes = timeToMinutes(endInput.value);
    if (endMinutes < startMinutes) {
        endMinutes = startMinutes;
        endInput.value = startInput.value;
    }

    startInput.disabled = mode !== 'harmonogram';
    endInput.disabled = mode !== 'harmonogram';

    const startPct = minutesToPercent(startMinutes);
    let endPct = minutesToPercent(endMinutes);

    if (mode === 'zawsze_wlaczone') {
        endPct = 100;
        bar.style.left = '0%';
        bar.style.width = '100%';
    } else if (mode === 'zawsze_wylaczone') {
        bar.style.left = '0%';
        bar.style.width = '0%';
    } else {
        bar.style.left = `${startPct}%`;
        bar.style.width = `${Math.max(0, endPct - startPct)}%`;
    }

    startInput.style.left = `${startPct}%`;
    endInput.style.left = `${endPct}%`;
}

function updatePointScheduleItem(item) {
    const pointInput = item.querySelector('.schedule-time-point');
    const point = item.querySelector('.schedule-point');
    if (!pointInput || !point) return;

    const pointPct = minutesToPercent(timeToMinutes(pointInput.value));
    point.style.left = `${pointPct}%`;
    pointInput.style.left = `${pointPct}%`;
}

function initScheduleTimeline() {
    const scheduleItems = document.querySelectorAll('#harmonogramy .schedule-item');
    scheduleItems.forEach(item => {
        const kind = item.getAttribute('data-schedule-kind') || 'point';
        if (kind === 'range') {
            const modeSelect = item.querySelector('.schedule-mode-select');
            const startInput = item.querySelector('.schedule-time-start');
            const endInput = item.querySelector('.schedule-time-end');
            const onRangeChange = () => {
                [modeSelect, startInput, endInput].forEach((el) => {
                    if (el) el.dataset.dirty = '1';
                });
                updateRangeScheduleItem(item);
                markScheduleDirty();
            };
            modeSelect?.addEventListener('change', onRangeChange);
            startInput?.addEventListener('input', onRangeChange);
            endInput?.addEventListener('input', onRangeChange);
            updateRangeScheduleItem(item);
        } else {
            const feedModeSelect = item.querySelector('select');
            const pointInput = item.querySelector('.schedule-time-point');
            const onPointChange = () => {
                [feedModeSelect, pointInput].forEach((el) => {
                    if (el) el.dataset.dirty = '1';
                });
                updatePointScheduleItem(item);
                markScheduleDirty();
            };
            feedModeSelect?.addEventListener('change', onPointChange);
            pointInput?.addEventListener('input', onPointChange);
            updatePointScheduleItem(item);
        }
    });
}

// Tab Switching Logic
function initNavigation() {
    const navItems = document.querySelectorAll('.nav-item[data-target]');
    
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const targetId = item.getAttribute('data-target');
            if(targetId) {
                switchTab(targetId);
            }
        });
    });
}

function switchTab(tabId) {
    // 1. Remove active from all nav items
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(nav => nav.classList.remove('active'));

    // 2. Add active to clicked nav item
    const activeNav = document.querySelector(`.nav-item[data-target="${tabId}"]`);
    if(activeNav) {
        activeNav.classList.add('active');
    }

    // 3. Hide all view sections
    const sections = document.querySelectorAll('.view-section');
    sections.forEach(sec => sec.classList.remove('active'));

    // 4. Show target section
    const targetSection = document.getElementById(tabId);
    if(targetSection) {
        targetSection.classList.add('active');
    }
}

// Feeder Logic
function triggerFeed() {
    const modal = document.getElementById('feed-modal');
    const icon = document.getElementById('modal-icon');
    const text = document.getElementById('modal-text');
    const p = document.getElementById('modal-subtext');

    if (!modal || !icon || !text || !p) {
        console.warn('Brak wymaganych elementow UI dla triggerFeed().');
        return;
    }

    modal.style.display = 'flex';

    sendAction('feed_now').catch(() => {
        // fallback only to local animation if backend not reachable
    });
    
    // Simulate feeding process
    setTimeout(() => {
        icon.className = 'fa-solid fa-check-circle fa-2xl';
        renderLocalIcon(icon);
        icon.style.color = 'var(--success-color)';
        text.textContent = 'Sukces';
        p.textContent = 'Karmienie zakończone pomyślnie. Status sensora: OK.';
        
        setTimeout(() => {
            modal.style.display = 'none';
            // Reset for next time
            setTimeout(() => {
                icon.className = 'fa-solid fa-spinner fa-spin fa-2xl';
                renderLocalIcon(icon);
                icon.style.color = 'var(--accent-cyan)';
                text.textContent = 'Trwa karmienie...';
                p.textContent = 'Sensor położenia w trakcie odczytu';
            }, 500);
        }, 1500);
    }, 2000);
}

// OTA Logic
function setOtaUploadReadyState(hasValidFile, fileName = '') {
    const uploadBtn = document.getElementById('upload-btn');
    const wrapper = document.getElementById('firmware-upload-wrapper');
    const fileNameLabel = document.getElementById('firmware-file-name');
    if (!uploadBtn) return;

    uploadBtn.disabled = !hasValidFile;
    uploadBtn.textContent = hasValidFile && fileName ? `Aktualizuj System (${fileName})` : 'Aktualizuj System';
    uploadBtn.style.opacity = hasValidFile ? '1' : '0.5';
    uploadBtn.style.filter = hasValidFile ? 'brightness(1.12)' : 'saturate(0.7)';
    uploadBtn.style.boxShadow = hasValidFile ? '0 10px 28px rgba(37, 99, 235, 0.35)' : 'none';

    if (wrapper) {
        wrapper.style.borderColor = hasValidFile ? 'rgba(59, 130, 246, 0.75)' : 'rgba(255,255,255,0.15)';
        wrapper.style.background = hasValidFile ? 'rgba(59, 130, 246, 0.10)' : 'rgba(0,0,0,0.2)';
    }

    if (fileNameLabel) {
        fileNameLabel.textContent = hasValidFile && fileName ? `Wybrany plik: ${fileName}` : 'Nie wybrano pliku .bin';
    }
}

function initOTA() {
    const fileInput = document.getElementById('firmware-file');
    const uploadBtn = document.getElementById('upload-btn');
    
    if(fileInput && uploadBtn) {
        setOtaUploadReadyState(false);
        fileInput.addEventListener('change', (e) => {
            if(e.target.files.length > 0) {
                const file = e.target.files[0];
                if(file.name.endsWith('.bin')) {
                    setOtaUploadReadyState(true, file.name);
                } else {
                    alert('Proszę wybrać poprawny plik firmware z rozszerzeniem .bin');
                    setOtaUploadReadyState(false);
                    e.target.value = '';
                }
            } else {
                setOtaUploadReadyState(false);
            }
        });
    }
}

function simulateOTA() {
    const progressContainer = document.getElementById('ota-progress');
    const fill = document.getElementById('ota-fill');
    const percentTxt = document.getElementById('ota-percent');
    const btn = document.getElementById('upload-btn');
    
    if(!progressContainer || !fill || !percentTxt || !btn) return;

    const firmwareFile = document.getElementById('firmware-file');
    if(!firmwareFile || !firmwareFile.files || firmwareFile.files.length === 0) {
        alert('Najpierw wybierz plik .bin.');
        return;
    }

    const selectedFileName = firmwareFile.files[0]?.name || '';

    const formData = new FormData();
    formData.append('update', firmwareFile.files[0]);

    progressContainer.style.display = 'block';
    btn.disabled = true;

    const xhr = new XMLHttpRequest();
    xhr.open('POST', API_OTA, true);

    xhr.upload.onprogress = function (event) {
        if(!event.lengthComputable) return;
        const progress = Math.min(100, Math.round((event.loaded / event.total) * 100));
        fill.style.width = `${progress}%`;
        percentTxt.textContent = `${progress}%`;
    };

    xhr.onload = function () {
        if (xhr.status >= 200 && xhr.status < 300) {
            btn.textContent = 'Wgrano pakiet OTA';
            btn.style.backgroundColor = 'var(--success-color)';
            
            setTimeout(() => {
                alert('Aktualizacja zakończona pomyślnie. Urządzenie zrestartuje się za chwilę.');
                // Reset UI
                progressContainer.style.display = 'none';
                fill.style.width = '0%';
                percentTxt.textContent = '0%';
                btn.style.backgroundColor = '';
                const firmwareFile = document.getElementById('firmware-file');
                if(firmwareFile) {
                    firmwareFile.value = '';
                }
                setOtaUploadReadyState(false);
            }, 1000);
        }
    };

    xhr.onerror = function () {
        btn.textContent = 'Błąd sieci OTA';
        btn.style.backgroundColor = 'var(--danger-color)';
        alert('Błąd połączenia podczas OTA.');
    };

    xhr.onloadend = function () {
        if (selectedFileName) {
            setOtaUploadReadyState(true, selectedFileName);
        } else {
            setOtaUploadReadyState(false);
        }
    };

    xhr.send(formData);
}

// Init Event Listeners
document.addEventListener('DOMContentLoaded', () => {
    initLocalIcons();
    updateClock();
    setInterval(updateClock, 1000);
    
    initNavigation();
    initOTA();
    initScheduleTimeline();
    ['settings-sta-ssid', 'settings-sta-password', 'settings-ap-ssid', 'settings-ap-password'].forEach((id) => {
        const el = document.getElementById(id);
        el?.addEventListener('input', () => {
            el.dataset.dirty = '1';
        });
    });
    setSettingsNetworkStatus('Zmiany SSID i hasel sa stosowane przy kolejnej sesji WiFi.', 'muted');

    const currentBtn = document.getElementById('logs-current-btn');
    const criticalBtn = document.getElementById('logs-critical-btn');
    const clearBtn = document.getElementById('clear-logs-btn');
    const deleteCriticalBtn = document.getElementById('delete-critical-btn');
    const downloadBtn = document.getElementById('download-logs-btn');
    const searchInput = document.getElementById('logs-search');

    currentBtn?.addEventListener('click', () => {
        activeLogType = 'normal';
        renderLogs();
    });
    criticalBtn?.addEventListener('click', () => {
        activeLogType = 'critical';
        renderLogs();
    });
    clearBtn?.addEventListener('click', () => {
        cachedLogs = { normal: [], critical: [] };
        renderLogs();
    });
    deleteCriticalBtn?.addEventListener('click', async () => {
        try {
            await sendAction('clear_critical_logs');
            await fetchLogs(true);
        } catch (_) {}
    });
    downloadBtn?.addEventListener('click', () => {
        const lines = (activeLogType === 'critical' ? cachedLogs.critical : cachedLogs.normal).join('\n');
        const blob = new Blob([lines], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `akwarium_logs_${Date.now()}.txt`;
        a.click();
        URL.revokeObjectURL(url);
    });
    searchInput?.addEventListener('input', renderLogs);

    startEventStream();
    startPollingFallback();
    fetchStatus(true);
    fetchLogs(true);

    // Mock toggle logic for dashboard toggles
    const toggles = document.querySelectorAll('input[type="checkbox"]');
    toggles.forEach(toggle => {
        toggle.addEventListener('change', (e) => {
            console.log(`${e.target.id} changed to ${e.target.checked}`);
        });
    });
});
