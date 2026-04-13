function normalizeFeedResultCode(code) {
    const raw = String(code || '').trim().toLowerCase();
    if (raw.startsWith('feed_')) {
        return raw.slice(5);
    }
    return raw;
}

function describeFeedResult(code, fallbackMessage = '') {
    switch (normalizeFeedResultCode(code)) {
        case 'ok':
            return {
                kind: 'success',
                title: 'Sukces',
                message: 'Karmienie zakonczylo sie pomyslnie.'
            };
        case 'busy':
            return {
                kind: 'error',
                title: 'Karmnik zajety',
                message: 'Poprzednie karmienie nadal trwa. Sprobuj ponownie za chwile.'
            };
        case 'sensor_not_ok':
            return {
                kind: 'error',
                title: 'Blad sensora',
                message: 'Sensor polozenia nie potwierdzil poprawnego cyklu karmienia.'
            };
        case 'timeout':
            return {
                kind: 'error',
                title: 'Timeout karmienia',
                message: 'Sterownik nie otrzymal potwierdzenia z sensora w oczekiwanym czasie.'
            };
        default:
            return {
                kind: 'error',
                title: 'Blad karmienia',
                message: fallbackMessage || 'Nie udalo sie uruchomic karmnika.'
            };
    }
}

function hideFeedModal() {
    const modal = document.getElementById('feed-modal');
    if (feedModalHideTimer) {
        clearTimeout(feedModalHideTimer);
        feedModalHideTimer = null;
    }
    if (modal) {
        modal.style.display = 'none';
    }
}

function showFeedModalState(kind, title, message, autoHideMs = 0) {
    const modal = document.getElementById('feed-modal');
    const icon = document.getElementById('modal-icon');
    const text = document.getElementById('modal-text');
    const subtext = document.getElementById('modal-subtext');

    if (!modal || !icon || !text || !subtext) {
        return;
    }

    if (feedModalHideTimer) {
        clearTimeout(feedModalHideTimer);
        feedModalHideTimer = null;
    }

    modal.style.display = 'flex';
    text.textContent = title;
    subtext.textContent = message;

    if (kind === 'success') {
        icon.className = 'fa-solid fa-check-circle fa-2xl';
        icon.style.color = 'var(--success-color)';
    } else if (kind === 'error') {
        icon.className = 'fa-solid fa-triangle-exclamation fa-2xl';
        icon.style.color = '#ef4444';
    } else {
        icon.className = 'fa-solid fa-spinner fa-spin fa-2xl';
        icon.style.color = 'var(--accent-cyan)';
    }
    renderLocalIcon(icon);

    if (autoHideMs > 0) {
        feedModalHideTimer = setTimeout(() => {
            hideFeedModal();
        }, autoHideMs);
    }
}

function resetFeedActionState() {
    feedActionState.awaitingResponse = false;
    feedActionState.awaitingCompletion = false;
    feedActionState.sawActive = false;
    feedActionState.startedAtMs = 0;
    feedActionState.baselineLastFeedEpoch = 0;
    feedActionState.baselineLastResult = '';
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

    container.innerHTML = [
        buildModuleBadge('fa-satellite-dish', 'AP', !!network.apMode, 'success'),
        buildModuleBadge('fa-wifi', 'STA', !!network.staConnected, 'success')
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
            stateLabel = 'Warto obserwowac';
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

async function toggleRelayQuickAction(kind) {
    const relayMap = {
        light: {
            relayValue: !!lastStatusData?.relays?.light,
            buttonId: 'relay-light-toggle',
            action: 'set_light'
        },
        filter: {
            relayValue: !!lastStatusData?.relays?.pump,
            buttonId: 'relay-filter-toggle',
            action: 'set_filter'
        }
    };

    const config = relayMap[kind];
    if (!config) return;

    const button = document.getElementById(config.buttonId);
    if (!button || button.dataset.busy === '1') return;

    const idleLabel = config.relayValue ? 'Wylacz' : 'Wlacz';
    const desiredState = config.relayValue ? '0' : '1';
    button.dataset.busy = '1';
    button.disabled = true;
    button.textContent = 'Zapisywanie...';

    try {
        await sendAction(config.action, { state: desiredState });
        await fetchStatus(true);
        await fetchLogs(true);
    } catch (error) {
        button.textContent = 'Blad';
        button.title = describeRequestError(error);
        setTimeout(() => {
            button.textContent = idleLabel;
            button.title = '';
        }, 1600);
    } finally {
        button.dataset.busy = '0';
        button.disabled = false;
        if (!button.title) {
            const relayActive = kind === 'light' ? !!lastStatusData?.relays?.light : !!lastStatusData?.relays?.pump;
            button.textContent = relayActive ? 'Wylacz' : 'Wlacz';
        }
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
        lightMode === 1 ? 'Zawsze wlaczone' : (lightMode === 2 ? 'Recznie wylaczone' : formatRange(schedule.dayStartHour, schedule.dayStartMin, schedule.dayEndHour, schedule.dayEndMin))
    );
    setRelayCard(
        'filter',
        filterState,
        filterMode === 1 ? 'Zawsze wlaczony' : (filterMode === 2 ? 'Recznie wylaczony' : formatRange(schedule.filterStartHour, schedule.filterStartMin, schedule.filterEndHour, schedule.filterEndMin))
    );
    setRelayCard(
        'heater',
        heaterState,
        heaterState === 'on' ? 'Dogrzewanie aktywne' : (heaterMode === 1 ? 'Tryb OFF' : `Cel ${formatTemperature(data.temperature?.target)}`)
    );
    setRelayCard(
        'aeration',
        aerationState,
        aerationState === 'on' ? `Otwarcie ${aerationPercent}%` : (airMode === 1 ? 'Zawsze aktywne' : (airMode === 2 ? 'Recznie zamkniete' : formatRange(schedule.airStartHour, schedule.airStartMin, schedule.airEndHour, schedule.airEndMin)))
    );

    const activeCount = [lightState, filterState, heaterState, aerationState].filter((state) => state === 'on').length;
    setText('relay-count', `${activeCount} / 4 aktywne`);

    const lightToggle = document.getElementById('relay-light-toggle');
    if (lightToggle && lightToggle.dataset.busy !== '1') {
        lightToggle.textContent = relays.light ? 'Wylacz' : 'Wlacz';
    }

    const filterToggle = document.getElementById('relay-filter-toggle');
    if (filterToggle && filterToggle.dataset.busy !== '1') {
        filterToggle.textContent = relays.pump ? 'Wylacz' : 'Wlacz';
    }
}

function describeFeedSchedule(feeding) {
    const freqLabel = formatFeedFrequency(feeding?.freq);
    if (freqLabel === 'Wylaczone') {
        return 'Automatyczne karmienie wylaczone';
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
            label: 'Swiatlo',
            value: modeValue(schedule.lightMode) === 1
                ? 'Zawsze wlaczone'
                : (modeValue(schedule.lightMode) === 2
                    ? 'Wylaczone'
                    : formatRange(schedule.dayStartHour, schedule.dayStartMin, schedule.dayEndHour, schedule.dayEndMin))
        },
        {
            label: 'Filtr',
            value: modeValue(schedule.filterMode) === 1
                ? 'Zawsze wlaczony'
                : (modeValue(schedule.filterMode) === 2
                    ? 'Wylaczony'
                    : formatRange(schedule.filterStartHour, schedule.filterStartMin, schedule.filterEndHour, schedule.filterEndMin))
        },
        {
            label: 'Napowietrzanie',
            value: modeValue(schedule.airMode) === 1
                ? 'Zawsze aktywne'
                : (modeValue(schedule.airMode) === 2
                    ? 'Wylaczone'
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
    const lastResult = normalizeFeedResultCode(feeding.lastResult);
    setText('feed-next-label', describeFeedSchedule(feeding));

    const lastFeedText = formatEpoch(feeding.lastFeedEpoch, {
        fallback: 'brak danych',
        includeDate: true,
        includeSeconds: false
    });
    setText('feed-last-label', `Ostatnie karmienie: ${lastFeedText}`);

    if (feeding.active && (feedActionState.awaitingResponse || feedActionState.awaitingCompletion)) {
        feedActionState.awaitingResponse = false;
        feedActionState.awaitingCompletion = true;
        feedActionState.sawActive = true;
        showFeedModalState('progress', 'Trwa karmienie...', 'Sensor polozenia jest w trakcie odczytu.');
    } else if (feedActionState.awaitingCompletion && !feeding.active) {
        const lastFeedEpoch = Math.max(0, Math.trunc(Number(feeding.lastFeedEpoch) || 0));
        const resultChanged = lastResult && lastResult !== feedActionState.baselineLastResult;
        const feedConfirmed = feedActionState.sawActive ||
            lastFeedEpoch > feedActionState.baselineLastFeedEpoch ||
            resultChanged;
        const waitExpired = (Date.now() - feedActionState.startedAtMs) > 15000;

        if (feedConfirmed || waitExpired) {
            const result = describeFeedResult(waitExpired && !feedConfirmed ? 'timeout' : lastResult, '');
            showFeedModalState(result.kind, result.title, result.message, 2400);
            resetFeedActionState();
        }
    }

    const button = document.getElementById('feed-now-btn');
    if (button) {
        const busy = !!feeding.active || feedActionState.awaitingResponse || feedActionState.awaitingCompletion;
        button.disabled = busy;
        button.textContent = feeding.active
            ? 'Trwa...'
            : (feedActionState.awaitingResponse ? 'Start...' : (feedActionState.awaitingCompletion ? 'Czekam...' : 'Karm teraz'));
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
    renderTemperatureSettingsPanel(data);
    renderDiagnosticsPanel(data);
    renderScheduleEditor(data);
    renderRelays(data);
    renderTodaySchedule(data);
    renderFeederCard(data);
    renderTemperatureChart(data.temperature || {});
}

async function triggerFeed() {
    if (feedActionState.awaitingResponse || feedActionState.awaitingCompletion || lastStatusData?.feeding?.active) {
        return;
    }

    feedActionState.awaitingResponse = true;
    feedActionState.awaitingCompletion = false;
    feedActionState.sawActive = false;
    feedActionState.startedAtMs = Date.now();
    feedActionState.baselineLastFeedEpoch = Math.max(0, Math.trunc(Number(lastStatusData?.feeding?.lastFeedEpoch) || 0));
    feedActionState.baselineLastResult = normalizeFeedResultCode(lastStatusData?.feeding?.lastResult);

    showFeedModalState('progress', 'Uruchamianie karmienia...', 'Wysylam polecenie do sterownika.');

    try {
        await sendAction('feed_now', {}, { showSaveAnimation: false });
        feedActionState.awaitingResponse = false;
        feedActionState.awaitingCompletion = true;
        showFeedModalState('progress', 'Trwa karmienie...', 'Sterownik przyjal polecenie. Czekam na potwierdzenie.');
        await fetchStatus(true);
    } catch (error) {
        const result = describeFeedResult(error?.code, describeRequestError(error));
        showFeedModalState(result.kind, result.title, result.message, 2600);
        resetFeedActionState();
        await fetchStatus(true);
    }
}
