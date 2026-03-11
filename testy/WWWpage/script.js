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
    modal.style.display = 'flex';
    
    // Simulate feeding process
    setTimeout(() => {
        const icon = document.getElementById('modal-icon');
        const text = document.getElementById('modal-text');
        const p = document.getElementById('modal-subtext');
        
        icon.className = 'fa-solid fa-check-circle fa-2xl';
        icon.style.color = 'var(--success-color)';
        text.textContent = 'Sukces';
        p.textContent = 'Karmienie zakończone pomyślnie. Status sensora: OK.';
        
        setTimeout(() => {
            modal.style.display = 'none';
            // Reset for next time
            setTimeout(() => {
                icon.className = 'fa-solid fa-spinner fa-spin fa-2xl';
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

    progressContainer.style.display = 'block';
    btn.disabled = true;
    
    let progress = 0;
    const interval = setInterval(() => {
        progress += Math.floor(Math.random() * 10) + 1;
        if(progress > 100) progress = 100;
        
        fill.style.width = `${progress}%`;
        percentTxt.textContent = `${progress}%`;
        
        if(progress >= 100) {
            clearInterval(interval);
            btn.textContent = 'Zakończono Pomyślnie!';
            btn.style.backgroundColor = 'var(--success-color)';
            
            setTimeout(() => {
                alert('Aktualizacja zakończona pomyślnie. Urządzenie zrestartuje się za chwilę.');
                // Reset UI
                progressContainer.style.display = 'none';
                fill.style.width = '0%';
                percentTxt.textContent = '0%';
                btn.textContent = 'Aktualizuj System';
                btn.style.backgroundColor = '';
                document.getElementById('firmware-file').value = '';
            }, 1000);
        }
    }, 300);
}

// Init Event Listeners
document.addEventListener('DOMContentLoaded', () => {
    updateClock();
    setInterval(updateClock, 1000);
    
    initNavigation();
    initOTA();
    initLogSearch();
    initLogBackend();

    // Mock toggle logic for dashboard toggles
    const toggles = document.querySelectorAll('input[type="checkbox"]');
    toggles.forEach(toggle => {
        toggle.addEventListener('change', (e) => {
            console.log(`${e.target.id} changed to ${e.target.checked}`);
        });
    });
});

// Log Search — real-time filtering with highlight
function initLogSearch() {
    const searchInput = document.getElementById('log-search');
    const logContainer = document.getElementById('log-entries');
    if (!searchInput || !logContainer) return;

    // Store original text content for each entry
    const entries = logContainer.children;

    searchInput.addEventListener('input', () => {
        const query = searchInput.value.trim().toLowerCase();
        let visibleCount = 0;

        // Remove existing "no results" message
        const existingMsg = logContainer.querySelector('.log-no-results');
        if (existingMsg) existingMsg.remove();

        for (const entry of entries) {
            // Skip the no-results placeholder
            if (entry.classList.contains('log-no-results')) continue;

            const spans = entry.querySelectorAll('span');
            const fullText = Array.from(spans).map(s => s.textContent).join(' ').toLowerCase();

            if (!query || fullText.includes(query)) {
                entry.style.display = '';
                visibleCount++;
                // Highlight matching text in message span (last span)
                if (query && spans.length > 0) {
                    highlightMatch(spans[spans.length - 1], query);
                } else {
                    // Remove highlights
                    spans.forEach(s => removeHighlight(s));
                }
            } else {
                entry.style.display = 'none';
            }
        }

        // Show "no results" if nothing matched
        if (query && visibleCount === 0) {
            const noResults = document.createElement('div');
            noResults.className = 'log-no-results';
            noResults.style.cssText = 'text-align: center; padding: 40px 20px; color: var(--text-muted); font-size: 14px;';
            noResults.innerHTML = '<i class="fa-solid fa-magnifying-glass" style="font-size: 24px; margin-bottom: 12px; display: block; opacity: 0.4;"></i>Brak wyników dla „' + escapeHtml(searchInput.value) + '"';
            logContainer.appendChild(noResults);
        }
    });
}

// Backend Log Sync + CSV Export
const LOG_POLL_MS = 5000;
let logPollTimer = null;
let lastLogsPayload = null;

function getApiBase() {
    if (window.API_BASE) {
        return String(window.API_BASE).replace(/\/$/, '');
    }
    return '';
}

function initLogBackend() {
    const logContainer = document.getElementById('log-entries');
    if (!logContainer) return;

    const downloadBtn = document.getElementById('log-download-btn');
    if (downloadBtn) {
        downloadBtn.addEventListener('click', handleLogDownload);
    }

    fetchLogsFromBackend();
    if (logPollTimer) clearInterval(logPollTimer);
    logPollTimer = setInterval(fetchLogsFromBackend, LOG_POLL_MS);
}

async function fetchLogsFromBackend() {
    try {
        const res = await fetch(`${getApiBase()}/api/logs`, { cache: 'no-store' });
        if (!res.ok) throw new Error(`http_${res.status}`);
        const data = await res.json();
        if (!data || !Array.isArray(data.normal) || !Array.isArray(data.critical)) {
            throw new Error('bad_payload');
        }
        lastLogsPayload = data;
        renderLogEntriesFromPayload(data);
        updateLogStatus('Polaczono', true);
    } catch (err) {
        updateLogStatus('Brak odpowiedzi sterownika.', false);
    }
}

function updateLogStatus(text, ok) {
    const statusEl = document.getElementById('log-status');
    if (!statusEl) return;
    statusEl.textContent = text;
    statusEl.style.color = ok ? 'var(--success-color)' : 'var(--text-muted)';
}

function renderLogEntriesFromPayload(payload) {
    const logContainer = document.getElementById('log-entries');
    if (!logContainer) return;

    logContainer.innerHTML = '';
    let infoCount = 0;
    let criticalCount = 0;

    const appendEntry = (raw, sourceType) => {
        const parsed = parseLogString(raw);
        const tag = inferTagFromMessage(parsed.message, sourceType === 'critical' ? 'CRIT' : 'INFO');
        const severity = isCriticalTag(tag) ? 'Krytyczny' : 'Informacyjny';

        if (severity === 'Krytyczny') {
            criticalCount++;
        } else {
            infoCount++;
        }

        logContainer.appendChild(buildLogEntry(tag, parsed.time, parsed.message, severity));
    };

    payload.normal.forEach(raw => appendEntry(raw, 'normal'));
    payload.critical.forEach(raw => appendEntry(raw, 'critical'));

    updateLogCounts(infoCount, criticalCount);
    reapplyLogSearch();
}

function buildLogEntry(tag, time, message, severity) {
    const row = document.createElement('div');
    row.style.cssText = 'background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 14px 20px; display: flex; align-items: center; font-size: 13px;';
    row.dataset.tag = tag;
    row.dataset.severity = severity;

    const tagSpan = document.createElement('span');
    tagSpan.textContent = tag;
    tagSpan.style.cssText = 'font-weight: 600; width: 80px;';
    tagSpan.style.color = pickTagColor(tag);

    const timeSpan = document.createElement('span');
    timeSpan.textContent = time || '--:--:--';
    timeSpan.style.cssText = 'color: var(--text-muted); width: 100px;';

    const msgSpan = document.createElement('span');
    msgSpan.textContent = message || '';
    msgSpan.style.color = 'var(--text-main)';

    row.appendChild(tagSpan);
    row.appendChild(timeSpan);
    row.appendChild(msgSpan);
    return row;
}

function pickTagColor(tag) {
    const t = String(tag || '').toUpperCase();
    if (t === 'WARN') return '#fb923c';
    if (t === 'ERROR' || t === 'CRIT') return '#ef4444';
    return 'var(--accent-cyan)';
}

function parseLogString(raw) {
    if (typeof raw !== 'string') return { time: '', message: '' };
    const match = raw.match(/^\[([^\]]+)\]\s*(.*)$/);
    if (match) {
        return { time: match[1].trim(), message: match[2].trim() };
    }
    return { time: '', message: raw.trim() };
}

function inferTagFromMessage(message, fallbackTag) {
    const text = String(message || '').toUpperCase();
    if (/\bCRIT\b/.test(text)) return 'CRIT';
    if (/\bERROR\b/.test(text)) return 'ERROR';
    if (/\bWARN\b/.test(text)) return 'WARN';
    return fallbackTag || 'INFO';
}

function isCriticalTag(tag) {
    const t = String(tag || '').toUpperCase();
    return t === 'WARN' || t === 'ERROR' || t === 'CRIT';
}

function updateLogCounts(infoCount, criticalCount) {
    const infoEl = document.getElementById('log-info-count');
    if (infoEl) infoEl.textContent = String(infoCount);
    const critEl = document.getElementById('log-critical-count');
    if (critEl) critEl.textContent = String(criticalCount);
}

function reapplyLogSearch() {
    const searchInput = document.getElementById('log-search');
    if (!searchInput) return;
    searchInput.dispatchEvent(new Event('input'));
}

function handleLogDownload() {
    const rows = lastLogsPayload ? buildCsvRowsFromPayload(lastLogsPayload) : buildCsvRowsFromDom();
    const csv = buildCsvContent(rows);
    const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `logi_akwarium_${formatDateForFilename(new Date())}.csv`;
    document.body.appendChild(a);
    a.click();
    a.remove();
    URL.revokeObjectURL(url);
}

function buildCsvRowsFromPayload(payload) {
    const rows = [];
    const add = (raw, sourceType) => {
        const parsed = parseLogString(raw);
        const tag = inferTagFromMessage(parsed.message, sourceType === 'critical' ? 'CRIT' : 'INFO');
        const severity = isCriticalTag(tag) ? 'Krytyczny' : 'Informacyjny';
        rows.push({ type: severity, time: parsed.time, message: parsed.message });
    };
    payload.normal.forEach(raw => add(raw, 'normal'));
    payload.critical.forEach(raw => add(raw, 'critical'));
    return rows;
}

function buildCsvRowsFromDom() {
    const logContainer = document.getElementById('log-entries');
    if (!logContainer) return [];
    const rows = [];
    const entries = logContainer.querySelectorAll(':scope > div');

    entries.forEach(entry => {
        if (entry.classList.contains('log-no-results')) return;
        const spans = entry.querySelectorAll('span');
        if (spans.length < 3) return;

        const tag = spans[0].textContent.trim();
        const time = spans[1].textContent.trim();
        const message = spans[2].textContent.trim();
        const severity = classifyDomEntry(tag, spans[0], message);

        rows.push({ type: severity, time, message });
    });

    return rows;
}

function classifyDomEntry(tag, tagSpan, message) {
    if (isCriticalTag(tag)) return 'Krytyczny';
    const msgUpper = String(message || '').toUpperCase();
    if (/\bCRIT\b/.test(msgUpper) || /\bERROR\b/.test(msgUpper) || /\bWARN\b/.test(msgUpper)) {
        return 'Krytyczny';
    }
    if (tagSpan) {
        const color = getComputedStyle(tagSpan).color;
        if (isCriticalColor(color)) return 'Krytyczny';
    }
    return 'Informacyjny';
}

function isCriticalColor(color) {
    const raw = String(color || '').toLowerCase();
    if (raw.includes('red') || raw.includes('orange')) return true;
    const match = raw.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)/);
    if (!match) return false;
    const r = parseInt(match[1], 10);
    const g = parseInt(match[2], 10);
    const b = parseInt(match[3], 10);
    return r >= 200 && g <= 180 && b <= 140;
}

function buildCsvContent(rows) {
    const lines = ['typ;czas;wiadomosc'];
    rows.forEach(row => {
        const type = row.type || 'Informacyjny';
        const time = row.time || '';
        const message = escapeCsvMessage(row.message || '');
        lines.push(`${type};${time};${message}`);
    });
    return '\uFEFF' + lines.join('\r\n');
}

function escapeCsvMessage(value) {
    const sanitized = String(value).replace(/\r?\n/g, ' ').replace(/"/g, '""');
    return `"${sanitized}"`;
}

function formatDateForFilename(date) {
    const yyyy = date.getFullYear();
    const mm = String(date.getMonth() + 1).padStart(2, '0');
    const dd = String(date.getDate()).padStart(2, '0');
    return `${yyyy}-${mm}-${dd}`;
}

function highlightMatch(span, query) {
    // First, remove existing highlights
    removeHighlight(span);
    const text = span.textContent;
    const lowerText = text.toLowerCase();
    const idx = lowerText.indexOf(query);
    if (idx === -1) return;

    const before = text.substring(0, idx);
    const match = text.substring(idx, idx + query.length);
    const after = text.substring(idx + query.length);

    span.innerHTML = escapeHtml(before) +
        '<mark style="background: rgba(34,211,238,0.25); color: var(--accent-cyan); border-radius: 3px; padding: 0 2px;">' +
        escapeHtml(match) + '</mark>' +
        escapeHtml(after);
}

function removeHighlight(span) {
    // If the span contains <mark> tags, replace with plain text
    if (span.querySelector('mark')) {
        span.textContent = span.textContent;
    }
}

function escapeHtml(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

// Save Schedules with visual feedback
function saveSchedules() {
    const btn = document.querySelector('[onclick="saveSchedules()"]');
    const lastSaved = btn ? btn.parentElement.querySelector('span') : null;

    if (btn) {
        const original = btn.innerHTML;
        btn.innerHTML = '<i class="fa-solid fa-check"></i> Zapisano!';
        btn.style.background = 'rgba(16, 185, 129, 0.15)';
        btn.style.borderColor = 'rgba(16, 185, 129, 0.4)';
        btn.style.color = '#10b981';
        btn.disabled = true;

        if (lastSaved) {
            const now = new Date();
            const timeStr = now.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' });
            lastSaved.textContent = `Ostatni zapis: dzisiaj ${timeStr}`;
        }

        setTimeout(() => {
            btn.innerHTML = original;
            btn.style.background = 'rgba(34, 211, 238, 0.08)';
            btn.style.borderColor = 'rgba(34, 211, 238, 0.25)';
            btn.style.color = 'var(--accent-cyan)';
            btn.disabled = false;
        }, 2500);
    }
}

// Schedule Bar Renderer — adapts scale for mobile vs desktop
function renderScheduleBars() {
    const containers = document.querySelectorAll('.schedule-bar-container[data-start]');
    if (!containers.length) return;

    const isMobile = window.innerWidth <= 768;

    // Parse HH:MM → decimal hours
    const parseH = t => { const [h, m] = t.split(':').map(Number); return h + m / 60; };

    // Always use full 0–24h scale
    const scaleStart = 0;
    const scaleEnd   = 24;
    const totalH = 24;

    containers.forEach(c => {
        const bar = c.querySelector('.schedule-bar');
        const point = c.querySelector('.schedule-point');
        const startPill = c.querySelector('.start-pill');
        const endPill   = c.querySelector('.end-pill');

        const startH = parseH(c.dataset.start);
        const isPoint = c.dataset.point === 'true';

        const leftPct = ((startH - scaleStart) / totalH * 100);

        // Helper: pick transform so pill doesn't clip at container edges
        const pillTransform = (pct) => {
            if (pct <= 5)  return 'translateX(0%)';
            if (pct >= 95) return 'translateX(-100%)';
            return 'translateX(-50%)';
        };

        if (isPoint) {
            // Feeding point marker
            if (point) { point.style.left = leftPct.toFixed(2) + '%'; }
            if (startPill) {
                startPill.style.left = leftPct.toFixed(2) + '%';
                startPill.style.transform = pillTransform(leftPct);
            }
        } else {
            const endH = parseH(c.dataset.end);
            const widthPct = ((endH - startH) / totalH * 100).toFixed(2);
            const endLeftPct = ((endH - scaleStart) / totalH * 100);

            if (bar) {
                bar.style.left  = leftPct.toFixed(2) + '%';
                bar.style.width = widthPct + '%';
            }
            if (startPill) {
                startPill.style.left = leftPct.toFixed(2) + '%';
                startPill.style.transform = pillTransform(leftPct);
            }
            if (endPill) {
                endPill.style.left = endLeftPct.toFixed(2) + '%';
                endPill.style.transform = pillTransform(endLeftPct);
            }
        }

        // Set margin-bottom to give pills room
        c.style.marginBottom = '36px';
    });
}

// Run on load and resize
window.addEventListener('resize', renderScheduleBars);
document.addEventListener('DOMContentLoaded', () => {
    renderScheduleBars();
    initScheduleInputs();
});

// Live-bind time inputs to schedule bar rendering
function initScheduleInputs() {
    const timePills = document.querySelectorAll('.schedule-bar-container .time-pill');
    timePills.forEach(pill => {
        pill.addEventListener('input', onTimePillChange);
        pill.addEventListener('change', onTimePillChange);
    });
}

function onTimePillChange(e) {
    const pill = e.target;
    const container = pill.closest('.schedule-bar-container');
    if (!container || !pill.value) return;

    if (pill.classList.contains('start-pill')) {
        container.dataset.start = pill.value;
    } else if (pill.classList.contains('end-pill')) {
        container.dataset.end = pill.value;
    }

    renderScheduleBars();
}
