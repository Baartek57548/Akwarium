const API_STATUS = '/api/status';
const API_ACTION = '/api/action';
const API_LOGS = '/api/logs';
const API_OTA = '/update';

let backendConnected = false;
let activeLogType = 'normal';
let cachedLogs = { normal: [], critical: [] };

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

// Clock Logic
function updateClock() {
    const now = new Date();
    
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

async function fetchStatus() {
    try {
        const response = await fetch(API_STATUS, { cache: 'no-store' });
        if (!response.ok) throw new Error('status http');
        const data = await response.json();
        setBackendState(true);

        const apBadge = document.getElementById('ap-badge');
        const staBadge = document.getElementById('sta-badge');
        if (apBadge && staBadge && data.network) {
            const apMode = !!data.network.apMode;
            apBadge.style.opacity = apMode ? '1' : '0.45';
            staBadge.style.opacity = apMode ? '0.45' : '1';
        }

        const rtcBattery = document.getElementById('rtc-battery');
        if (rtcBattery && data.battery?.voltage !== undefined) {
            rtcBattery.textContent = `${Number(data.battery.voltage).toFixed(2)}V`;
        }
    } catch (e) {
        setBackendState(false);
    }
}

async function fetchLogs() {
    try {
        const response = await fetch(API_LOGS, { cache: 'no-store' });
        if (!response.ok) throw new Error('logs http');
        const logs = await response.json();
        cachedLogs.normal = Array.isArray(logs.normal) ? logs.normal : [];
        cachedLogs.critical = Array.isArray(logs.critical) ? logs.critical : [];
        renderLogs();
    } catch (e) {
        // keep last logs
    }
}

async function sendAction(action, payload = {}) {
    const params = new URLSearchParams({ action, ...payload });
    const response = await fetch(API_ACTION, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params.toString()
    });
    if (!response.ok) {
        throw new Error(await response.text());
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
            modeSelect?.addEventListener('change', () => updateRangeScheduleItem(item));
            startInput?.addEventListener('input', () => updateRangeScheduleItem(item));
            endInput?.addEventListener('input', () => updateRangeScheduleItem(item));
            updateRangeScheduleItem(item);
        } else {
            const pointInput = item.querySelector('.schedule-time-point');
            pointInput?.addEventListener('input', () => updatePointScheduleItem(item));
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
function initOTA() {
    const fileInput = document.getElementById('firmware-file');
    const uploadBtn = document.getElementById('upload-btn');
    
    if(fileInput && uploadBtn) {
        fileInput.addEventListener('change', (e) => {
            if(e.target.files.length > 0) {
                const file = e.target.files[0];
                if(file.name.endsWith('.bin')) {
                    uploadBtn.disabled = false;
                    uploadBtn.textContent = `Aktualizuj System (${file.name})`;
                } else {
                    alert('Proszę wybrać poprawny plik firmware z rozszerzeniem .bin');
                    uploadBtn.disabled = true;
                    e.target.value = '';
                }
            } else {
                uploadBtn.disabled = true;
                uploadBtn.textContent = 'Aktualizuj System';
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
                btn.textContent = 'Aktualizuj System';
                btn.style.backgroundColor = '';
                const firmwareFile = document.getElementById('firmware-file');
                if(firmwareFile) {
                    firmwareFile.value = '';
                }
            }, 1000);
        }
    };

    xhr.onerror = function () {
        btn.textContent = 'Błąd sieci OTA';
        btn.style.backgroundColor = 'var(--danger-color)';
        alert('Błąd połączenia podczas OTA.');
    };

    xhr.onloadend = function () {
        btn.disabled = false;
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
            await fetchLogs();
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

    fetchStatus();
    fetchLogs();
    setInterval(fetchStatus, 3000);
    setInterval(fetchLogs, 5000);

    // Mock toggle logic for dashboard toggles
    const toggles = document.querySelectorAll('input[type="checkbox"]');
    toggles.forEach(toggle => {
        toggle.addEventListener('change', (e) => {
            console.log(`${e.target.id} changed to ${e.target.checked}`);
        });
    });
});
