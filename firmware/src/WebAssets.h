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
    <link rel="stylesheet" href="style.css?v=4">
</head>

<body>
    <div class="container">

        <!-- Header -->
        <header class="app-header">
            <div class="header-main">
                <div class="title-area">
                    <h1 class="title-row">Akwarium đź </h1>
                    <span class="badge" id="otaNetworkMode">--</span>
                </div>
                <div class="system-status">
                    <span id="systemTime" class="clock">--:--:--</span>
                    <div id="connectionStatus" class="conn-status ok">PoĹ‚Ä…czono</div>
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
                            <span class="box-icon">đźŚˇď¸Ź</span>
                            <div class="box-content">
                                <div class="box-title">Temperatura</div>
                                <div class="box-value"><span id="valTemp">--.-</span> <small>Â°C</small></div>
                            </div>
                        </div>
                        <div class="data-compact">
                            <div class="row"><span>Min Temp:</span> <strong><span id="valTempMin">--.- Â°C</span> <span
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
                            <span class="box-icon">đź”‹</span>
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
                            <div class="row"><span>NapiÄ™cie:</span> <strong id="valBattVolt">-.-- V</strong></div>
                        </div>
                    </div>
                </div>

                <!-- Relays/Schedules merged -->
                <div class="panel span-2">
                    <div class="panel-header">UrzÄ…dzenia i Harmonogramy</div>
                    <div class="panel-body no-pad">
                        <table class="data-table devices-table">
                            <thead>
                                <tr>
                                    <th>UrzÄ…dzenie</th>
                                    <th>Status</th>
                                    <th>Harmonogram</th>
                                </tr>
                            </thead>
                            <tbody>
                                <tr id="relayLight">
                                    <td data-label="Urzadzenie">đź’ˇ OĹ›wietlenie</td>
                                    <td data-label="Status">
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indLight"></span>
                                            <strong id="statusLight">WyĹ‚Ä…czone</strong>
                                        </div>
                                    </td>
                                    <td data-label="Harmonogram">
                                        <div class="schedule-edit schedule-edit-wrap">
                                            <select class="input-select" id="schedLightMode">
                                                <option value="0">Harmonogram</option>
                                                <option value="1">Zawsze ON</option>
                                                <option value="2">Zawsze OFF</option>
                                            </select>
                                            <span class="text-muted">ON:</span> <input type="time" step="300" class="input-time" id="schedDayStart">
                                            <span class="text-muted">OFF:</span> <input type="time" step="300" class="input-time" id="schedDayEnd">
                                        </div>
                                        <div class="field-error" id="errLight"></div>
                                    </td>
                                </tr>
                                <tr id="relayPump">
                                    <td data-label="Urzadzenie">đź«§ Filtr</td>
                                    <td data-label="Status">
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indPump"></span>
                                            <strong id="statusPump">WyĹ‚Ä…czone</strong>
                                        </div>
                                    </td>
                                    <td data-label="Harmonogram">
                                        <div class="schedule-edit schedule-edit-wrap">
                                            <select class="input-select" id="schedFilterMode">
                                                <option value="0">Harmonogram</option>
                                                <option value="1">Zawsze ON</option>
                                                <option value="2">Zawsze OFF</option>
                                            </select>
                                            <span class="text-muted">ON:</span> <input type="time" step="300" class="input-time" id="schedFilterOn">
                                            <span class="text-muted">OFF:</span> <input type="time" step="300" class="input-time" id="schedFilterOff">
                                        </div>
                                        <div class="field-error" id="errFilter"></div>
                                    </td>
                                </tr>
                                <tr id="relayTermostat">
                                    <td data-label="Urzadzenie">đźŚˇď¸Ź Termostat</td>
                                    <td data-label="Status">
                                        <div class="status-cell">
                                            <span class="status-dot off" id="indHeater"></span>
                                            <strong id="statusHeater">WyĹ‚Ä…czone</strong>
                                        </div>
                                    </td>
                                    <td data-label="Harmonogram">
                                        <div class="schedule-edit schedule-edit-wrap">
                                            <select class="input-select" id="schedHeaterMode">
                                                <option value="0">Prog</option>
                                                <option value="1">OFF</option>
                                            </select>
                                            <span class="text-muted">Maks:</span> <input type="number" class="input-time input-number-compact" id="schedTargetTemp" step="1" min="18" max="30">
                                            <span class="text-muted">Histereza:</span> <input type="number" class="input-time input-number-compact" id="schedTempHyst" step="0.1" min="0.1" max="5.0">
                                        </div>
                                        <div class="field-error" id="errHeater"></div>
                                    </td>
                                </tr>
                                <tr id="relayAeration">
                                    <td data-label="Urzadzenie">đź’¨ Napowietrzanie</td>
                                    <td data-label="Status">
                                        <div class="status-cell">
                                            <strong id="valServoAngle" style="width:50px; display:inline-block;">--%</strong>
                                            <small>(<span id="statusServoMode">Zbrojny</span>)</small>
                                        </div>
                                    </td>
                                    <td data-label="Harmonogram">
                                        <div class="schedule-edit schedule-edit-wrap">
                                            <select class="input-select" id="schedAirMode">
                                                <option value="0">Harmonogram</option>
                                                <option value="1">Zawsze ON</option>
                                                <option value="2">Zawsze OFF</option>
                                            </select>
                                            <span class="text-muted">ON:</span> <input type="time" step="300" class="input-time" id="schedAirOn">
                                            <span class="text-muted">OFF:</span> <input type="time" step="300" class="input-time" id="schedAirOff">
                                            <input type="range" min="0" max="100" value="0" class="input-range servo-slider" id="valServoSlider">
                                            <input type="hidden" id="valPreOffSlider" value="30">
                                            <span id="valPreOffLabel" style="display:none;"></span>
                                        </div>
                                        <div class="field-error" id="errAeration"></div>
                                    </td>
                                </tr>
                                <tr id="relayServo">
                                    <td data-label="Urzadzenie">âš™ď¸Ź Karmnik</td>
                                    <td data-label="Status">
                                        <div class="status-cell">
                                            <button class="btn btn-small btn-primary" id="btnFeedNow">Karm teraz</button>
                                        </div>
                                    </td>
                                    <td data-label="Harmonogram">
                                        <div class="schedule-edit schedule-edit-wrap">
                                            <input type="time" step="300" class="input-time" id="schedFeedTime">
                                            <select class="input-select" id="valFeedMode">
                                                <option value="1">Codziennie</option>
                                                <option value="2">Co 2 dni</option>
                                                <option value="3">Co 3 dni</option>
                                                <option value="0">WyĹ‚Ä…czony</option>
                                            </select>
                                        </div>
                                        <div class="field-error" id="errFeed"></div>
                                    </td>
                                </tr>
                            </tbody>
                        </table>
                        <div class="schedule-footer">
                            <div class="field-error" id="scheduleValidationSummary"></div>
                            <div class="schedule-actions">
                                <button class="btn btn-small btn-secondary" id="btnSaveSchedules" disabled>Zapisz harmonogramy</button>
                            </div>
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
                    <div class="actions panel-actions">
                        <span class="text-muted last-feed">Ostatnie karmienie: <strong
                                id="valLastFeed" class="text-main-strong">Nigdy</strong></span>
                        <div class="log-toggles">
                            <label class="log-toggle-option"><input type="radio" name="logTypeToggle" value="critical" id="toggleCritical" style="accent-color: var(--danger);"> Krytyczne</label>
                            <label class="log-toggle-option"><input type="radio" name="logTypeToggle" value="normal" id="toggleNormal" checked style="accent-color: var(--accent);"> Systemowe</label>
                        </div>
                        <button class="btn btn-small btn-secondary log-clear-btn" id="btnClearLogs">WyczyĹ›Ä‡ widok</button>
                        <button class="btn btn-small" id="btnDeleteCritical" style="background:var(--danger); color:white; display:none;">UsuĹ„ krytyczne</button>
                        <button class="btn btn-small btn-secondary" id="btnDownloadLogs">Pobierz TXT</button>
                    </div>
                </div>
                <div class="panel-body no-pad">
                    <div class="terminal" id="logContainer">
                        <div class="log-line">[SYSTEM] Terminal gotowy. Brak nowych logĂłw.</div>
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
                        <div class="strip-item"><span>Wersja:</span> <strong id="otaCurrentVersion">--</strong></div>
                    </div>

                    <div class="grid-2 gap-15">
                        <div class="data-compact">
                            <div class="row"><span>Firmware:</span> <strong id="otaFirmwareName">Aquarium Controller</strong></div>
                            <div class="row"><span>Build:</span> <strong id="otaBuildStamp">--</strong></div>
                            <div class="row"><span>Partycje:</span> <strong id="otaPartitions">--</strong></div>
                            <div class="row"><span>ESP-IDF:</span> <strong id="otaIdfVersion">--</strong></div>
                        </div>
                        <div class="data-compact">
                            <div class="row"><span>Stan OTA:</span> <strong id="otaTransportState">IDLE</strong></div>
                            <div class="row"><span>BLE OTA:</span> <strong id="otaBleSupport">--</strong></div>
                            <div class="row"><span>HTTP OTA:</span> <strong id="otaHttpSupport">--</strong></div>
                            <div class="row"><span>Slot OTA:</span> <strong id="otaSlotSize">--</strong></div>
                        </div>
                    </div>

                    <div class="data-compact">
                        <div class="row"><span>Tryb web:</span> <strong>HTTP OTA przez /update</strong></div>
                        <div class="row"><span>Tryb natywny:</span> <strong>BLE OTA w aplikacji Windows / Android</strong></div>
                    </div>

                    <form id="uploadForm" class="upload-form" method="POST" action="/update"
                        enctype="multipart/form-data">
                        <div class="file-dropzone" id="dropzone">
                            <div class="dz-icon">đź“</div>
                            <div class="dz-text">Wybierz lub upuĹ›Ä‡ plik <strong>.bin</strong></div>
                            <input type="file" name="update" accept=".bin" required id="fileInput">
                        </div>

                        <div class="file-selected" id="selectedFile" style="display: none;">
                            Plik: <strong id="fileNameDisplay">brak</strong>
                        </div>

                        <div class="status-msg" style="margin-top: 8px;">BLE OTA wymaga bezpiecznego kontekstu przeglÄ…darki, dlatego panel WWW pozostaje kanaĹ‚em HTTP OTA.</div>

                        <button type="submit" class="btn btn-primary" id="submitBtn">Wgraj aktualizacjÄ™ i
                            zrestartuj</button>
                        <div id="uploadStatus" class="status-msg"></div>
                    </form>

                </div>
            </div>
        </main>

        <footer class="app-footer">
            Made by Bartosz Wolny 2026
        </footer>

    </div>

    <script src="script.js?v=5"></script>
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
    min-height: 100vh;
    -webkit-text-size-adjust: 100%;
}

/* Layout Container */
.container {
    max-width: 1024px;
    margin: 0 auto;
    padding: clamp(12px, 3vw, 20px);
    display: flex;
    flex-direction: column;
    gap: clamp(14px, 3vw, 20px);
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
    gap: 16px;
}

.title-area {
    display: flex;
    flex-direction: column;
    gap: 6px;
    min-width: 0;
}

.title-row {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
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
    overflow-x: auto;
    scrollbar-width: none;
}

.tabs::-webkit-scrollbar {
    display: none;
}

.tab-btn {
    flex: 1 0 140px;
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

@media (max-width: 920px) {
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
    min-width: 0;
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
    min-width: 0;
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
    gap: 12px;
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
    min-height: 42px;
    min-width: 0;
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

.schedule-edit-wrap {
    flex-wrap: wrap;
    row-gap: 10px;
}

.schedule-edit-wrap .input-select {
    flex: 1 1 180px;
}

.schedule-edit-wrap .input-time {
    flex: 1 1 120px;
}

.input-number-compact {
    max-width: 120px;
}

.servo-slider {
    width: 100%;
    margin-top: 15px;
}

.schedule-footer {
    padding: 15px;
    border-top: 1px solid var(--border);
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.schedule-actions {
    display: flex;
    justify-content: flex-end;
}

.schedule-actions .btn {
    min-width: 190px;
}

.field-error {
    margin-top: 6px;
    min-height: 16px;
    color: var(--danger);
    font-size: 0.8rem;
}

.btn {
    padding: 10px 16px;
    border: none;
    border-radius: 6px;
    font-family: inherit;
    font-weight: 500;
    cursor: pointer;
    transition: 0.2s;
    min-height: 42px;
    touch-action: manipulation;
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

.btn:disabled,
.btn:disabled:hover {
    cursor: not-allowed;
    opacity: 0.5;
    background: var(--bg-card);
}

.input-time:disabled,
.input-select:disabled {
    opacity: 0.55;
    cursor: not-allowed;
}

/* OTA Tab */
.info-strip {
    display: flex;
    gap: 15px;
    background: var(--bg-card);
    padding: 12px 20px;
    border-radius: 6px;
    flex-wrap: wrap;
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
    text-align: center;
    margin-top: 4px;
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

.panel-actions {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
    justify-content: flex-end;
}

.last-feed {
    margin-right: auto;
}

.text-main-strong {
    color: var(--text-main);
}

.log-toggles {
    display: flex;
    flex-wrap: wrap;
    gap: 8px 12px;
}

.log-toggle-option {
    cursor: pointer;
}

.log-clear-btn {
    margin-left: auto;
}

.devices-table td:first-child {
    white-space: nowrap;
}

@media (max-width: 780px) {
    .flex-between {
        flex-direction: column;
        align-items: stretch;
        gap: 12px;
    }

    .panel-header,
    .panel-body {
        padding-left: 16px;
        padding-right: 16px;
    }

    .panel-body.no-pad {
        padding: 0;
    }

    .data-compact .row {
        flex-direction: column;
        align-items: flex-start;
        gap: 2px;
    }

    .info-strip {
        flex-direction: column;
    }
}

@media (max-width: 600px) {
    .container {
        padding: 12px;
    }

    .header-main {
        flex-direction: column;
        gap: 10px;
        align-items: flex-start;
    }

    .system-status {
        text-align: left;
    }

    .clock,
    .conn-status {
        justify-content: flex-start;
    }

    .tab-btn {
        flex: 0 0 auto;
        min-width: 132px;
        padding: 13px 16px;
    }

    .box-value {
        font-size: 1.85rem;
    }

    .devices-table,
    .devices-table thead,
    .devices-table tbody,
    .devices-table tr,
    .devices-table td {
        display: block;
        width: 100%;
    }

    .devices-table thead {
        display: none;
    }

    .devices-table tbody {
        padding: 12px;
        display: grid;
        gap: 12px;
    }

    .devices-table tr {
        border: 1px solid var(--border);
        border-radius: 10px;
        overflow: hidden;
        background: rgba(15, 23, 42, 0.55);
    }

    .devices-table td {
        padding: 12px 14px;
        border-bottom: 1px solid var(--border);
    }

    .devices-table td:last-child {
        border-bottom: none;
    }

    .devices-table td::before {
        content: attr(data-label);
        display: block;
        margin-bottom: 6px;
        color: var(--text-muted);
        font-size: 0.72rem;
        font-weight: 600;
        letter-spacing: 0.06em;
        text-transform: uppercase;
    }

    .devices-table td:first-child {
        background: rgba(0, 0, 0, 0.14);
        white-space: normal;
    }

    .status-cell {
        flex-wrap: wrap;
    }

    .schedule-edit-wrap {
        align-items: stretch;
        gap: 10px;
    }

    .schedule-edit-wrap .text-muted,
    .schedule-edit-wrap .input-select,
    .schedule-edit-wrap .input-time,
    .schedule-edit-wrap .input-range,
    .schedule-edit-wrap .btn {
        width: 100%;
        flex-basis: 100%;
    }

    .input-number-compact {
        max-width: none;
    }

    .schedule-actions .btn,
    .panel-actions .btn,
    .panel-actions .btn-small {
        width: 100%;
    }

    .panel-actions,
    .log-toggles {
        align-items: stretch;
    }

    .last-feed,
    .log-clear-btn {
        margin-right: 0;
        margin-left: 0;
    }

    .terminal {
        height: 280px;
        font-size: 0.84rem;
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

const elSchedLightMode = document.getElementById('schedLightMode');
const elSchedDayStart = document.getElementById('schedDayStart');
const elSchedDayEnd = document.getElementById('schedDayEnd');
const elSchedAirMode = document.getElementById('schedAirMode');
const elSchedAirOn = document.getElementById('schedAirOn');
const elSchedAirOff = document.getElementById('schedAirOff');
const elSchedFilterMode = document.getElementById('schedFilterMode');
const elSchedFilterOn = document.getElementById('schedFilterOn');
const elSchedFilterOff = document.getElementById('schedFilterOff');
const elValPreOffSlider = document.getElementById('valPreOffSlider');
const elValPreOffLabel = document.getElementById('valPreOffLabel');
const elSchedHeaterMode = document.getElementById('schedHeaterMode');
const elSchedTargetTemp = document.getElementById('schedTargetTemp');
const elSchedTempHyst = document.getElementById('schedTempHyst');
const indHeater = document.getElementById('indHeater');
const statusHeater = document.getElementById('statusHeater');

const elSchedFeedTime = document.getElementById('schedFeedTime');
const elValFeedMode = document.getElementById('valFeedMode');
const elValLastFeed = document.getElementById('valLastFeed');
const btnSaveSchedules = document.getElementById('btnSaveSchedules');
const scheduleValidationSummary = document.getElementById('scheduleValidationSummary');
const errLight = document.getElementById('errLight');
const errFilter = document.getElementById('errFilter');
const errHeater = document.getElementById('errHeater');
const errAeration = document.getElementById('errAeration');
const errFeed = document.getElementById('errFeed');

// DOM Elements - OTA
const fileInput = document.getElementById('fileInput');
const dropzone = document.getElementById('dropzone');
const selectedFileDiv = document.getElementById('selectedFile');
const fileNameDisplay = document.getElementById('fileNameDisplay');
const otaIpAddress = document.getElementById('otaIpAddress');
const otaNetworkMode = document.getElementById('otaNetworkMode');
const otaCurrentVersion = document.getElementById('otaCurrentVersion');
const otaFirmwareName = document.getElementById('otaFirmwareName');
const otaBuildStamp = document.getElementById('otaBuildStamp');
const otaPartitions = document.getElementById('otaPartitions');
const otaIdfVersion = document.getElementById('otaIdfVersion');
const otaTransportState = document.getElementById('otaTransportState');
const otaBleSupport = document.getElementById('otaBleSupport');
const otaHttpSupport = document.getElementById('otaHttpSupport');
const otaSlotSize = document.getElementById('otaSlotSize');

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

const fallbackValidationProfile = {
    minuteStep: 5,
    temperature: { min: 18, max: 30, step: 1, supportsOff: true },
    hysteresis: { min: 0.1, max: 5.0, step: 0.1 },
    feeding: { modeMin: 0, modeMax: 3 }
};

let currentValidationProfile = fallbackValidationProfile;

function resolveValidationProfile(systemSection) {
    const validation = systemSection?.validation;
    if (!validation) {
        return fallbackValidationProfile;
    }

    return {
        minuteStep: validation.minuteStep ?? fallbackValidationProfile.minuteStep,
        temperature: validation.temperature ?? fallbackValidationProfile.temperature,
        hysteresis: validation.hysteresis ?? fallbackValidationProfile.hysteresis,
        feeding: validation.feeding ?? fallbackValidationProfile.feeding
    };
}

function setFieldError(element, message) {
    if (element) {
        element.textContent = message || '';
    }
}

function modeRequiresSchedule(mode) {
    return String(mode) === '0';
}

function syncScheduleFieldAvailability() {
    const lightActive = modeRequiresSchedule(elSchedLightMode.value);
    elSchedDayStart.disabled = !lightActive;
    elSchedDayEnd.disabled = !lightActive;

    const filterActive = modeRequiresSchedule(elSchedFilterMode.value);
    elSchedFilterOn.disabled = !filterActive;
    elSchedFilterOff.disabled = !filterActive;

    const aerationActive = modeRequiresSchedule(elSchedAirMode.value);
    elSchedAirOn.disabled = !aerationActive;
    elSchedAirOff.disabled = !aerationActive;

    const heaterEnabled = String(elSchedHeaterMode.value) === '0';
    elSchedTargetTemp.disabled = !heaterEnabled;
}

function isMinuteStepValue(timeValue, minuteStep) {
    if (!timeValue || timeValue.length !== 5) {
        return false;
    }

    const [hourText, minuteText] = timeValue.split(':');
    const hour = parseInt(hourText, 10);
    const minute = parseInt(minuteText, 10);

    return Number.isInteger(hour) && Number.isInteger(minute) && hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && minute % minuteStep === 0;
}

function validateScheduleForm() {
    const profile = currentValidationProfile || fallbackValidationProfile;
    const minuteStep = profile.minuteStep || 5;
    const errors = [];

    setFieldError(errLight, '');
    setFieldError(errFilter, '');
    setFieldError(errHeater, '');
    setFieldError(errAeration, '');
    setFieldError(errFeed, '');

    const validateScheduleMode = (modeValue, startInput, endInput, errorElement, label) => {
        if (!modeRequiresSchedule(modeValue)) {
            return;
        }

        if (!isMinuteStepValue(startInput.value, minuteStep) || !isMinuteStepValue(endInput.value, minuteStep)) {
            const message = `${label}: czas musi byc w kroku ${minuteStep} min.`;
            setFieldError(errorElement, message);
            errors.push(message);
        }
    };

    validateScheduleMode(elSchedLightMode.value, elSchedDayStart, elSchedDayEnd, errLight, 'Oswietlenie');
    validateScheduleMode(elSchedFilterMode.value, elSchedFilterOn, elSchedFilterOff, errFilter, 'Filtr');
    validateScheduleMode(elSchedAirMode.value, elSchedAirOn, elSchedAirOff, errAeration, 'Napowietrzanie');

    if (!isMinuteStepValue(elSchedFeedTime.value, minuteStep)) {
        const message = `Karmnik: godzina musi byc w kroku ${minuteStep} min.`;
        setFieldError(errFeed, message);
        errors.push(message);
    }

    const threshold = Number.parseFloat(elSchedTargetTemp.value);
    if (String(elSchedHeaterMode.value) === '0') {
        const tempRule = profile.temperature || fallbackValidationProfile.temperature;
        if (!Number.isFinite(threshold) || threshold < tempRule.min || threshold > tempRule.max || Math.round(threshold) !== threshold) {
            const message = `Termostat: prog musi byc calkowity w zakresie ${tempRule.min}-${tempRule.max}.`;
            setFieldError(errHeater, message);
            errors.push(message);
        }
    }

    const hysteresis = Number.parseFloat(elSchedTempHyst.value);
    const hysteresisRule = profile.hysteresis || fallbackValidationProfile.hysteresis;
    const hysteresisUnits = Math.round(hysteresis / hysteresisRule.step);
    if (!Number.isFinite(hysteresis) || hysteresis < hysteresisRule.min || hysteresis > hysteresisRule.max || Math.abs(hysteresis - (hysteresisUnits * hysteresisRule.step)) > 0.001) {
        const message = `Termostat: histereza musi byc w zakresie ${hysteresisRule.min}-${hysteresisRule.max} ze skokiem ${hysteresisRule.step}.`;
        setFieldError(errHeater, message);
        errors.push(message);
    }

    const feedMode = Number.parseInt(elValFeedMode.value, 10);
    const feedingRule = profile.feeding || fallbackValidationProfile.feeding;
    if (!Number.isInteger(feedMode) || feedMode < feedingRule.modeMin || feedMode > feedingRule.modeMax) {
        const message = `Karmnik: tryb musi byc w zakresie ${feedingRule.modeMin}-${feedingRule.modeMax}.`;
        setFieldError(errFeed, message);
        errors.push(message);
    }

    scheduleValidationSummary.textContent = errors.length > 0 ? errors[0] : '';
    btnSaveSchedules.disabled = errors.length > 0;
    syncScheduleFieldAvailability();
    return errors.length === 0;
}

// Data Fetching
async function fetchStatus() {
    try {
        const response = await fetch(API_STATUS);
        if (!response.ok) throw new Error('Network response was not ok');
        const data = await response.json();

        // Update Connection UI
        if (elConnStatus.className !== 'conn-status ok') {
            elConnStatus.className = 'conn-status ok';
            elConnStatus.textContent = 'PoĹ‚Ä…czono';
            addLogLine("NawiÄ…zano poĹ‚Ä…czenie z API /status", "success");
        }

        updateUI(data);
    } catch (error) {
        if (elConnStatus.className !== 'conn-status err') {
            addLogLine("Brak poĹ‚Ä…czenia z /api/status. PrzeĹ‚Ä…czanie w tryb Mock Data.", "warning");
        }

        elConnStatus.className = 'conn-status err';
        elConnStatus.textContent = 'Tryb Offline';

        // Mock data injection if offline
        updateUI(getMockData());
    }
}

// UI Updater
function updateUI(data) {
    // 1. Temperature
    elValTemp.innerText = data.temperature.current.toFixed(1);
    elValTempMin.innerText = `${(data.temperature.min || 22.0).toFixed(1)} Â°C`;
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
    elStatusServoMode.innerText = data.servo.angle === 0 ? "Otwarty" : data.servo.angle >= 90 ? "ZamkniÄ™ty" : "PĂłĹ‚otwarty";
    // Tylko uaktualniaj suwak jesli uzytkownik go nie przesuwa (zabezpieczenie przed resetem)
    if (!servoSliderLocked) {
        elValServoSlider.value = percent;
    }

    currentValidationProfile = resolveValidationProfile(data.system);

    if (data.relays && typeof data.relays.heater !== 'undefined') {
        indHeater.className = 'status-dot ' + (data.relays.heater ? 'on' : 'off');
        statusHeater.innerText = data.relays.heater ? 'Grzeje' : 'WyĹ‚Ä…czone';
    } else {
        indHeater.className = 'status-dot off';
        statusHeater.innerText = 'WyĹ‚Ä…czone';
    }

    if (document.activeElement !== elSchedLightMode) elSchedLightMode.value = String(data.schedule.lightMode ?? 0);
    if (document.activeElement !== elSchedFilterMode) elSchedFilterMode.value = String(data.schedule.filterMode ?? 0);
    if (document.activeElement !== elSchedAirMode) elSchedAirMode.value = String(data.schedule.airMode ?? 0);
    if (document.activeElement !== elSchedHeaterMode) elSchedHeaterMode.value = String(data.schedule.heaterMode ?? data.temperature.heaterMode ?? 0);
    if (document.activeElement !== elSchedTargetTemp) elSchedTargetTemp.value = data.temperature.threshold ?? data.temperature.target ?? 25.0;
    if (document.activeElement !== elSchedTempHyst) elSchedTempHyst.value = data.temperature.hysteresis || 0.5;

    if (document.activeElement !== elSchedDayStart) elSchedDayStart.value = padTime(data.schedule.dayStartHour, data.schedule.dayStartMin);
    if (document.activeElement !== elSchedDayEnd) elSchedDayEnd.value = padTime(data.schedule.dayEndHour, data.schedule.dayEndMin);
    if (document.activeElement !== elSchedAirOn) elSchedAirOn.value = padTime(data.schedule.airStartHour, data.schedule.airStartMin);
    if (document.activeElement !== elSchedAirOff) elSchedAirOff.value = padTime(data.schedule.airEndHour, data.schedule.airEndMin);
    if (document.activeElement !== elSchedFilterOn && data.schedule.filterStartHour !== undefined) elSchedFilterOn.value = padTime(data.schedule.filterStartHour, data.schedule.filterStartMin);
    if (document.activeElement !== elSchedFilterOff && data.schedule.filterEndHour !== undefined) elSchedFilterOff.value = padTime(data.schedule.filterEndHour, data.schedule.filterEndMin);

    if (document.activeElement !== elValPreOffSlider && data.schedule.servoPreOffMins !== undefined) {
        elValPreOffSlider.value = data.schedule.servoPreOffMins;
        elValPreOffLabel.innerText = data.schedule.servoPreOffMins;
    }

    if (document.activeElement !== elSchedFeedTime) elSchedFeedTime.value = padTime(data.feeding.hour, data.feeding.minute);
    if (document.activeElement !== elValFeedMode) elValFeedMode.value = data.feeding.freq || "1";
    validateScheduleForm();

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

    if (data.system) {
        otaCurrentVersion.innerText = data.system.firmwareVersion || "--";
        otaFirmwareName.innerText = data.system.firmwareName || "Aquarium Controller";
        otaBuildStamp.innerText = `${data.system.buildDate || "--"} ${data.system.buildTime || ""}`.trim();
        otaPartitions.innerText = `${data.system.runningPartition || "--"} -> ${data.system.nextPartition || "--"}`;
        otaIdfVersion.innerText = data.system.idfVersion || "--";
        otaTransportState.innerText = data.system.otaInProgress
            ? `${(data.system.otaTransport || "busy").toUpperCase()} ACTIVE`
            : (data.system.otaTransport || "idle").toUpperCase();
        otaBleSupport.innerText = data.system.bleOtaSupported ? "Tak" : "Nie";
        otaHttpSupport.innerText = data.system.httpOtaSupported ? "Tak" : "Nie";
        otaSlotSize.innerText = data.system.otaPartitionSize
            ? `${(data.system.otaPartitionSize / 1024 / 1024).toFixed(2)} MB`
            : "--";
        otaTransportState.style.color = data.system.otaInProgress ? "var(--warning)" : "var(--success)";
    }
}

function updateRelayUI(isOn, badgeEl, indicatorEl) {
    if (isOn) {
        // Log state change if it was off previously (simplified checking for mock)
        if (badgeEl.textContent === 'WyĹ‚Ä…czone') addLogLine(`${badgeEl.id.replace('status', '')} zostaĹ‚o WĹÄ„CZONE`, "system");

        badgeEl.textContent = 'WĹ‚Ä…czone';
        badgeEl.className = 'status-badge on';
        indicatorEl.style.backgroundColor = 'var(--success)';
        indicatorEl.style.boxShadow = '0 0 15px var(--success)';
    } else {
        if (badgeEl.textContent === 'WĹ‚Ä…czone') addLogLine(`${badgeEl.id.replace('status', '')} zostaĹ‚o WYĹÄ„CZONE`, "system");

        badgeEl.textContent = 'WyĹ‚Ä…czone';
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

    addLogLine(`Ĺ»Ä…danie synchronizacji czasu (Epoch: ${epoch})...`, "system");

    fetch(`${API_TIME}?epoch=${epoch}`, { method: 'POST' })
        .then(response => {
            if (response.ok) {
                addLogLine("Czas zsynchronizowany z ukĹ‚adem ESP32 pomyĹ›lnie.", "success");
            }
        })
        .catch(err => {
            addLogLine("Synchronizacja wymuszona. (Mock Offline)", "warning");
        });
}

// API Action function
function sendAction(actionName, payload = {}) {
    const params = new URLSearchParams({ action: actionName, ...payload });
    addLogLine(`WysyĹ‚anie akcji: ${actionName}...`, "system");

    fetch(`/api/action`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: params.toString()
    })
        .then(async response => {
            const responseText = await response.text();
            if (response.ok) {
                addLogLine(`Akcja '${actionName}' zakoĹ„czona sukcesem.`, "success");
                fetchStatus(); // Refresh immediately
            } else {
                const detail = responseText ? `: ${responseText}` : '';
                addLogLine(`BĹ‚Ä…d akcji '${actionName}' (HTTP ${response.status})${detail}`, "danger");
            }
        })
        .catch(err => addLogLine(`BĹ‚Ä…d sieci (action: ${actionName})`, "danger"));
}

// Button Events
document.getElementById('btnFeedNow').onclick = (e) => {
    e.preventDefault();
    sendAction('feed_now');
};

btnSaveSchedules.onclick = (e) => {
    e.preventDefault();
    if (!validateScheduleForm()) {
        addLogLine('Formularz harmonogramu zawiera bledy walidacji.', 'warning');
        return;
    }

    const lightMode = String(elSchedLightMode.value ?? '0');
    const aerationMode = String(elSchedAirMode.value ?? '0');
    const filterMode = String(elSchedFilterMode.value ?? '0');
    const heaterMode = String(elSchedHeaterMode.value ?? '0');

    const payload = {
        lightMode,
        aerationMode,
        filterMode,
        heaterMode,
        tempHyst: elSchedTempHyst.value,
        servoPreOffMins: elValPreOffSlider.value,
        feedTime: elSchedFeedTime.value,
        feedFreq: elValFeedMode.value
    };

    if (lightMode === '0') {
        payload.dayStart = elSchedDayStart.value;
        payload.dayEnd = elSchedDayEnd.value;
    }

    if (aerationMode === '0') {
        payload.airOn = elSchedAirOn.value;
        payload.airOff = elSchedAirOff.value;
    }

    if (filterMode === '0') {
        payload.filterOn = elSchedFilterOn.value;
        payload.filterOff = elSchedFilterOff.value;
    }

    if (heaterMode === '0') {
        payload.targetTemp = elSchedTargetTemp.value;
    }

    sendAction('save_schedule', payload);
};

[
    elSchedLightMode,
    elSchedDayStart,
    elSchedDayEnd,
    elSchedFilterMode,
    elSchedFilterOn,
    elSchedFilterOff,
    elSchedAirMode,
    elSchedAirOn,
    elSchedAirOff,
    elSchedHeaterMode,
    elSchedTargetTemp,
    elSchedTempHyst,
    elSchedFeedTime,
    elValFeedMode
].forEach(element => {
    element.addEventListener('change', validateScheduleForm);
    element.addEventListener('input', validateScheduleForm);
});

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
    addLogLine("Logi zostaĹ‚y wyczyszczone.", "system");
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
            alert('ProszÄ™ wybraÄ‡ plik z rozszerzeniem .bin');
            fileInput.value = '';
            selectedFileDiv.style.display = 'none';
            return;
        }
        fileNameDisplay.innerText = `${file.name} (${(file.size / 1024 / 1024).toFixed(2)} MB)`;
        selectedFileDiv.style.display = 'block';
        addLogLine(`ZaĹ‚adowano plik firmware: ${file.name}`, "ota");
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
    btnSubmitOTA.innerText = 'âŹł Wgrywanie... Nie wyĹ‚Ä…czaj zasialania!';
    uploadStatus.innerText = 'Trwa przesyĹ‚anie i zapisywanie do pamiÄ™ci Flash... (OdĹ›wieĹĽ stronÄ™ po minucie)';
    addLogLine(`Rozpoczeto wgrywanie pliku OTA (${fileInput.files[0].name}). ProszÄ™ czekaÄ‡...`, "ota");

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
    // ZapamiÄ™taj rzeczywistoĹ›Ä‡ jeĹ›li to moĹĽliwe, lub uĹĽyj Ĺ‚agodnego dryfu by nie skakaÄ‡ jak szalone
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
            threshold: 25.0,
            heaterMode: parseInt(elSchedHeaterMode.value, 10) || 0,
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
            pump: Math.random() > 0.5,
            heater: false
        },
        servo: {
            angle: parseInt(elValServoSlider.value) || 0,
            mode: parseInt(elValServoSlider.value) || 0
        },
        schedule: {
            lightMode: parseInt(elSchedLightMode.value, 10) || 0,
            dayStartHour: parseInt(elSchedDayStart.value.split(':')[0]) || 8,
            dayStartMin: parseInt(elSchedDayStart.value.split(':')[1]) || 0,
            dayEndHour: parseInt(elSchedDayEnd.value.split(':')[0]) || 22,
            dayEndMin: parseInt(elSchedDayEnd.value.split(':')[1]) || 0,
            airMode: parseInt(elSchedAirMode.value, 10) || 0,
            airStartHour: parseInt(elSchedAirOn.value.split(':')[0]) || 22,
            airStartMin: parseInt(elSchedAirOn.value.split(':')[1]) || 30,
            airEndHour: parseInt(elSchedAirOff.value.split(':')[0]) || 7,
            airEndMin: parseInt(elSchedAirOff.value.split(':')[1]) || 30,
            filterMode: parseInt(elSchedFilterMode.value, 10) || 0,
            filterStartHour: parseInt(elSchedFilterOn?.value?.split(':')[0]) || 0,
            filterStartMin: parseInt(elSchedFilterOn?.value?.split(':')[1]) || 0,
            filterEndHour: parseInt(elSchedFilterOff?.value?.split(':')[0]) || 23,
            filterEndMin: parseInt(elSchedFilterOff?.value?.split(':')[1]) || 59,
            heaterMode: parseInt(elSchedHeaterMode.value, 10) || 0,
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
        },
        system: {
            firmwareName: "Aquarium Controller",
            firmwareVersion: "2.0.0",
            buildDate: "Mar 07 2026",
            buildTime: "00:00:00",
            idfVersion: "v4.4.7-dirty",
            runningPartition: "app0",
            nextPartition: "app1",
            otaPartitionSize: 1966080,
            otaInProgress: false,
            otaTransport: "idle",
            bleOtaSupported: true,
            httpOtaSupported: true,
            validation: fallbackValidationProfile
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
        logContainer.innerHTML = `<div class="log-line system">[SYSTEM] Terminal gotowy. Brak logĂłw.</div>`;
        return;
    }
    
    activeData.forEach(lg => {
        const row = document.createElement('div');
        row.className = currentLogView === 'critical' ? 'log-line danger' : 'log-line system';
        if (currentLogView === 'critical') {
            row.style.color = '#f87171'; // Czerwony kolor dla logĂłw krytycznych
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
    if (confirm("Czy na pewno chcesz usunÄ…Ä‡ trwale zapisane logi krytyczne z pamiÄ™ci urzÄ…dzenia?")) {
        sendAction('clear_critical_logs');
        lastKnownCriticalLogs = [];
        renderStoredLogs();
    }
});

// Initial fetch and layout
syncScheduleFieldAvailability();
validateScheduleForm();
fetchStatus();
fetchLogs();

// Refresh data every 5 seconds
setInterval(() => {
    fetchStatus();
    fetchLogs();
    // Simulate some system actions in Mock mode to make log active
    if (elConnStatus.className.includes('err') && Math.random() > 0.9) {
        addLogLine("Odebrano weryfikacyjny ping...", "normal");
    }
}, 5000);

// Auto-sync time on load
window.onload = () => {
    syncDeviceTime();
}

)rawliteral";

#endif // WEBASSETS_H

