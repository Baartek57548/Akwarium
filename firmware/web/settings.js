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

function formatSleepBlockerLabel(code) {
    switch (code) {
        case 'idle_window':
            return 'idle window';
        case 'outputs_active':
            return 'outputs active';
        case 'not_night':
            return 'not night';
        case 'ota':
            return 'OTA';
        case 'ap_mode':
            return 'AP mode';
        case 'service_mode':
            return 'service mode';
        case 'time_sync':
            return 'time sync';
        case 'sta_active':
            return 'STA active';
        case 'feeding':
            return 'feeding';
        default:
            return code || 'unknown';
    }
}

function renderTemperatureSettingsPanel(data) {
    const schedule = data.schedule || {};
    const temperature = data.temperature || {};

    setCheckboxIfClean('settings-temp-enabled', Number(schedule.heaterMode) !== 1);
    setNumericValueIfClean('settings-temp-target', temperature.target, 0);
    setNumericValueIfClean('settings-temp-hyst', temperature.hysteresis, 1);
}

function renderDiagnosticsPanel(data) {
    const system = data.system || {};
    const network = data.network || {};

    setText('system-uptime', formatDuration(system.uptimeSeconds));
    setText('system-reset-reason', system.resetReason || 'unknown');
    setText('system-reset-count', String(Math.max(0, Math.trunc(Number(system.resetCount) || 0))));
    setText('system-power-mode', system.powerMode || 'unknown');

    const blockers = Array.isArray(system.sleepBlockers) ? system.sleepBlockers : [];
    setText(
        'system-sleep-blockers',
        blockers.length === 0 ? 'ready for light sleep' : blockers.map(formatSleepBlockerLabel).join(', ')
    );

    const lastSyncLabel = network.lastTimeSyncOk
        ? formatEpoch(network.lastTimeSyncEpoch, {
            fallback: '',
            includeDate: true,
            includeSeconds: false
        })
        : '';
    const ntpSummary = network.timeSyncInProgress
        ? 'Synchronizacja trwa...'
        : (network.lastTimeSyncStatus || 'Brak danych');
    setText('system-ntp-status', ntpSummary);
    if (lastSyncLabel) {
        setText('system-ntp-status', `${ntpSummary} / ${lastSyncLabel}`);
    }

    const rssi = toFiniteNumber(network.rssi);
    const clients = Math.max(0, Math.trunc(Number(network.clients) || 0));
    const networkSummary = network.staConnected
        ? `STA ${rssi === null ? '--' : `${Math.round(rssi)} dBm`} / AP clients ${clients}`
        : `STA offline / AP clients ${clients}`;
    setText('system-network-quality', networkSummary);

    let apIdleText = 'AP inactive';
    if (network.apMode) {
        apIdleText = network.apIdleCountdownActive
            ? `Auto-stop za ${formatCountdownMs(network.apIdleRemainingMs)}`
            : 'AP aktywne, klient polaczony';
    }
    setText('system-ap-idle', apIdleText);
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

function setSettingsNetworkStatus(message, tone = 'muted') {
    setInlineStatus('settings-network-status', message, tone);
}

function setScheduleStatus(message, tone = 'muted') {
    setInlineStatus('schedule-save-status', message, tone);
}

function setTemperatureStatus(message, tone = 'muted') {
    setInlineStatus('settings-temp-status', message, tone);
}

function setDeviceActionStatus(message, tone = 'muted') {
    setInlineStatus('device-actions-status', message, tone);
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

async function saveTemperatureSettings() {
    const enabledInput = document.getElementById('settings-temp-enabled');
    const targetInput = document.getElementById('settings-temp-target');
    const hystInput = document.getElementById('settings-temp-hyst');
    const button = document.getElementById('save-temperature-btn');
    if (!enabledInput || !targetInput || !hystInput || !button) return;

    const payload = {
        heaterMode: enabledInput.checked ? '0' : '1',
        targetTemp: String(targetInput.value || ''),
        tempHyst: String(hystInput.value || '')
    };

    button.disabled = true;
    setTemperatureStatus('Zapisywanie ustawien temperatury...', 'muted');

    try {
        const response = await sendAction('save_temperature', payload, { showSaveAnimation: false });
        ['settings-temp-enabled', 'settings-temp-target', 'settings-temp-hyst'].forEach((id) => {
            const el = document.getElementById(id);
            if (el) el.dataset.dirty = '0';
        });
        setTemperatureStatus(
            response?.code === 'settings_partial'
                ? 'Zapisano po korekcie wartosci spoza profilu walidacji.'
                : 'Zapisano ustawienia temperatury.',
            response?.code === 'settings_partial' ? 'warning' : 'success'
        );
        await fetchStatus(true);
    } catch (error) {
        setTemperatureStatus(`Blad ustawien temperatury: ${describeRequestError(error)}`, 'error');
    } finally {
        button.disabled = false;
    }
}

async function syncBrowserTime() {
    const ntpBtn = document.getElementById('sync-ntp-btn');
    const browserBtn = document.getElementById('sync-browser-time-btn');
    const epoch = Math.trunc(Date.now() / 1000);

    ntpBtn && (ntpBtn.disabled = true);
    browserBtn && (browserBtn.disabled = true);
    setInlineStatus('settings-clock-status', 'Wysylam czas z przegladarki do sterownika...', 'muted');

    try {
        const response = await fetch(API_SETTIME, {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: new URLSearchParams({ epoch: String(epoch) }).toString()
        });
        const message = await response.text();
        if (!response.ok) {
            const error = new Error(message || 'request_failed');
            error.code = message || 'request_failed';
            throw error;
        }
        await fetchStatus(true);
        setInlineStatus('settings-clock-status', 'Czas sterownika zsynchronizowany z przegladarka.', 'success');
    } catch (error) {
        setInlineStatus('settings-clock-status', `Blad synchronizacji czasu: ${describeRequestError(error)}`, 'error');
    } finally {
        ntpBtn && (ntpBtn.disabled = false);
        browserBtn && (browserBtn.disabled = false);
    }
}

async function syncTimeWithNtp() {
    const ntpBtn = document.getElementById('sync-ntp-btn');
    const browserBtn = document.getElementById('sync-browser-time-btn');

    ntpBtn && (ntpBtn.disabled = true);
    browserBtn && (browserBtn.disabled = true);
    setInlineStatus('settings-clock-status', 'Uruchamiam reczna synchronizacje NTP...', 'muted');

    try {
        const response = await sendAction('sync_time_ntp', {}, { showSaveAnimation: false });
        await fetchStatus(true);
        setInlineStatus(
            'settings-clock-status',
            response?.message || 'Synchronizacja NTP zakonczona powodzeniem.',
            'success'
        );
    } catch (error) {
        await fetchStatus(true);
        setInlineStatus(
            'settings-clock-status',
            error?.message || `Blad synchronizacji NTP (${error?.code || 'request_failed'}).`,
            error?.code === 'busy' ? 'warning' : 'error'
        );
    } finally {
        ntpBtn && (ntpBtn.disabled = false);
        browserBtn && (browserBtn.disabled = false);
    }
}

async function runDeviceAction(action, confirmationMessage, successMessage) {
    if (!window.confirm(confirmationMessage)) {
        return;
    }

    const restartBtn = document.getElementById('restart-device-btn');
    const resetBtn = document.getElementById('factory-reset-btn');
    let unlockButtons = true;
    restartBtn && (restartBtn.disabled = true);
    resetBtn && (resetBtn.disabled = true);
    setDeviceActionStatus('Wysylam polecenie do sterownika...', 'muted');

    try {
        await sendAction(action, {}, { showSaveAnimation: false });
        setDeviceActionStatus(successMessage, action === 'factory_reset' ? 'warning' : 'success');
        setBackendState(false);
        unlockButtons = false;
    } catch (error) {
        setDeviceActionStatus(`Blad akcji urzadzenia: ${describeRequestError(error)}`, 'error');
    } finally {
        if (unlockButtons) {
            restartBtn && (restartBtn.disabled = false);
            resetBtn && (resetBtn.disabled = false);
        }
    }
}
