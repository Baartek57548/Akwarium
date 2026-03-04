#ifndef WEBASSETS_H
#define WEBASSETS_H

#include <Arduino.h>

const char web_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pl">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Akwarium Dashboard</title>
    <!-- Basic, clean font -->
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="style.css?v=3">
</head>

<body>
    <div class="container">

        <!-- Header -->
        <header class="app-header">
            <div class="header-main">
                <div class="title-area">
                    <h1 style="display: flex; align-items: center; gap: 8px;">Akwarium 🐠</h1>
                    <span class="badge" id="otaNetworkMode">--</span>
                </div>
                <div class="system-status">
                    <span id="systemTime" class="clock">--:--:--</span>
                    <div id="connectionStatus" class="conn-status ok">Połączono</div>
                </div>
            </div>

            <!-- Tabs -->
            <nav class="tabs">
                <button class="tab-btn active" data-target="tabDashboard">DASHBOARD</button>
                <button class="tab-btn" data-target="tabLogs">LOGI</button>
                <button class="tab-btn" data-target="tabOTA">SYSTEM / OTA</button>
            </nav>
        </header>

        <!-- DASHBOARD TAB -->
        <main class="tab-content active" id="tabDashboard">

            <div class="grid-layout">

                <!-- Sensors -->
                <div class="panel">
                    <div class="panel-header">Czujniki</div>
                    <div class="panel-body flex-col gap-15">
                        <div class="data-box">
                            <span class="box-icon">🌡️</span>
                            <div class="box-content">
                                <div class="box-title">Temperatura</div>
                                <div class="box-value"><span id="valTemp">--.-</span> <small>°C</small></div>
                            </div>
                        </div>
                        <div class="data-compact">
                            <div class="row"><span>Min Temp:</span> <strong><span id="valTempMin">--.- °C</span> <span
                                        style="font-size: 0.8em; font-weight: normal; color: var(--text-muted);"
                                        id="valTempMinTime">(--:--)</span></strong></div>
                        </div>
                    </div>
                </div>

                <!-- Power -->
                <div class="panel">
                    <div class="panel-header">Zasilanie</div>
                    <div class="panel-body flex-col gap-15">
                        <div class="data-box">
                            <span class="box-icon">🔋</span>
                            <div class="box-content">
                                <div class="box-title">Bateria</div>
                                <div class="box-value"><span id="valBattPct">--</span> <small>%</small></div>
                            </div>
                        </div>
                        <div class="progress-wrap">
                            <div class="progress-bar">
                                <div class="progress-fill" id="battFill" style="width: 0%;"></div>
                            </div>
                        </div>
                        <div class="data-compact">
                            <div class="row"><span>Napięcie:</span> <strong id="valBattVolt">-.-- V</strong></div>
                        </div>
                    </div>
                </div>

                <!-- Relays/Schedules merged -->
                <div class="panel span-2">
                    <div class="panel-header">Urządzenia i Harmonogramy</div>
                    <div class="panel-body no-pad">
                        <table class="data-table devices-table">
                            <thead>
                                <tr>
                                    <th>Urządzenie</th>
                                    <th>Status</th>
                                    <th>Harmonogram</th>
                                </tr>
                            </thead>
                            <tbody>
                                <tr id="relayLight">
                                    <td>💡 Oświetlenie</td>
                                    <td>
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indLight"></span>
                                            <strong id="statusLight">Wyłączone</strong>
                                        </div>
                                    </td>
                                    <td>
                                        <div class="schedule-edit">
                                            <span class="text-muted">ON:</span> <input type="time" class="input-time"
                                                id="schedDayStart">
                                            <span class="text-muted">OFF:</span> <input type="time" class="input-time"
                                                id="schedDayEnd">
                                        </div>
                                    </td>
                                </tr>
                                <tr id="relayPump">
                                    <td>🫧 Filtr</td>
                                    <td>
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indPump"></span>
                                            <strong id="statusPump">Wyłączone</strong>
                                        </div>
                                    </td>
                                    <td>
                                        <div class="schedule-edit">
                                            <span class="text-muted">ON:</span> <input type="time" class="input-time"
                                                id="schedFilterOn">
                                            <span class="text-muted">OFF:</span> <input type="time" class="input-time"
                                                id="schedFilterOff">
                                        </div>
                                    </td>
                                </tr>
                                <tr id="relayTermostat">
                                    <td>🌡️ Termostat</td>
                                    <td>
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indHeater"></span>
                                            <strong id="statusHeater">Wyłączone</strong>
                                        </div>
                                    </td>
                                    <td>
                                        <div class="schedule-edit">
                                            <span class="text-muted">Docelowa:</span> <input type="number" class="input-time" id="schedTargetTemp" step="0.5" min="20" max="35" style="width:75px; margin-right:8px;">
                                            <span class="text-muted">Histereza:</span> <input type="number" class="input-time" id="schedTempHyst" step="0.1" min="0.1" max="2.0" style="width:70px;">
                                        </div>
                                    </td>
                                </tr>
                                <tr id="relayAeration">
                                    <td>💨 Napowietrzanie</td>
                                    <td>
                                        <div class="status-cell">
                                            <strong id="valServoAngle"
                                                style="width:50px; display:inline-block;">--%</strong>
                                            <small>(<span id="statusServoMode">Zbrojny</span>)</small>
                                        </div>
                                    </td>
                                    <td>
                                        <div class="schedule-edit">
                                            <span class="text-muted">ON:</span> <input type="time" class="input-time"
                                                id="schedAirOn">
                                            <span class="text-muted">OFF:</span> <input type="time" class="input-time"
                                                id="schedAirOff">
                                            <input type="range" min="0" max="100" value="0" class="input-range"
                                                id="valServoSlider" style="width: 100%; margin-top: 15px;">
                                            <input type="hidden" id="valPreOffSlider" value="30">
                                            <span id="valPreOffLabel" style="display:none;"></span>
                                        </div>
                                    </td>
                                </tr>
                                <tr id="relayServo">
                                    <td>⚙️ Karmnik</td>
                                    <td>
                                        <div class="status-cell">
                                            <button class="btn btn-small btn-primary" id="btnFeedNow"
                                                onclick="alert('Karmienie wymuszone (Mock)')">Karm teraz</button>
                                        </div>
                                    </td>
                                    <td>
                                        <div class="schedule-edit">
                                            <input type="time" class="input-time" id="schedFeedTime">
                                            <select class="input-select" id="valFeedMode">
                                                <option value="1">Codziennie</option>
                                                <option value="2">Co 2 dni</option>
                                                <option value="3">Co 3 dni</option>
                                                <option value="0">Wyłączony</option>
                                            </select>
                                        </div>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                        <div style="padding: 15px; text-align: right; border-top: 1px solid var(--border);">
                            <button class="btn btn-small btn-secondary" id="btnSaveSchedules"
                                onclick="alert('Harmonogram zapisany (Zaimplementuj w C++)')">Zapisz
                                harmonogramy</button>
                        </div>
                    </div>
                </div>

            </div>
        </main>

        <!-- LOGS TAB -->
        <main class="tab-content" id="tabLogs">
            <div class="panel">
                <div class="panel-header flex-between">
                    <span>Logi Systemowe</span>
                    <div class="actions" style="display: flex; align-items: center; gap: 15px;">
                        <span class="text-muted" style="margin-right: 15px;">Ostatnie karmienie: <strong
                                id="valLastFeed" style="color: var(--text-main);">Nigdy</strong></span>
                        <div class="log-toggles">
                            <label style="cursor: pointer; margin-right: 8px;"><input type="radio" name="logTypeToggle" value="critical" id="toggleCritical" style="accent-color: var(--danger);"> Krytyczne</label>
                            <label style="cursor: pointer; margin-right: 8px;"><input type="radio" name="logTypeToggle" value="normal" id="toggleNormal" checked style="accent-color: var(--accent);"> Systemowe</label>
                        </div>
                        <button class="btn btn-small" id="btnClearLogs" style="margin-left:auto;">Wyczyść widok</button>
                        <button class="btn btn-small" id="btnDeleteCritical" style="background:var(--danger); color:white; display:none;">Usuń krytyczne</button>
                        <button class="btn btn-small btn-secondary" id="btnDownloadLogs">Pobierz TXT</button>
                    </div>
                </div>
                <div class="panel-body no-pad">
                    <div class="terminal" id="logContainer">
                        <div class="log-line">[SYSTEM] Terminal gotowy. Brak nowych logów.</div>
                    </div>
                </div>
            </div>
        </main>

        <!-- SYSTEM / OTA TAB -->
        <main class="tab-content" id="tabOTA">
            <div class="panel">
                <div class="panel-header">Aktualizacja Firmware (OTA)</div>
                <div class="panel-body flex-col gap-20">

                    <div class="info-strip">
                        <div class="strip-item"><span>Adres IP:</span> <strong id="otaIpAddress">--.--.--.--</strong>
                        </div>
                    </div>

                    <form id="uploadForm" class="upload-form" method="POST" action="/update"
                        enctype="multipart/form-data">
                        <div class="file-dropzone" id="dropzone">
                            <div class="dz-icon">📁</div>
                            <div class="dz-text">Wybierz lub upuść plik <strong>.bin</strong></div>
                            <input type="file" name="update" accept=".bin" required id="fileInput">
                        </div>

                        <div class="file-selected" id="selectedFile" style="display: none;">
                            Plik: <strong id="fileNameDisplay">brak</strong>
                        </div>

                        <button type="submit" class="btn btn-primary" id="submitBtn">Wgraj aktualizację i
                            zrestartuj</button>
                        <div id="uploadStatus" class="status-msg"></div>
                    </form>

                </div>
            </div>
        </main>

        <footer class="app-footer"
            style="text-align: center; margin-top: 10px; color: var(--text-muted); font-size: 0.85rem;">
            Made by Bartosz Wolny 2026
        </footer>

    </div>

    <script src="script.js?v=4"></script>
</body>

</html>
)rawliteral";

const char web_style_css[] PROGMEM = R"rawliteral(
:root {
    --bg-page: #0f172a;
    --bg-panel: #1e293b;
    --bg-panel-hover: #334155;
    --bg-card: #0f172a;

    --border: #334155;

    --text-main: #f8fafc;
    --text-muted: #94a3b8;

    --accent: #38bdf8;
    --accent-hover: #0284c7;

    --success: #10b981;
    --warning: #f59e0b;
    --danger: #ef4444;
    --off: #475569;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Inter', system-ui, sans-serif;
    background-color: var(--bg-page);
    color: var(--text-main);
    line-height: 1.5;
    font-size: 15px;
}

/* Layout Container */
.container {
    max-width: 1024px;
    margin: 0 auto;
    padding: 20px;
    display: flex;
    flex-direction: column;
    gap: 20px;
}

/* Header */
.app-header {
    background: var(--bg-panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    overflow: hidden;
}

.header-main {
    padding: 20px 24px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid var(--border);
}

.title-area h1 {
    font-size: 1.2rem;
    font-weight: 600;
    color: var(--text-main);
}

.badge {
    display: inline-block;
    background: rgba(255, 255, 255, 0.1);
    color: var(--text-muted);
    padding: 2px 8px;
    border-radius: 4px;
    font-size: 0.8rem;
    margin-top: 4px;
}

.system-status {
    text-align: right;
}

.clock {
    font-family: monospace;
    font-size: 1.2rem;
    font-weight: 600;
    color: var(--accent);
    display: block;
}

.conn-status {
    font-size: 0.85rem;
    font-weight: 500;
    display: flex;
    justify-content: flex-end;
    align-items: center;
    gap: 6px;
}

.conn-status::before {
    content: '';
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
}

.conn-status.ok {
    color: var(--success);
}

.conn-status.ok::before {
    background: var(--success);
    box-shadow: 0 0 5px var(--success);
}

.conn-status.err {
    color: var(--danger);
}

.conn-status.err::before {
    background: var(--danger);
}

/* Tabs */
.tabs {
    display: flex;
    background: rgba(0, 0, 0, 0.1);
}

.tab-btn {
    flex: 1;
    background: none;
    border: none;
    border-bottom: 2px solid transparent;
    color: var(--text-muted);
    padding: 14px;
    font-size: 0.9rem;
    font-weight: 600;
    cursor: pointer;
    transition: 0.2s;
}

.tab-btn:hover {
    background: rgba(255, 255, 255, 0.05);
    color: var(--text-main);
}

.tab-btn.active {
    color: var(--accent);
    border-bottom-color: var(--accent);
    background: rgba(56, 189, 248, 0.05);
}

.tab-content {
    display: none;
}

.tab-content.active {
    display: block;
    animation: fade 0.3s;
}

@keyframes fade {
    from {
        opacity: 0;
    }

    to {
        opacity: 1;
    }
}

/* Grid System */
.grid-layout {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 20px;
}

.span-2 {
    grid-column: span 2;
}

@media (max-width: 768px) {
    .grid-layout {
        grid-template-columns: 1fr;
    }

    .span-2 {
        grid-column: span 1;
    }
}

/* Panels */
.panel {
    background: var(--bg-panel);
    border: 1px solid var(--border);
    border-radius: 8px;
    display: flex;
    flex-direction: column;
}

.panel-header {
    padding: 14px 20px;
    font-size: 0.95rem;
    font-weight: 600;
    border-bottom: 1px solid var(--border);
    color: var(--text-muted);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.panel-body {
    padding: 20px;
}

.panel-body.no-pad {
    padding: 0;
}

.flex-between {
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.flex-col {
    display: flex;
    flex-direction: column;
}

.grid-2 {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
}

.grid-3 {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
}

.gap-15 {
    gap: 15px;
}

.gap-20 {
    gap: 20px;
}

/* Dashboard Specifics */
.data-box {
    display: flex;
    align-items: center;
    gap: 15px;
}

.box-icon {
    font-size: 2rem;
    background: var(--bg-card);
    padding: 12px;
    border-radius: 8px;
    border: 1px solid var(--border);
}

.box-title {
    font-size: 0.9rem;
    color: var(--text-muted);
    margin-bottom: 2px;
}

.box-value {
    font-size: 2.2rem;
    font-weight: 600;
    line-height: 1;
}

.box-value small {
    font-size: 1rem;
    color: var(--accent);
    font-weight: 500;
}

.data-compact {
    background: var(--bg-card);
    padding: 12px;
    border-radius: 6px;
    font-size: 0.9rem;
}

.data-compact .row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 4px;
}

.data-compact .row:last-child {
    margin-bottom: 0;
}

.data-compact span {
    color: var(--text-muted);
}

.progress-wrap {
    width: 100%;
    height: 6px;
    background: var(--bg-card);
    border-radius: 3px;
    overflow: hidden;
}

.progress-fill {
    height: 100%;
    background: var(--success);
    transition: width 0.5s;
}

/* Status Cards (Relays) & Indicators */
.sc-body {
    display: flex;
    align-items: center;
    gap: 8px;
    font-weight: 500;
}

.status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
}

.status-dot.on {
    background: var(--success);
    box-shadow: 0 0 5px var(--success);
}

.status-dot.off {
    background: var(--off);
}

.status-dot.active {
    background: var(--accent);
}

/* Data Tables */
.data-table {
    width: 100%;
    border-collapse: collapse;
    text-align: left;
    font-size: 0.95rem;
}

.data-table th,
.data-table td {
    padding: 14px 20px;
    border-bottom: 1px solid var(--border);
}

.data-table th {
    color: var(--text-muted);
    font-weight: 500;
    background: rgba(0, 0, 0, 0.1);
}

.data-table tr:last-child td {
    border-bottom: none;
}

.data-table strong {
    font-family: monospace;
    font-size: 1.05rem;
}

/* Status cell in table */
.status-cell {
    display: flex;
    align-items: center;
    gap: 8px;
}

.text-muted {
    color: var(--text-muted);
    font-size: 0.85rem;
}

/* Info Cards  */
.info-card {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 16px;
}

.ic-title {
    font-size: 0.9rem;
    color: var(--text-muted);
    margin-bottom: 6px;
}

.ic-value {
    font-size: 1.6rem;
    font-weight: 600;
    font-family: monospace;
}

.ic-sub {
    font-size: 0.85rem;
    color: var(--text-muted);
    margin-top: 4px;
}

.text-accent {
    color: var(--warning);
}

.text-normal {
    color: var(--text-main);
    font-size: 1.1rem;
}

/* Forms & Inputs & Buttons */
.input-time,
.input-select {
    background: rgba(0, 0, 0, 0.2);
    border: 1px solid var(--border);
    color: var(--text-main);
    padding: 6px 10px;
    border-radius: 4px;
    font-family: monospace;
    font-size: 0.95rem;
    outline: none;
    transition: 0.2s;
}

.input-time:focus,
.input-select:focus {
    border-color: var(--accent);
}

.input-time::-webkit-calendar-picker-indicator {
    filter: invert(1);
    opacity: 0.5;
    cursor: pointer;
}

.input-select {
    font-family: inherit;
    cursor: pointer;
}

.schedule-edit {
    display: flex;
    align-items: center;
    gap: 8px;
}

.btn {
    padding: 10px 16px;
    border: none;
    border-radius: 6px;
    font-family: inherit;
    font-weight: 500;
    cursor: pointer;
    transition: 0.2s;
}

.btn-primary {
    background: var(--accent);
    color: #fff;
    width: 100%;
    font-size: 1rem;
    padding: 14px;
}

.btn-primary:hover {
    background: var(--accent-hover);
}

.btn-small {
    padding: 6px 12px;
    font-size: 0.85rem;
}

.btn-secondary {
    background: var(--bg-card);
    color: var(--text-main);
    border: 1px solid var(--border);
}

.btn-secondary:hover {
    background: var(--bg-panel-hover);
}

/* OTA Tab */
.info-strip {
    display: flex;
    gap: 15px;
    background: var(--bg-card);
    padding: 12px 20px;
    border-radius: 6px;
}

.info-strip .strip-item {
    font-size: 0.95rem;
}

.info-strip span {
    color: var(--text-muted);
    margin-right: 8px;
}

.info-strip strong {
    font-family: monospace;
    color: var(--accent);
}

.file-dropzone {
    border: 2px dashed var(--border);
    border-radius: 8px;
    padding: 40px 20px;
    text-align: center;
    background: var(--bg-card);
    cursor: pointer;
    position: relative;
    transition: 0.2s;
}

.file-dropzone:hover,
.file-dropzone.dragover {
    border-color: var(--accent);
    background: rgba(56, 189, 248, 0.05);
}

.file-dropzone input {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    opacity: 0;
    cursor: pointer;
}

.dz-icon {
    font-size: 2.5rem;
    margin-bottom: 10px;
}

.dz-text {
    color: var(--text-muted);
    font-size: 1rem;
}

.file-selected {
    background: rgba(16, 185, 129, 0.1);
    color: var(--success);
    padding: 12px;
    border-radius: 6px;
    border: 1px solid rgba(16, 185, 129, 0.2);
    text-align: center;
}

.status-msg {
    text-align: center;
    color: var(--success);
    font-weight: 500;
    min-height: 20px;
}

/* Logs Terminal */
.terminal {
    background: #000;
    height: 350px;
    overflow-y: auto;
    padding: 15px;
    font-family: 'Consolas', monospace;
    font-size: 0.9rem;
    color: #cbd5e1;
    border-radius: 0 0 8px 8px;
}

.log-line {
    margin-bottom: 4px;
    word-break: break-all;
}

.terminal::-webkit-scrollbar {
    width: 8px;
}

.terminal::-webkit-scrollbar-track {
    background: #000;
}

.terminal::-webkit-scrollbar-thumb {
    background: #333;
    border-radius: 4px;
}

.log-line.system {
    color: #38bdf8;
}

.log-line.success {
    color: #10b981;
}

.log-line.warning {
    color: #f59e0b;
}

.log-line.ota {
    color: #c084fc;
}

/* Footer */
.app-footer {
    padding: 15px 0;
    font-size: 0.85rem;
    color: var(--text-muted);
}

.footer-wrap {
    display: flex;
    justify-content: space-between;
}

.link-btn {
    cursor: pointer;
    color: var(--accent);
}

.link-btn:hover {
    text-decoration: underline;
}

@media (max-width: 600px) {
    .header-main {
        flex-direction: column;
        gap: 10px;
        align-items: flex-start;
    }

    .system-status {
        text-align: left;
    }

    .footer-wrap {
        flex-direction: column;
        gap: 10px;
        align-items: center;
        text-align: center;
    }
}
)rawliteral";

const char web_script_js[] PROGMEM = R"rawliteral(
// Akwarium Web Panel JS Logic

// Tab Navigation
const tabBtns = document.querySelectorAll('.tab-btn');
const tabContents = document.querySelectorAll('.tab-content');

tabBtns.forEach(btn => {
    btn.addEventListener('click', () => {
        // Remove active class from all
        tabBtns.forEach(b => b.classList.remove('active'));
        tabContents.forEach(c => c.classList.remove('active'));

        // Add active class to clicked
        btn.classList.add('active');
        const targetId = btn.getAttribute('data-target');

        if (targetId !== 'tabDashboard') {
            sendAction('clear_servo');
        }

        document.getElementById(targetId).classList.add('active');
    });
});

// DOM Elements - Dashboard
const elTime = document.getElementById('systemTime');
const elConnStatus = document.getElementById('connectionStatus');

const elTempCard = document.getElementById('tempCard');
const elValTemp = document.getElementById('valTemp');
const elValTempMin = document.getElementById('valTempMin');
const elValTempMinTime = document.getElementById('valTempMinTime');

const elBattFill = document.getElementById('battFill');
const elValBattPct = document.getElementById('valBattPct');
const elValBattVolt = document.getElementById('valBattVolt');

const elStatusLight = document.getElementById('statusLight');
const elIndLight = document.getElementById('indLight');
const elStatusPump = document.getElementById('statusPump');
const elIndPump = document.getElementById('indPump');

const elValServoAngle = document.getElementById('valServoAngle');
const elStatusServoMode = document.getElementById('statusServoMode');
const elValServoSlider = document.getElementById('valServoSlider');

const elSchedDayStart = document.getElementById('schedDayStart');
const elSchedDayEnd = document.getElementById('schedDayEnd');
const elSchedAirOn = document.getElementById('schedAirOn');
const elSchedAirOff = document.getElementById('schedAirOff');
const elSchedFilterOn = document.getElementById('schedFilterOn');
const elSchedFilterOff = document.getElementById('schedFilterOff');
const elValPreOffSlider = document.getElementById('valPreOffSlider');
const elValPreOffLabel = document.getElementById('valPreOffLabel');
const elSchedTargetTemp = document.getElementById('schedTargetTemp');
const elSchedTempHyst = document.getElementById('schedTempHyst');
const indHeater = document.getElementById('indHeater');
const statusHeater = document.getElementById('statusHeater');

const elSchedFeedTime = document.getElementById('schedFeedTime');
const elValFeedMode = document.getElementById('valFeedMode');
const elValLastFeed = document.getElementById('valLastFeed');

// DOM Elements - OTA
const fileInput = document.getElementById('fileInput');
const dropzone = document.getElementById('dropzone');
const selectedFileDiv = document.getElementById('selectedFile');
const fileNameDisplay = document.getElementById('fileNameDisplay');
const otaIpAddress = document.getElementById('otaIpAddress');
const otaNetworkMode = document.getElementById('otaNetworkMode');

const otaForm = document.getElementById('uploadForm');
const btnSubmitOTA = document.getElementById('submitBtn');
const uploadStatus = document.getElementById('uploadStatus');

// DOM Elements - Logs
const logContainer = document.getElementById('logContainer');
const btnClearLogs = document.getElementById('btnClearLogs');
const btnDeleteCritical = document.getElementById('btnDeleteCritical');
const btnDownloadLogs = document.getElementById('btnDownloadLogs');
const radioLogs = document.getElementsByName('logTypeToggle');

// State for active log view
let currentLogView = 'normal'; // normal or critical
let lastDiagSignature = "";

radioLogs.forEach(radio => {
    radio.addEventListener('change', (e) => {
        currentLogView = e.target.value;
        if (currentLogView === 'critical') {
            btnDeleteCritical.style.display = 'inline-block';
        } else {
            btnDeleteCritical.style.display = 'none';
        }
        // Force refresh logs view exactly on switch without waiting for fetch
        renderStoredLogs();
    });
});

// Local timer for clock display
setInterval(() => {
    const now = new Date();
    elTime.innerText = now.toLocaleTimeString('pl-PL', { hour12: false });
}, 1000);

// API URL (relative, assuming ESP32 hosts this)
const API_STATUS = '/api/status';
const API_TIME = '/settime';

// Random function for mock logs
function addLogLine(msg, type = "normal") {
    const row = document.createElement('div');
    row.className = `log-line ${type}`;
    const time = new Date().toLocaleTimeString('pl-PL', { hour12: false });
    row.innerText = `[${time}] ${msg}`;

    logContainer.appendChild(row);
    // Auto-scroll to bottom
    logContainer.scrollTop = logContainer.scrollHeight;
}

// Data Fetching
async function fetchStatus() {
    try {
        const response = await fetch(API_STATUS);
        if (!response.ok) throw new Error('Network response was not ok');
        const data = await response.json();

        // Update Connection UI
        if (elConnStatus.className !== 'connection-status connected') {
            elConnStatus.className = 'connection-status connected';
            elConnStatus.innerHTML = '<span class="pulse-dot"></span> Połączono';
            addLogLine("Nawiązano połączenie z API /status", "success");
        }

        updateUI(data);
    } catch (error) {
        if (elConnStatus.className !== 'connection-status disconnected') {
            addLogLine("Brak połączenia z /api/status. Przełączanie w tryb Mock Data.", "warning");
        }

        elConnStatus.className = 'connection-status disconnected';
        elConnStatus.innerHTML = '<span class="pulse-dot" style="animation:none; opacity:1"></span> Tryb Offline';

        // Mock data injection if offline
        updateUI(getMockData());
    }
}

// UI Updater
function updateUI(data) {
    // 1. Temperature
    elValTemp.innerText = data.temperature.current.toFixed(1);
    elValTempMin.innerText = `${(data.temperature.min || 22.0).toFixed(1)} °C`;
    if (data.temperature.minTimeEpoch) {
        const dMin = new Date(data.temperature.minTimeEpoch * 1000);
        elValTempMinTime.innerText = `(${dMin.toLocaleDateString('pl-PL')} ${dMin.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' })})`;
    } else {
        elValTempMinTime.innerText = "";
    }

    // 2. Battery
    elValBattPct.innerText = data.battery.percent;
    elBattFill.style.width = `${data.battery.percent}%`;
    elValBattVolt.innerText = `${data.battery.voltage.toFixed(2)} V`;

    // Color battery based on level
    if (data.battery.percent < 20) {
        elBattFill.style.backgroundColor = 'var(--danger)';
    } else if (data.battery.percent < 50) {
        elBattFill.style.backgroundColor = 'var(--warning)';
    } else {
        elBattFill.style.backgroundColor = 'var(--success)';
    }

    // 3. Relays
    updateRelayUI(data.relays.light, elStatusLight, elIndLight);
    updateRelayUI(data.relays.pump, elStatusPump, elIndPump);

    // 4. Servo
    const percent = Math.round((1 - (data.servo.angle / 90)) * 100);
    elValServoAngle.innerText = `${percent}%`;
    elStatusServoMode.innerText = data.servo.angle === 0 ? "Otwarty" : data.servo.angle >= 90 ? "Zamknięty" : "Półotwarty";
    // Tylko uaktualniaj suwak jesli uzytkownik go nie przesuwa (zabezpieczenie przed resetem)
    if (!servoSliderLocked) {
        elValServoSlider.value = percent;
    }

    // 5. Schedules
    // Heater / Temp config
    if (data.temperature) {
        elSchedTargetTemp.value = data.temperature.target || 25.0;
        elSchedTempHyst.value = data.temperature.hysteresis || 0.5;
    }
    
    // Relay heater status
    if (data.relays && typeof data.relays.heater !== 'undefined') {
        indHeater.className = 'status-dot ' + (data.relays.heater ? 'on' : 'off');
        statusHeater.innerText = data.relays.heater ? 'Grzeje' : 'Wyłączone';
    } else {
        // Fallback for mock environment
        indHeater.className = 'status-dot off';
        statusHeater.innerText = 'Wyłączone';
    }

    elSchedDayStart.value = String(data.schedule.dayStartHour).padStart(2, '0') + ":" + String(data.schedule.dayStartMin).padStart(2, '0');
    if (document.activeElement !== elSchedDayEnd) elSchedDayEnd.value = padTime(data.schedule.dayEndHour, data.schedule.dayEndMin);
    if (document.activeElement !== elSchedAirOn) elSchedAirOn.value = padTime(data.schedule.airStartHour, data.schedule.airStartMin);
    if (document.activeElement !== elSchedAirOff) elSchedAirOff.value = padTime(data.schedule.airEndHour, data.schedule.airEndMin);
    if (document.activeElement !== elSchedFilterOn && data.schedule.filterStartHour !== undefined) elSchedFilterOn.value = padTime(data.schedule.filterStartHour, data.schedule.filterStartMin);
    if (document.activeElement !== elSchedFilterOff && data.schedule.filterEndHour !== undefined) elSchedFilterOff.value = padTime(data.schedule.filterEndHour, data.schedule.filterEndMin);

    if (document.activeElement !== elValPreOffSlider && data.schedule.servoPreOffMins !== undefined) {
        elValPreOffSlider.value = data.schedule.servoPreOffMins;
        elValPreOffLabel.innerText = data.schedule.servoPreOffMins;
    }

    // 6. Feeding
    if (document.activeElement !== elSchedFeedTime) elSchedFeedTime.value = padTime(data.feeding.hour, data.feeding.minute);
    if (document.activeElement !== elValFeedMode) elValFeedMode.value = data.feeding.freq || "1";

    if (data.feeding.lastFeedEpoch > 0) {
        const dDate = new Date(data.feeding.lastFeedEpoch * 1000);
        elValLastFeed.innerText = dDate.toLocaleString('pl-PL');
    } else {
        elValLastFeed.innerText = "Nigdy / Brak danych";
    }

    // Update OTA tab dynamic info
    if (data.network) {
        otaIpAddress.innerText = data.network.ip || "192.168.x.x";
        otaNetworkMode.innerText = data.network.apMode ? "Access Point" : "Station (WIFI DOM)";
        otaNetworkMode.style.color = data.network.apMode ? "var(--warning)" : "var(--success)";
    }

    if (data.diag) {
        const sig = `${data.diag.bootCount}|${data.diag.lastResetReason}|${data.diag.lastWakeupCause}|${data.diag.brownoutCount}|${data.diag.wdtCount}|${data.diag.panicCount}`;
        if (sig !== lastDiagSignature) {
            lastDiagSignature = sig;
            addLogLine(`DIAG boot=${data.diag.bootCount} reset=${data.diag.lastResetReason} wake=${data.diag.lastWakeupCause} brownout=${data.diag.brownoutCount} wdt=${data.diag.wdtCount} panic=${data.diag.panicCount}`, "system");
        }
    }
}

function updateRelayUI(isOn, badgeEl, indicatorEl) {
    if (isOn) {
        // Log state change if it was off previously (simplified checking for mock)
        if (badgeEl.textContent === 'Wyłączone') addLogLine(`${badgeEl.id.replace('status', '')} zostało WŁĄCZONE`, "system");

        badgeEl.textContent = 'Włączone';
        badgeEl.className = 'status-badge on';
        indicatorEl.style.backgroundColor = 'var(--success)';
        indicatorEl.style.boxShadow = '0 0 15px var(--success)';
    } else {
        if (badgeEl.textContent === 'Włączone') addLogLine(`${badgeEl.id.replace('status', '')} zostało WYŁĄCZONE`, "system");

        badgeEl.textContent = 'Wyłączone';
        badgeEl.className = 'status-badge off';
        indicatorEl.style.backgroundColor = 'var(--off-state)';
        indicatorEl.style.boxShadow = 'none';
    }
}

function padTime(hour, min) {
    const h = hour.toString().padStart(2, '0');
    const m = min.toString().padStart(2, '0');
    return `${h}:${m}`;
}

// Time Sync Function
function syncDeviceTime() {
    const now = new Date();
    const epoch = Math.floor(now.getTime() / 1000);

    addLogLine(`Żądanie synchronizacji czasu (Epoch: ${epoch})...`, "system");

    fetch(`${API_TIME}?epoch=${epoch}`, { method: 'POST' })
        .then(response => {
            if (response.ok) {
                addLogLine("Czas zsynchronizowany z układem ESP32 pomyślnie.", "success");
            }
        })
        .catch(err => {
            addLogLine("Synchronizacja wymuszona. (Mock Offline)", "warning");
        });
}

// API Action function
function sendAction(actionName, payload = {}) {
    const params = new URLSearchParams({ action: actionName, ...payload });
    addLogLine(`Wysyłanie akcji: ${actionName}...`, "system");

    fetch(`/api/action`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params.toString()
    })
        .then(response => {
            if (response.ok) {
                addLogLine(`Akcja '${actionName}' zakończona sukcesem.`, "success");
                fetchStatus(); // Refresh immediately
            } else {
                addLogLine(`Błąd akcji '${actionName}' (HTTP ${response.status})`, "danger");
            }
        })
        .catch(err => addLogLine(`Błąd sieci (action: ${actionName})`, "danger"));
}

// Button Events
document.getElementById('btnFeedNow').onclick = (e) => {
    e.preventDefault();
    sendAction('feed_now');
};

document.getElementById('btnSaveSchedules').onclick = (e) => {
    e.preventDefault();
    const payload = {
        dayStart: elSchedDayStart.value,
        dayEnd: elSchedDayEnd.value,
        targetTemp: elSchedTargetTemp.value,
        tempHyst: elSchedTempHyst.value,
        airOn: elSchedAirOn.value,
        airOff: elSchedAirOff.value,
        filterOn: elSchedFilterOn.value,
        filterOff: elSchedFilterOff.value,
        servoPreOffMins: elValPreOffSlider.value,
        feedTime: elSchedFeedTime.value,
        feedFreq: elValFeedMode.value
    };
    sendAction('save_schedule', payload);
};

// Blokada auto-aktualizacji suwaka servo podczas interakcji uzytkownika
let servoSliderLocked = false;
let servoSliderLockTimer = null;

function lockServoSlider() {
    servoSliderLocked = true;
    clearTimeout(servoSliderLockTimer);
    // Zwolnij blokade 3s po ostatniej interakcji
    servoSliderLockTimer = setTimeout(() => { servoSliderLocked = false; }, 3000);
}

// Blokuj podczas przeciagania
elValServoSlider.addEventListener('mousedown', lockServoSlider);
elValServoSlider.addEventListener('touchstart', lockServoSlider);
elValServoSlider.addEventListener('input', lockServoSlider);

// Wyslij akcje po zakonczeniu ruchu i blokuj na 3s
elValServoSlider.addEventListener('change', (e) => {
    lockServoSlider();
    sendAction('set_servo', { angle: Math.round(90 - (e.target.value / 100 * 90)) });
});

elValPreOffSlider.addEventListener('input', (e) => {
    elValPreOffLabel.innerText = e.target.value;
});

// Logs Tab Functionality
btnClearLogs.addEventListener('click', () => {
    logContainer.innerHTML = '';
    addLogLine("Logi zostały wyczyszczone.", "system");
});

btnDownloadLogs.addEventListener('click', () => {
    const lines = logContainer.innerText;
    const blob = new Blob([lines], { type: "text/plain" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `esp32_logs_${new Date().getTime()}.txt`;
    a.click();
});

// OTA Drag and Drop UX
['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
    dropzone.addEventListener(eventName, preventDefaults, false);
});
function preventDefaults(e) {
    e.preventDefault();
    e.stopPropagation();
}

['dragenter', 'dragover'].forEach(eventName => {
    dropzone.addEventListener(eventName, () => dropzone.classList.add('dragover'), false);
});

['dragleave', 'drop'].forEach(eventName => {
    dropzone.addEventListener(eventName, () => dropzone.classList.remove('dragover'), false);
});

dropzone.addEventListener('drop', (e) => {
    let dt = e.dataTransfer;
    let files = dt.files;
    fileInput.files = files; // Assign files to real input
    handleFiles(files);
});

fileInput.addEventListener('change', function () {
    handleFiles(this.files);
});

function handleFiles(files) {
    if (files.length > 0) {
        const file = files[0];
        if (!file.name.endsWith('.bin')) {
            alert('Proszę wybrać plik z rozszerzeniem .bin');
            fileInput.value = '';
            selectedFileDiv.style.display = 'none';
            return;
        }
        fileNameDisplay.innerText = `${file.name} (${(file.size / 1024 / 1024).toFixed(2)} MB)`;
        selectedFileDiv.style.display = 'block';
        addLogLine(`Załadowano plik firmware: ${file.name}`, "ota");
    }
}

// OTA Upload Handling
otaForm.addEventListener('submit', function (e) {
    if (!fileInput.files.length) {
        e.preventDefault();
        alert('Najpierw wybierz plik .bin z najnowszym oprogramowaniem.');
        return;
    }

    // UI Feedback
    btnSubmitOTA.disabled = true;
    btnSubmitOTA.innerText = '⏳ Wgrywanie... Nie wyłączaj zasialania!';
    uploadStatus.innerText = 'Trwa przesyłanie i zapisywanie do pamięci Flash... (Odśwież stronę po minucie)';
    addLogLine(`Rozpoczeto wgrywanie pliku OTA (${fileInput.files[0].name}). Proszę czekać...`, "ota");

    // Let the form submit naturally as it points to /update handler inside C++.
});

// Mock data generator for offline designing
let lastLightToggle = true;

// Offline memory buffer
const offlineData = {
    temp: 24.5,
    minTemp: 23.8,
    minTempEpoch: Math.floor(Date.now() / 1000) - 3600,
    volt: 4.1,
    pct: 85,
    firstInit: true
};

function getMockData() {
    // Zapamiętaj rzeczywistość jeśli to możliwe, lub użyj łagodnego dryfu by nie skakać jak szalone
    if (elValTemp.innerText && elValTemp.innerText !== "--.-") {
        if (offlineData.firstInit) {
            offlineData.temp = parseFloat(elValTemp.innerText) || 24.5;
            offlineData.pct = parseInt(elValBattPct.innerText) || 85;
            offlineData.volt = parseFloat(elValBattVolt.innerText) || 4.1;
            offlineData.firstInit = false;
        }
    }

    const mockTemp = offlineData.temp + (Math.random() * 0.2 - 0.1);
    offlineData.temp = mockTemp;

    // randomly toggle light
    if (Math.random() > 0.95) {
        lastLightToggle = !lastLightToggle;
    }

    return {
        temperature: {
            current: mockTemp,
            target: 25.0,
            hysteresis: 0.5,
            min: offlineData.minTemp,
            minTimeEpoch: offlineData.minTempEpoch
        },
        battery: {
            voltage: offlineData.volt,
            percent: offlineData.pct
        },
        relays: {
            light: lastLightToggle,
            pump: Math.random() > 0.5
        },
        servo: {
            angle: parseInt(elValServoSlider.value) || 0,
            mode: parseInt(elValServoSlider.value) || 0
        },
        schedule: {
            dayStartHour: parseInt(elSchedDayStart.value.split(':')[0]) || 8,
            dayStartMin: parseInt(elSchedDayStart.value.split(':')[1]) || 0,
            dayEndHour: parseInt(elSchedDayEnd.value.split(':')[0]) || 22,
            dayEndMin: parseInt(elSchedDayEnd.value.split(':')[1]) || 0,
            airStartHour: parseInt(elSchedAirOn.value.split(':')[0]) || 22,
            airStartMin: parseInt(elSchedAirOn.value.split(':')[1]) || 30,
            airEndHour: parseInt(elSchedAirOff.value.split(':')[0]) || 7,
            airEndMin: parseInt(elSchedAirOff.value.split(':')[1]) || 30,
            filterStartHour: parseInt(elSchedFilterOn?.value?.split(':')[0]) || 0,
            filterStartMin: parseInt(elSchedFilterOn?.value?.split(':')[1]) || 0,
            filterEndHour: parseInt(elSchedFilterOff?.value?.split(':')[0]) || 23,
            filterEndMin: parseInt(elSchedFilterOff?.value?.split(':')[1]) || 59,
            servoPreOffMins: parseInt(elValPreOffSlider?.value) || 30
        },
        feeding: {
            hour: parseInt(elSchedFeedTime.value.split(':')[0]) || 12,
            minute: parseInt(elSchedFeedTime.value.split(':')[1]) || 0,
            mode: 1,
            freq: parseInt(elValFeedMode.value) || 1,
            lastFeedEpoch: Math.floor(Date.now() / 1000) - 86400
        },
        network: {
            ip: "Tryb Offline",
            apMode: true
        }
    };
}

// Przeniesiona i zaktualizowana logika logow
let lastKnownNormalLogs = [];
let lastKnownCriticalLogs = [];

function renderStoredLogs() {
    logContainer.innerHTML = '';
    const activeData = currentLogView === 'critical' ? lastKnownCriticalLogs : lastKnownNormalLogs;
    
    if (activeData.length === 0) {
        logContainer.innerHTML = `<div class="log-line system">[SYSTEM] Terminal gotowy. Brak logów.</div>`;
        return;
    }
    
    activeData.forEach(lg => {
        const row = document.createElement('div');
        row.className = currentLogView === 'critical' ? 'log-line danger' : 'log-line system';
        if (currentLogView === 'critical') {
            row.style.color = '#f87171'; // Czerwony kolor dla logów krytycznych
        }
        row.innerText = lg;
        logContainer.appendChild(row);
    });
    logContainer.scrollTop = logContainer.scrollHeight;
}

async function fetchLogs() {
    try {
        const response = await fetch('/api/logs');
        if (!response.ok) return;
        const data = await response.json();
        
        let shouldRender = false;
        
        // Logi systemowe (normalne) z RAM (nadpisywane po resecie)
        if (data.normal && JSON.stringify(data.normal) !== JSON.stringify(lastKnownNormalLogs)) {
            lastKnownNormalLogs = data.normal;
            if (currentLogView === 'normal') shouldRender = true;
        }

        // Logi krytyczne (z Preferences, trwale)
        if (data.critical && JSON.stringify(data.critical) !== JSON.stringify(lastKnownCriticalLogs)) {
            lastKnownCriticalLogs = data.critical;
            if (currentLogView === 'critical') shouldRender = true;
        }
        
        if (shouldRender) {
            renderStoredLogs();
        }

    } catch (e) { }
}

btnDeleteCritical.addEventListener('click', () => {
    if (confirm("Czy na pewno chcesz usunąć trwale zapisane logi krytyczne z pamięci urządzenia?")) {
        sendAction('clear_critical_logs');
        lastKnownCriticalLogs = [];
        renderStoredLogs();
    }
});

// Initial fetch and layout
fetchStatus();
fetchLogs();

// Refresh data every 5 seconds
setInterval(() => {
    fetchStatus();
    fetchLogs();
    // Simulate some system actions in Mock mode to make log active
    if (elConnStatus.className.includes('disconnected') && Math.random() > 0.9) {
        addLogLine("Odebrano weryfikacyjny ping...", "normal");
    }
}, 5000);

// Auto-sync time on load
window.onload = () => {
    syncDeviceTime();
}

)rawliteral";

#endif // WEBASSETS_H
