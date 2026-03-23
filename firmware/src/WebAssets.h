#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

const char web_index_html[] PROGMEM = R"AQWEB(
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aquarium Controller ESP32</title>
    <link rel="stylesheet" href="style.css?v=20260323d">
    <style>
        /* Custom Time Inputs (Pills) */
        input[type="time"].time-pill {
            background: rgba(11, 19, 36, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.15);
            color: #f8fafc;
            border-radius: 12px;
            padding: 4px 6px;
            font-size: 11px;
            cursor: pointer;
            font-family: inherit;
            outline: none;
            transition: background 0.2s, border-color 0.2s;
            text-align: center;
        }
        input[type="time"].time-pill:hover {
            background: rgba(0, 0, 0, 0.9);
            border-color: rgba(255, 255, 255, 0.3);
        }
        input[type="time"].time-pill::-webkit-calendar-picker-indicator {
            filter: invert(0.6) sepia(1) saturate(5) hue-rotate(175deg);
            cursor: pointer;
            width: 12px;
            height: 12px;
            padding: 0;
            margin-left: 2px;
        }
        
        /* Temp Chart Styles */
        .temp-chart {
            display: flex;
            align-items: flex-end;
            justify-content: space-between;
            height: 120px;
            gap: 8px;
            padding-top: 35px;
            margin-bottom: 10px;
            position: relative;
        }
        .target-temp-line {
            position: absolute;
            left: 0;
            width: 100%;
            height: 1px;
            border-top: 1px dashed rgba(255, 255, 255, 0.6);
            z-index: 5;
            pointer-events: none;
        }
        .target-temp-label {
            position: absolute;
            right: 0;
            top: -22px;
            font-size: 10px;
            font-weight: 500;
            color: rgba(255, 255, 255, 0.9);
            background: rgba(0,0,0,0.6);
            padding: 4px 8px;
            border-radius: 6px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .hysteresis-zone {
            position: absolute;
            left: 0;
            width: 100%;
            background: rgba(34, 211, 238, 0.05);
            z-index: 4;
            pointer-events: none;
            border-top: 1px solid rgba(34, 211, 238, 0.2);
            border-bottom: 1px solid rgba(34, 211, 238, 0.2);
        }
        .temp-bar-wrap {
            flex: 1;
            display: flex;
            align-items: flex-end;
            height: 100%;
        }
        .temp-bar {
            width: 100%;
            background: rgba(139, 92, 246, 0.6);
            border-radius: 6px 6px 2px 2px;
            transition: all 0.3s;
            min-height: 8px;
            z-index: 10;
        }
        .temp-bar:hover {
            background: rgba(139, 92, 246, 0.9);
            transform: scaleY(1.05);
            transform-origin: bottom;
        }
        .temp-bar.active {
            background: var(--accent-cyan);
            box-shadow: 0 0 12px rgba(34, 211, 238, 0.6);
        }
        .temp-bar.hot {
            background: rgba(251, 146, 60, 0.85);
            box-shadow: 0 0 12px rgba(251, 146, 60, 0.4);
        }

        /* Module Badges Additions */
        .pulse-dot-cyan {
            width: 8px;
            height: 8px;
            background: var(--accent-cyan);
            border-radius: 50%;
            box-shadow: 0 0 10px var(--accent-cyan);
            animation: pulse-cyan 2s infinite;
        }
        @keyframes pulse-cyan {
            0% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(34, 211, 238, 0.7); }
            70% { transform: scale(1); box-shadow: 0 0 0 6px rgba(34, 211, 238, 0); }
            100% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(34, 211, 238, 0); }
        }
    </style>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
</head>
<body>
    <div class="app-container">
        <!-- Sidebar Navigation -->
        <nav class="sidebar">
            <div class="brand">
                <i class="fa-solid fa-water fa-bounce" style="--fa-animation-duration: 3s; color: var(--accent-cyan);"></i>
                <div class="brand-text">
                    <h1>AquaSync</h1>
                    <span>ESP32-S3 System</span>
                </div>
            </div>
            
            <ul class="nav-menu">
                <li class="nav-item active" data-target="dashboard">
                    <a href="#"><i class="fa-solid fa-gauge-high"></i> Dashboard</a>
                </li>
                <li class="nav-item" data-target="harmonogramy">
                    <a href="#"><i class="fa-solid fa-calendar-days"></i> Harmonogramy</a>
                </li>
                <li class="nav-item" data-target="logi">
                    <a href="#"><i class="fa-solid fa-terminal"></i> Logi Systemu</a>
                </li>
            </ul>

            <div class="sidebar-bottom">
                <ul class="nav-menu">
                    <li class="nav-item" data-target="ota">
                        <a href="#"><i class="fa-solid fa-cloud-arrow-up"></i> OTA Aktualizacja</a>
                    </li>
                    <li class="nav-item" data-target="ustawienia">
                        <a href="#"><i class="fa-solid fa-gear"></i> Ustawienia</a>
                    </li>
                </ul>
                <div class="sys-version">
                    v4.2.0-stable<br>
                    <span>FreeRTOS Core</span>
                </div>
            </div>
        </nav>

        <!-- Main Content -->
        <main class="main-content">
            <!-- Topbar -->
            <header class="topbar">
                <div class="topbar-activity">
                    <span class="topbar-label">AKTYWNE MODUŁY</span>
                    <div id="topbar-active-list" class="topbar-active-list">
                        <div class="status-badge status-badge-muted">
                            <span class="pulse-dot pulse-dot-muted"></span>
                            Oczekiwanie na dane sterownika
                        </div>
                    </div>
                </div>

                <div class="topbar-widgets">
                    <div class="info-pill battery-pill" title="Aktualne napięcie baterii">
                        <i class="fa-solid fa-battery-three-quarters"></i>
                        <span id="rtc-battery">--.--V</span>
                    </div>
                    <div class="time-widget">
                        <span id="current-time">00:00:00</span>
                        <span id="current-date">01 sty 2026</span>
                    </div>
                </div>
            </header>

            <!-- Dashboard View -->
            <section id="dashboard" class="view-section active">
                <div class="view-header">
                    <h2>Przegląd Systemu</h2>
                    <p>Bieżący status wszystkich urządzeń i sensorów.</p>
                </div>

                <div class="dashboard-grid">
                    <div class="card glass dashboard-card metric-card">
                        <span class="eyebrow">TEMPERATURA</span>
                        <div class="metric-value">
                            <span id="dashboard-temp-current">--.-</span>
                            <span class="unit">&deg;C</span>
                        </div>
                        <div class="metric-subline">
                            <span>Cel <strong id="dashboard-temp-target">25.0&deg;C</strong></span>
                            <span>Histereza <strong id="dashboard-temp-hysteresis">0.5&deg;C</strong></span>
                        </div>
                    </div>

                    <div class="card glass dashboard-card metric-card">
                        <span class="eyebrow">BATERIA</span>
                        <div class="metric-value">
                            <span id="dashboard-battery-percent">0</span>
                            <span class="unit">%</span>
                        </div>
                        <div class="metric-subline">
                            <span>Napięcie <strong id="dashboard-battery-voltage">--.--V</strong></span>
                            <span id="dashboard-battery-state">Brak pomiaru</span>
                        </div>
                        <div class="progress-bar battery-progress">
                            <div id="dashboard-battery-fill" class="progress-fill battery-fill" style="width: 0%;"></div>
                        </div>
                    </div>

                    <div id="network-card" class="card glass dashboard-card network-card network-offline">
                        <span class="eyebrow">SIEĆ</span>
                        <div id="network-status" class="metric-value network-status">OFFLINE</div>
                        <div class="network-details">
                            <div class="network-detail-row">
                                <span>SSID STA</span>
                                <strong id="network-ssid">-</strong>
                            </div>
                            <div class="network-detail-row">
                                <span>Ostatnie połączenie</span>
                                <strong id="network-last-seen">Brak historii</strong>
                            </div>
                        </div>
                    </div>

                    <div class="card glass dashboard-card relay-panel">
                        <div class="card-header compact-header">
                            <span class="card-heading">Przekaźniki</span>
                            <span id="relay-count" class="card-chip">0 / 4 aktywne</span>
                        </div>
                        <div class="relay-grid">
                            <article id="relay-light" class="relay-status relay-standby">
                                <div class="relay-status-top">
                                    <span class="relay-icon-shell"><i class="fa-solid fa-lightbulb"></i></span>
                                    <span id="relay-light-state" class="relay-state-badge">STANDBY</span>
                                </div>
                                <div class="relay-name">Światło</div>
                                <div id="relay-light-meta" class="relay-meta">Harmonogram</div>
                            </article>

                            <article id="relay-filter" class="relay-status relay-standby">
                                <div class="relay-status-top">
                                    <span class="relay-icon-shell"><i class="fa-solid fa-filter"></i></span>
                                    <span id="relay-filter-state" class="relay-state-badge">STANDBY</span>
                                </div>
                                <div class="relay-name">Filtr</div>
                                <div id="relay-filter-meta" class="relay-meta">Harmonogram</div>
                            </article>

                            <article id="relay-heater" class="relay-status relay-standby">
                                <div class="relay-status-top">
                                    <span class="relay-icon-shell"><i class="fa-solid fa-temperature-half"></i></span>
                                    <span id="relay-heater-state" class="relay-state-badge">STANDBY</span>
                                </div>
                                <div class="relay-name">Grzałka</div>
                                <div id="relay-heater-meta" class="relay-meta">Czeka na próg</div>
                            </article>

                            <article id="relay-aeration" class="relay-status relay-standby">
                                <div class="relay-status-top">
                                    <span class="relay-icon-shell"><i class="fa-solid fa-wind"></i></span>
                                    <span id="relay-aeration-state" class="relay-state-badge">STANDBY</span>
                                </div>
                                <div class="relay-name">Napowietrzanie</div>
                                <div id="relay-aeration-meta" class="relay-meta">Harmonogram</div>
                            </article>
                        </div>
                    </div>

                    <div class="card glass dashboard-card schedule-panel">
                        <div class="card-header compact-header">
                            <span class="card-heading">Harmonogram dzisiaj</span>
                            <button class="btn btn-secondary btn-pill" onclick="switchTab('harmonogramy')">Edytuj</button>
                        </div>
                        <div id="today-schedule-list" class="schedule-summary-list">
                            <div class="schedule-summary-item">
                                <span>Ładowanie danych</span>
                                <strong>...</strong>
                            </div>
                        </div>
                    </div>

                    <div class="card glass dashboard-card feeder-card">
                        <span class="card-heading">Karmnik</span>
                        <div class="feeder-orb">
                            <button id="feed-now-btn" class="feed-now-btn" onclick="triggerFeed()">Karm teraz</button>
                        </div>
                        <span id="feed-next-label" class="feeder-note">Codziennie 18:00</span>
                        <span id="feed-last-label" class="feeder-subnote">Ostatnie karmienie: brak danych</span>
                        <button class="btn btn-secondary feeder-manage-btn" onclick="switchTab('harmonogramy')">Zarządzaj</button>
                    </div>

                    <div class="card glass dashboard-card chart-wide temp-history-card">
                        <div class="temp-chart-header">
                            <span class="card-heading">Zakres temperatury</span>
                            <span id="temperature-chart-meta" class="card-chip card-chip-accent">20 ostatnich pomiarów</span>
                        </div>

                        <div class="temp-chart-shell">
                            <div class="temp-chart-legend">
                                <span><span class="legend-line legend-line-live"></span>Temperatura</span>
                                <span><span class="legend-line legend-line-target"></span>Docelowa</span>
                                <span><span class="legend-line legend-line-hysteresis"></span>Histereza</span>
                            </div>
                            <div id="temperature-chart-empty" class="temp-chart-empty">Oczekiwanie na historię temperatur ze sterownika...</div>
                            <svg id="temperature-chart-svg" class="temp-line-chart" viewBox="0 0 960 320" preserveAspectRatio="none" aria-label="Wykres temperatury"></svg>
                            <div id="temperature-chart-tooltip" class="temp-chart-tooltip" hidden></div>
                            <div class="temp-chart-axis">
                                <span id="temperature-chart-start">Najstarszy pomiar</span>
                                <span id="temperature-chart-end">Teraz</span>
                            </div>
                        </div>
                    </div>
                </div>
            </section>

            <!-- Harmonogramy View -->
            <section id="harmonogramy" class="view-section">
                <div class="view-header">
                    <h2>Harmonogramy</h2>
                    <p>Zarządzaj cyklem dobowym wszystkich urządzeń akwarium.</p>
                </div>
                
                <div class="timeline-container glass p-4">
                    <h3 class="mh-2 mb-4">Grafik Pracy (24h)</h3>
                    
                    <div class="schedule-item" data-schedule-kind="range">
                        <div class="schedule-icon"><i class="fa-regular fa-lightbulb" style="color: var(--accent-yellow);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Oświetlenie Główne</div>
                                <select class="form-control schedule-mode-select" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączony</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączony</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" style="margin-bottom: 30px;">
                                <div class="schedule-bar" style="background: var(--accent-yellow);"></div>
                                <input type="time" class="time-pill schedule-time-start" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="10:00">
                                <input type="time" class="time-pill schedule-time-end" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="21:30">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4" data-schedule-kind="range">
                        <div class="schedule-icon"><i class="fa-solid fa-wind" style="color: var(--accent-white);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Napowietrzanie</div>
                                <select class="form-control schedule-mode-select" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączony</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączony</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" style="margin-bottom: 30px;">
                                <div class="schedule-bar" style="background: var(--accent-white);"></div>
                                <input type="time" class="time-pill schedule-time-start" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="10:00">
                                <input type="time" class="time-pill schedule-time-end" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="19:00">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4" data-schedule-kind="range">
                        <div class="schedule-icon"><i class="fa-solid fa-filter" style="color: var(--accent-blue);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Filtracja</div>
                                <select class="form-control schedule-mode-select" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączony</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączony</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" style="margin-bottom: 30px;">
                                <div class="schedule-bar" style="background: var(--accent-blue);"></div>
                                <input type="time" class="time-pill schedule-time-start" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="10:30">
                                <input type="time" class="time-pill schedule-time-end" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="20:30">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4">
                        <div class="schedule-icon"><i class="fa-solid fa-fish" style="color: var(--accent-cyan);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Karmienie (Automatyczne)</div>
                                <select class="form-control" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="wylaczone">Wyłączone</option>
                                    <option value="codziennie" selected>Codziennie</option>
                                    <option value="co_2_dni">Co 2 dni</option>
                                    <option value="co_3_dni">Co 3 dni</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" style="margin-bottom: 30px;">
                                <div class="schedule-point" style="left: 75%; background: var(--accent-cyan);"></div>
                                <input type="time" class="time-pill schedule-time-point" style="position: absolute; top: 16px; transform: translateX(-50%); width: 75px;" value="18:00">
                            </div>
                        </div>
                    </div>
                </div>
            </section>

            <!-- Logi View -->
            <section id="logi" class="view-section">
                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 30px;">
                    <div class="view-header" style="margin-bottom: 0;">
                        <h2>Logi systemowe</h2>
                        <p>Biezace logi aplikacji, komendy i wpisy krytyczne.</p>
                    </div>
                    <div style="display: flex; gap: 15px;">
                        <button id="clear-logs-btn" class="btn btn-secondary" style="border-radius: 12px; padding: 10px 24px; font-weight: 500; font-size: 13px; background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.1);">Wyczysc widok</button>
                        <button id="delete-critical-btn" class="btn btn-primary" style="background: rgba(239, 68, 68, 0.15); color: #ef4444; border: 1px solid rgba(239, 68, 68, 0.3); border-radius: 12px; padding: 10px 24px; font-weight: 500; font-size: 13px; transition: 0.2s;" onmouseover="this.style.background='rgba(239, 68, 68, 0.25)'" onmouseout="this.style.background='rgba(239, 68, 68, 0.15)'">Usun krytyczne</button>
                    </div>
                </div>
                
                <div style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; margin-bottom: 30px;">
                    <div class="card glass" style="padding: 20px; border-radius: 16px;">
                        <div style="font-size: 11px; font-weight: 600; color: var(--text-muted); margin-bottom: 10px; letter-spacing: 1px;">INFO</div>
                        <div id="info-count" style="font-size: 28px; font-weight: 600; color: var(--accent-cyan);">2</div>
                    </div>
                    <div class="card glass" style="padding: 20px; border-radius: 16px;">
                        <div style="font-size: 11px; font-weight: 600; color: var(--text-muted); margin-bottom: 10px; letter-spacing: 1px;">KRYTYCZNYCH</div>
                        <div id="critical-count" style="font-size: 28px; font-weight: 600; color: #ef4444;">0</div>
                    </div>
                </div>
            
                <div class="card glass" style="padding: 20px; border-radius: 16px; min-height: 500px;">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;">
                        <div style="display: flex; gap: 10px; align-items: center;">
                            <button id="logs-current-btn" class="btn btn-secondary active" style="padding: 8px 16px; border-radius: 12px; border: 1px solid rgba(6, 182, 212, 0.4); color: var(--accent-cyan); font-size: 13px; font-weight: 600; background: rgba(6, 182, 212, 0.1);">Biezace</button>
                            <button id="logs-critical-btn" class="btn btn-secondary" style="padding: 8px 16px; border-radius: 12px; border: 1px solid transparent; color: var(--text-main); font-size: 13px; font-weight: 600; transition: 0.2s;" onmouseover="this.style.background='rgba(239, 68, 68, 0.15)'; this.style.color='#ef4444';" onmouseout="this.style.background='transparent'; this.style.color='var(--text-main)';">Krytyczne</button>
                            <span id="logs-status" style="font-size: 13px; color: var(--text-muted); margin-left: 10px;">Brak odpowiedzi sterownika.</span>
                        </div>
                        <div style="display: flex; gap: 10px; align-items: center;">
                            <input id="logs-search" type="text" placeholder="Szukaj..." style="background: rgba(0,0,0,0.3); border: 1px solid var(--glass-border); border-radius: 8px; padding: 10px 16px; color: var(--text-main); font-size: 13px; width: 250px; outline: none;">
                            <button id="download-logs-btn" class="btn btn-secondary" style="padding: 10px 14px; border-radius: 8px; border: 1px solid var(--glass-border); background: rgba(255,255,255,0.03);" title="Pobierz logi"><i class="fa-solid fa-download" style="color: var(--accent-cyan);"></i></button>
                        </div>
                    </div>
            
                    <div id="logs-list" style="display: flex; flex-direction: column; gap: 10px;">
                        <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 14px 20px; display: flex; align-items: center; font-size: 13px;">
                            <span style="color: var(--accent-cyan); font-weight: 600; width: 80px;">INFO</span>
                            <span style="color: var(--text-muted); width: 100px;">17:54:08</span>
                            <span style="color: var(--text-main);">Streaming logow firmware nie jest obecnie wystawiony przez kontrakt BLE. Widok pokazuje logi aplikacji i wyniki komend.</span>
                        </div>
                        <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 14px 20px; display: flex; align-items: center; font-size: 13px;">
                            <span style="color: var(--accent-cyan); font-weight: 600; width: 80px;">INFO</span>
                            <span style="color: var(--text-muted); width: 100px;">17:54:08</span>
                            <span style="color: var(--text-main);">Panel BLE uruchomiony. Oczekiwanie na polaczenie.</span>
                        </div>
                    </div>
                </div>
            </section>


            <!-- OTA View -->
            <section id="ota" class="view-section">
                <div class="view-header">
                    <h2>OTA Aktualizacja</h2>
                    <p>Wgraj nowy plik .bin firmware'u do mikrokontrolera ESP32-S3.</p>
                </div>
                
                <div class="card glass" style="max-width: 640px; margin: 0 auto; padding: 50px 40px; border: 1px solid rgba(255,255,255,0.05); border-radius: 24px; box-shadow: 0 20px 40px rgba(0,0,0,0.3);">
                    <div class="card-body" style="text-align: center;">
                        <div style="width: 80px; height: 80px; background: rgba(59, 130, 246, 0.1); border-radius: 50%; display: flex; align-items: center; justify-content: center; margin: 0 auto 24px auto;">
                            <i class="fa-solid fa-cloud-arrow-up fa-2x" style="color: var(--accent-blue);"></i>
                        </div>
                        
                        <h3 class="mb-2" style="font-size: 24px; font-weight: 600; color: #f8fafc;">Instalacja nowej wersji</h2>
                        <p class="text-muted mb-4" style="font-size: 14px; max-width: 400px; margin-left: auto; margin-right: auto; line-height: 1.5;">Prześlij skompilowany plik <code style="background: rgba(255,255,255,0.05); padding: 2px 6px; border-radius: 4px; font-family: monospace; color: var(--accent-cyan);">.bin</code> aby zaktualizować oprogramowanie bazowe mikrokontrolera ESP32-S3 sterującego akwarium.</p>
                        
                        <div class="file-upload-wrapper mb-4" style="border: 2px dashed rgba(255,255,255,0.15); border-radius: 16px; padding: 40px 20px; transition: border-color 0.3s, background 0.3s; cursor: pointer; background: rgba(0,0,0,0.2);" onmouseover="this.style.borderColor='var(--accent-blue)'; this.style.background='rgba(59, 130, 246, 0.05)';" onmouseout="this.style.borderColor='rgba(255,255,255,0.15)'; this.style.background='rgba(0,0,0,0.2)';" onclick="document.getElementById('firmware-file').click()">
                            <input type="file" id="firmware-file" accept=".bin" style="display: none;">
                            <i class="fa-solid fa-file-arrow-up mb-3" style="font-size: 32px; color: var(--text-muted);"></i>
                            <div style="font-size: 16px; font-weight: 500; color: var(--text-main); margin-bottom: 8px;">Przeciągnij plik tutaj lub kliknij</div>
                            <div style="font-size: 12px; color: var(--text-muted);">Maksymalny rozmiar pliku: 4 MB</div>
                        </div>

                        <div id="ota-progress" style="display: none; width: 100%; text-align: left; background: rgba(0,0,0,0.2); padding: 20px; border-radius: 16px; border: 1px solid rgba(255,255,255,0.05); margin-bottom: 24px;">
                            <div class="progress-label mb-2" style="display: flex; justify-content: space-between; font-size: 14px; font-weight: 500;">
                                <span style="color: var(--accent-cyan);">Wgrywanie aktualizacji... <i class="fa-solid fa-circle-notch fa-spin ml-2"></i></span>
                                <span id="ota-percent" style="color: var(--text-main);">0%</span>
                            </div>
                            <div class="progress-bar mb-3" style="height: 8px; background: rgba(255,255,255,0.1); border-radius: 4px; overflow: hidden;">
                                <div class="progress-fill" id="ota-fill" style="width: 0%; height: 100%; background: linear-gradient(90deg, var(--accent-blue), var(--accent-cyan)); border-radius: 4px; transition: width 0.3s ease;"></div>
                            </div>
                            <p class="text-muted" style="font-size: 12px; margin: 0;"><i class="fa-solid fa-triangle-exclamation" style="color: var(--accent-yellow); margin-right: 6px;"></i> Proszę nie wyłączać urządzenia ani nie odświeżać strony w trakcie aktualizacji.</p>
                        </div>
                        
                        <button class="btn btn-primary w-100" id="upload-btn" onclick="simulateOTA()" disabled style="padding: 14px; font-size: 15px; background: linear-gradient(135deg, var(--accent-blue), #2563eb); border: none; font-weight: 600; text-transform: uppercase; letter-spacing: 1px; border-radius: 12px; transition: 0.3s; opacity: 0.5;">
                            <i class="fa-solid fa-microchip" style="margin-right: 8px;"></i> Rozpocznij Aktualizację
                        </button>
                    </div>
                </div>
            </section>

            <!-- Ustawienia View -->
            <section id="ustawienia" class="view-section">
                <div class="view-header">
                    <h2>Ustawienia Systemu</h2>
                    <p>Konfiguracja urządzenia, sieci WiFi i preferencji sprzętowych.</p>
                </div>
                
                <div class="dashboard-grid">
                    <!-- WiFi config -->
                    <div class="card glass">
                        <div class="card-header">
                            <div class="card-title">
                                <i class="fa-solid fa-wifi" style="color: var(--success-color);"></i>
                                <h2>Sieć WiFi</h2>
                            </div>
                        </div>
                        <div class="card-body form-group">
                            <label>Aktywne SSID (Station):</label>
                            <input type="text" class="form-control mb-3" value="MojaSiec_5G" disabled>
                            
                            <label>Przydzielony Adres IP:</label>
                            <input type="text" class="form-control mb-4" value="192.168.1.144" disabled>
                            
                            <button class="btn btn-secondary w-100"><i class="fa-solid fa-satellite-dish"></i> Skanuj i Konfiguruj Sieci AP</button>
                        </div>
                    </div>
                    
                    <!-- Temp config -->
                    <div class="card glass">
                        <div class="card-header" style="justify-content: space-between; align-items: center;">
                            <div class="card-title">
                                <i class="fa-solid fa-temperature-half" style="color: var(--accent-orange);"></i>
                                <h2>Automatyka Temp.</h2>
                            </div>
                            <label class="switch" title="Włącz/Wyłącz Regulację">
                                <input type="checkbox" checked>
                                <span class="slider round"></span>
                            </label>
                        </div>
                        <div class="card-body form-group">
                            <div class="grid-2-col mb-4">
                                <div>
                                    <label>Cel Temp. (°C):</label>
                                    <input type="number" class="form-control" step="0.5" value="25.0">
                                </div>
                                <div>
                                    <label>Histereza (°C):</label>
                                    <input type="number" class="form-control" step="0.1" value="0.5">
                                </div>
                            </div>
                            
                            <button class="btn btn-primary w-100"><i class="fa-solid fa-floppy-disk"></i> Zapisz Ustawienia Temp.</button>
                        </div>
                    </div>

                    <!-- System Info -->
                    <div class="card glass">
                        <div class="card-header">
                            <div class="card-title">
                                <i class="fa-solid fa-circle-info" style="color: var(--accent-blue);"></i>
                                <h2>Informacje Systemowe</h2>
                            </div>
                        </div>
                        <div class="card-body" style="gap: 12px;">
                            <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 12px 16px; display: flex; justify-content: space-between; align-items: center;">
                                <span class="text-muted" style="font-size: 13px;">Wersja Oprogramowania:</span>
                                <span style="font-weight: 600; color: var(--text-main); font-size: 13px;">v4.2.0-stable</span>
                            </div>
                            <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 12px 16px; display: flex; justify-content: space-between; align-items: center;">
                                <span class="text-muted" style="font-size: 13px;">Środowisko RTOS:</span>
                                <span style="font-weight: 600; color: var(--text-main); font-size: 13px;">FreeRTOS Core 1</span>
                            </div>
                            <div style="background: rgba(255,255,255,0.02); border: 1px solid rgba(255,255,255,0.05); border-radius: 8px; padding: 12px 16px; display: flex; justify-content: space-between; align-items: center;">
                                <span class="text-muted" style="font-size: 13px;">Czas pracy (Uptime):</span>
                                <span style="font-weight: 600; color: var(--text-main); font-size: 13px;">14 dni, 08:31:02</span>
                            </div>
                        </div>
                    </div>

                    <!-- Time Setup -->
                    <div class="card glass">
                        <div class="card-header">
                            <div class="card-title">
                                <i class="fa-solid fa-clock" style="color: var(--accent-magenta);"></i>
                                <h2>Ustawienia Zegara RTC</h2>
                            </div>
                        </div>
                        <div class="card-body form-group">
                            <label>Ostatnia synchronizacja RTC (DS3231):</label>
                            <input type="text" class="form-control mb-3" value="11 mar 2026, 18:00" disabled>
                            
                            <div class="grid-2-col">
                                <button class="btn btn-secondary w-100" style="padding: 10px 14px; font-size: 13px;"><i class="fa-solid fa-arrows-rotate"></i> Synch. z NTP</button>
                                <button class="btn btn-secondary w-100" style="padding: 10px 14px; font-size: 13px;"><i class="fa-solid fa-laptop"></i> Pobierz Czas Przeglądarki</button>
                            </div>
                        </div>
                    </div>

                    <!-- System actions -->
                    <div class="card glass span-2">
                        <div class="card-header">
                            <div class="card-title">
                                <i class="fa-solid fa-microchip" style="color: var(--danger-color);"></i>
                                <h2>Zarządzanie Urządzeniem</h2>
                            </div>
                        </div>
                        <div class="card-body" style="gap: 15px;">
                            <div style="margin-bottom: 5px;">
                                <p style="font-size: 13px; color: var(--text-muted); line-height: 1.5; max-width: 700px;">
                                    <strong>Restart ESP32</strong> wymusza ponowne uruchomienie systemu z zachowaniem wszystkich ustawień i harmonogramów w uPamięci (NVS).<br>
                                    <strong>Przywrócenie Ustawień Fabrycznych</strong> (hard reset) trwale kasuje wszystkie dane na urządzeniu (w tym dane logowania WiFi AP oraz ustawioną automatykę temp/karmnika).
                                </p>
                            </div>
                            <div class="grid-2-col">
                                <button class="btn btn-secondary w-100" style="padding: 14px; font-size: 14px;"><i class="fa-solid fa-rotate-right"></i> Zrestartuj Sterownik</button>
                                <button class="btn btn-primary outline w-100" style="color: #ef4444; border-color: rgba(239, 68, 68, 0.4); padding: 14px; background: rgba(239, 68, 68, 0.05); font-size: 14px;"><i class="fa-solid fa-triangle-exclamation"></i> Przywróć Ustawienia Fabryczne</button>
                            </div>
                        </div>
                    </div>
                </div>
            </section>
        </main>
    </div>

    <!-- Feedback Modal -->
    <div class="overlay" id="feed-modal">
        <div class="modal glass">
            <div class="modal-content">
                <i class="fa-solid fa-spinner fa-spin fa-2xl" id="modal-icon" style="color: var(--accent-cyan); margin-bottom: 20px;"></i>
                <h3 id="modal-text">Trwa karmienie...</h3>
                <p id="modal-subtext">Sensor położenia w trakcie odczytu</p>
            </div>
        </div>
    </div>

    <script src="script.js?v=20260323d"></script>
</body>
</html>

)AQWEB";

const char web_style_css[] PROGMEM = R"AQSTYLE(
:root {
    /* Color Palette */
    --bg-dark: #030712;
    --glass-bg: rgba(10, 15, 30, 0.55);
    --glass-border: rgba(255, 255, 255, 0.12);
    --glass-highlight: rgba(255, 255, 255, 0.05);
    
    --text-main: #f8fafc;
    --text-muted: #cbd5e1;
    
    --accent-cyan: #22d3ee;
    --accent-blue: #3b82f6;
    --accent-orange: #fb923c;
    --accent-yellow: #fde047;
    --accent-white: #f8fafc;
    
    --success-color: #10b981;
    --danger-color: #ef4444;
    --warning-color: #f59e0b;

    /* Shadow & Effects */
    --shadow-sm: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
    --shadow-md: 0 10px 25px -5px rgba(0, 0, 0, 0.6);
    --glow-cyan: 0 0 20px rgba(34, 211, 238, 0.4);
    
    /* Layout */
    --sidebar-width: 304px;
    --border-radius-lg: 24px;
    --border-radius-md: 16px;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
    font-family: 'Inter', sans-serif;
}


i.fa-solid,
i.fa-regular {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 1em;
    height: 1em;
    font-style: normal;
    line-height: 1;
    vertical-align: -0.125em;
}

i.fa-solid svg,
i.fa-regular svg {
    width: 100%;
    height: 100%;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
    overflow: visible;
}

.fa-2x { font-size: 2em; }
.fa-2xl { font-size: 2.35em; }
.fa-spin { animation: fa-local-spin 1.2s linear infinite; }
.fa-bounce {
    transform-origin: center bottom;
    animation: fa-local-bounce 1.8s ease infinite;
}

@keyframes fa-local-spin {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
}

@keyframes fa-local-bounce {
    0%, 100% { transform: translateY(0); }
    30% { transform: translateY(-18%); }
    45% { transform: translateY(0); }
    60% { transform: translateY(-8%); }
}

body {
    position: relative;
    background-color: var(--bg-dark);
    background-image:
        radial-gradient(circle at 18% 12%, rgba(34, 211, 238, 0.1), transparent 24%),
        radial-gradient(circle at 82% 18%, rgba(59, 130, 246, 0.12), transparent 28%),
        linear-gradient(180deg, rgba(15, 28, 46, 0.92) 0%, rgba(7, 14, 24, 0.92) 45%, rgba(3, 7, 18, 1) 100%);
    background-size: cover;
    background-position: center;
    background-attachment: fixed;
    color: var(--text-main);
    min-height: 100vh;
    overflow-x: hidden;
}

body::before {
    content: "";
    position: fixed;
    inset: 0;
    background:
        radial-gradient(circle at 72% 64%, rgba(203, 213, 225, 0.1), transparent 18%),
        radial-gradient(circle at 40% 78%, rgba(34, 211, 238, 0.12), transparent 20%),
        linear-gradient(180deg, transparent 52%, rgba(7, 14, 24, 0.06) 58%, rgba(7, 14, 24, 0.65) 100%);
    pointer-events: none;
    z-index: 0;
}

body::after {
    content: "";
    position: fixed;
    left: -10vw;
    right: -10vw;
    bottom: -12vh;
    height: 46vh;
    background:
        radial-gradient(120% 110% at 50% 100%, rgba(12, 21, 36, 0.98) 34%, rgba(12, 21, 36, 0.92) 47%, rgba(12, 21, 36, 0) 48%),
        radial-gradient(60% 38% at 50% 64%, rgba(148, 163, 184, 0.16) 0, rgba(148, 163, 184, 0.03) 36%, rgba(148, 163, 184, 0) 60%),
        linear-gradient(180deg, rgba(24, 42, 68, 0.6), rgba(5, 12, 22, 0.92));
    filter: blur(14px);
    opacity: 0.95;
    pointer-events: none;
    z-index: 0;
}

/* Glassmorphism utility */
.glass {
    background: var(--glass-bg);
    backdrop-filter: blur(16px);
    -webkit-backdrop-filter: blur(16px);
    border: 1px solid var(--glass-border);
    border-radius: var(--border-radius-lg);
    box-shadow: var(--shadow-md);
}

.app-container {
    display: flex;
    min-height: 100vh;
    position: relative;
    z-index: 1;
}

/* Sidebar */
.sidebar {
    width: var(--sidebar-width);
    background: rgba(11, 15, 30, 0.6);
    backdrop-filter: blur(20px);
    border-right: 1px solid var(--glass-border);
    display: flex;
    flex-direction: column;
    padding: 24px;
    position: fixed;
    height: 100vh;
    z-index: 100;
}

.brand {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-bottom: 48px;
}

.brand i {
    font-size: 28px;
}

.brand h1 {
    font-size: 22px;
    font-weight: 700;
    letter-spacing: 0.5px;
    background: linear-gradient(135deg, #fff, #94a3b8);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
}

.brand span {
    font-size: 11px;
    color: var(--accent-cyan);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 600;
}

.nav-menu {
    list-style: none;
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.nav-item a {
    text-decoration: none;
    color: var(--text-muted);
    font-size: 14px;
    font-weight: 500;
    padding: 12px 16px;
    border-radius: var(--border-radius-md);
    display: flex;
    align-items: center;
    gap: 12px;
    transition: all 0.3s ease;
}

.nav-item a i {
    font-size: 16px;
    width: 20px;
    text-align: center;
}

.nav-item:hover a, .nav-item.active a {
    background: var(--glass-highlight);
    color: var(--text-main);
}

.nav-item.active a {
    background: rgba(6, 182, 212, 0.1);
    color: var(--accent-cyan);
    border-left: 3px solid var(--accent-cyan);
}

.sidebar-bottom {
    margin-top: auto;
    padding-top: 24px;
    border-top: 1px solid var(--glass-border);
}

.sys-version {
    margin-top: 24px;
    font-size: 12px;
    color: var(--text-muted);
    text-align: center;
}
.sys-version span {
    font-size: 10px;
    opacity: 0.6;
}

/* Main Content */
.main-content {
    flex: 1;
    margin-left: var(--sidebar-width);
    padding: 32px 48px 48px;
    width: calc(100vw - var(--sidebar-width));
    max-width: none;
    position: relative;
    z-index: 2;
}

/* Topbar */
.topbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 40px;
}

.status-badge {
    display: flex;
    align-items: center;
    gap: 10px;
    background: var(--glass-bg);
    padding: 8px 16px;
    border-radius: 30px;
    border: 1px solid var(--glass-border);
    font-size: 14px;
    font-weight: 500;
}

.pulse-dot {
    width: 10px;
    height: 10px;
    background: var(--success-color);
    border-radius: 50%;
    box-shadow: 0 0 10px var(--success-color);
    animation: pulse 2s infinite;
}

@keyframes pulse {
    0% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0.7); }
    70% { transform: scale(1); box-shadow: 0 0 0 6px rgba(16, 185, 129, 0); }
    100% { transform: scale(0.95); box-shadow: 0 0 0 0 rgba(16, 185, 129, 0); }
}

.topbar-widgets {
    display: flex;
    align-items: center;
    gap: 16px;
}

.info-pill {
    display: flex;
    align-items: center;
    gap: 8px;
    background: var(--glass-bg);
    padding: 8px 14px;
    border-radius: 20px;
    border: 1px solid var(--glass-border);
    font-size: 13px;
    color: var(--text-muted);
}

.time-widget {
    text-align: right;
    display: flex;
    flex-direction: column;
}

#current-time {
    font-size: 20px;
    font-weight: 600;
    letter-spacing: 1px;
}

#current-date {
    font-size: 12px;
    color: var(--text-muted);
}

/* Grid Layout */
.dashboard-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 24px;
    align-items: start;
}

.card.span-2 {
    grid-column: span 2;
}
@media (max-width: 1200px) {
    .card.span-2 { grid-column: span 1; }
}

/* Cards */
.card {
    display: flex;
    flex-direction: column;
    padding: 24px;
    transition: transform 0.3s ease, border-color 0.3s ease;
}

.card:hover {
    border-color: rgba(255, 255, 255, 0.15);
    transform: translateY(-2px);
}

.card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

.card-title {
    display: flex;
    align-items: center;
    gap: 12px;
}

.card-title i {
    font-size: 20px;
}

.card-title h2 {
    font-size: 16px;
    font-weight: 600;
    color: var(--text-main);
}

.card-body {
    flex: 1;
    display: flex;
    flex-direction: column;
}

/* Temp Card */
.temp-display {
    text-align: center;
    margin: 10px 0;
}

.temp-value {
    font-size: 48px;
    font-weight: 700;
    color: var(--text-main);
    line-height: 1;
    text-shadow: 0 0 20px rgba(249, 115, 22, 0.3);
}

.temp-value .unit {
    font-size: 24px;
    font-weight: 400;
    color: var(--text-muted);
}

.temp-target {
    font-size: 14px;
    color: var(--text-muted);
    margin-top: 8px;
}

.temp-target span {
    color: var(--accent-cyan);
    font-weight: 600;
}

.sparkline path {
    animation: dash 5s linear forwards;
}

@keyframes dash {
    from { stroke-dasharray: 1000; stroke-dashoffset: 1000; }
    to { stroke-dasharray: 1000; stroke-dashoffset: 0; }
}

/* Info Rows */
.info-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 0;
    border-bottom: 1px solid var(--glass-border);
    font-size: 14px;
}
.info-row:last-of-type { border-bottom: none; }
.info-row span:first-child { color: var(--text-muted); }
.info-row span:last-child { font-weight: 500; }

.value-highlight {
    color: var(--accent-yellow);
    text-shadow: 0 0 10px rgba(250, 204, 21, 0.4);
}

/* Progress bar */
.progress-bar-container { width: 100%; }
.progress-label {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    color: var(--text-muted);
    margin-bottom: 6px;
}
.progress-bar {
    width: 100%;
    height: 6px;
    background: rgba(0,0,0,0.3);
    border-radius: 3px;
    overflow: hidden;
}
.progress-fill {
    height: 100%;
    border-radius: 3px;
    transition: width 1s ease;
}

.mt-3 { margin-top: 16px; }
.w-100 { width: 100%; }

/* Buttons */
.btn {
    padding: 10px 16px;
    border-radius: 8px;
    border: none;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
}

.btn-primary {
    background: var(--accent-blue);
    color: #fff;
    box-shadow: 0 4px 12px rgba(59, 130, 246, 0.3);
}

.btn-primary:hover {
    background: #2563eb;
    transform: translateY(-1px);
}

.btn-primary.outline {
    background: transparent;
    border: 1px solid var(--accent-blue);
    color: var(--accent-blue);
    box-shadow: none;
}
.btn-primary.outline:hover {
    background: rgba(59, 130, 246, 0.1);
}

.btn-secondary {
    background: rgba(255, 255, 255, 0.05);
    color: var(--text-main);
    border: 1px solid var(--glass-border);
}

.btn-secondary:hover {
    background: rgba(255, 255, 255, 0.1);
}

.btn-icon {
    background: transparent;
    border: none;
    color: var(--text-muted);
    cursor: pointer;
    font-size: 16px;
    transition: color 0.2s;
}
.btn-icon:hover { color: var(--text-main); }

/* Switches */
.switch {
    position: relative;
    display: inline-block;
    width: 44px;
    height: 24px;
}
.switch input { opacity: 0; width: 0; height: 0; }
.slider {
    position: absolute;
    cursor: pointer;
    top: 0; left: 0; right: 0; bottom: 0;
    background-color: rgba(255,255,255,0.1);
    transition: .4s;
    border: 1px solid var(--glass-border);
}
.slider:before {
    position: absolute;
    content: "";
    height: 18px;
    width: 18px;
    left: 2px;
    bottom: 2px;
    background-color: var(--text-muted);
    transition: .4s;
}
input:checked + .slider {
    background-color: var(--accent-cyan);
    border-color: var(--accent-cyan);
    box-shadow: var(--glow-cyan);
}
input:checked + .slider:before {
    transform: translateX(20px);
    background-color: #fff;
}
.slider.round { border-radius: 24px; }
.slider.round:before { border-radius: 50%; }

/* Labels */
.status-label {
    font-size: 12px;
    padding: 4px 10px;
    border-radius: 12px;
    font-weight: 600;
}
.status-label.warning {
    background: rgba(245, 158, 11, 0.2);
    color: var(--warning-color);
    border: 1px solid rgba(245, 158, 11, 0.3);
}
.status-label.success {
    background: rgba(16, 185, 129, 0.2);
    color: var(--success-color);
    border: 1px solid rgba(16, 185, 129, 0.3);
}
.status-label.danger {
    background: rgba(239, 68, 68, 0.2);
    color: var(--danger-color);
    border: 1px solid rgba(239, 68, 68, 0.3);
}

.status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    display: inline-block;
}
.status-dot.success {
    background: var(--success-color);
    box-shadow: 0 0 8px var(--success-color);
}

/* Features */
.feed-status {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 15px;
    background: rgba(0,0,0,0.2);
    border-radius: var(--border-radius-md);
}

.feed-timer span {
    font-size: 12px;
    color: var(--text-muted);
}

.feed-timer h3 {
    font-size: 20px;
    color: var(--accent-cyan);
    margin-top: 4px;
}

/* Water animation for Filter */
.water-animation {
    position: relative;
    height: 100px;
    background: rgba(0,0,0,0.2);
    border-radius: var(--border-radius-md);
    overflow: hidden;
    margin-top: 10px;
}

.wave {
    position: absolute;
    width: 200%;
    height: 200%;
    background: rgba(6, 182, 212, 0.2);
    border-radius: 40%;
    bottom: -150%;
    left: -50%;
    animation: rotate 6s linear infinite;
}

.wave2 {
    background: rgba(59, 130, 246, 0.2);
    animation: rotate 8s linear infinite reverse;
    bottom: -160%;
}

@keyframes rotate {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
}

.water-info {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    flex-direction: column;
    align-items: center;
    z-index: 10;
    width: 100%;
}
.water-info span {
    font-size: 13px;
    color: var(--text-main);
}
.water-info b { color: var(--accent-cyan); }


/* System Info */
.grid-2-col {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 16px;
}
.sys-stat {
    display: flex;
    flex-direction: column;
    padding: 12px;
    background: rgba(255,255,255,0.02);
    border-radius: 8px;
    border: 1px solid rgba(255,255,255,0.05);
}
.stat-label {
    font-size: 12px;
    color: var(--text-muted);
    margin-bottom: 4px;
}
.stat-value {
    font-size: 15px;
    font-weight: 500;
}

/* Modal */
.overlay {
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: rgba(0,0,0,0.7);
    backdrop-filter: blur(5px);
    display: none;
    justify-content: center;
    align-items: center;
    z-index: 1000;
}

.modal {
    padding: 40px;
    text-align: center;
    border-radius: var(--border-radius-lg);
    max-width: 400px;
    width: 90%;
    border: 1px solid rgba(6, 182, 212, 0.3);
    box-shadow: var(--glow-cyan);
}

.modal h3 {
    margin-bottom: 8px;
}

.modal p {
    color: var(--text-muted);
    font-size: 14px;
}

/* Views & Tabs */
.view-section {
    display: none;
    animation: fadeIn 0.4s ease-out;
}

.view-section.active {
    display: block;
}

@keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
}

.view-header {
    margin-bottom: 30px;
}
.view-header h2 {
    font-size: 24px;
    font-weight: 600;
    color: var(--text-main);
    margin-bottom: 8px;
}
.view-header p {
    color: var(--text-muted);
    font-size: 14px;
}

/* Forms & Inputs */
.form-group label {
    display: block;
    font-size: 13px;
    color: var(--text-muted);
    margin-bottom: 6px;
    font-weight: 500;
}
.form-control {
    width: 100%;
    background: rgba(0,0,0,0.3);
    border: 1px solid var(--glass-border);
    color: var(--text-main);
    padding: 10px 14px;
    border-radius: 8px;
    font-size: 14px;
    transition: border-color 0.2s;
}
.form-control:focus {
    outline: none;
    border-color: var(--accent-cyan);
}
.form-control:disabled {
    opacity: 0.6;
    cursor: not-allowed;
}

.mb-2 { margin-bottom: 8px; }
.mb-3 { margin-bottom: 16px; }
.mb-4 { margin-bottom: 24px; }
.text-muted { color: var(--text-muted); }

/* Timeline Container (Harmonogramy) */
.timeline-container {
    max-width: none;
    width: 100%;
}
.p-4 { padding: 24px; }
.mh-2 { font-size: 18px; font-weight: 600; }

.schedule-item {
    display: flex;
    gap: 16px;
    align-items: center;
}
.schedule-icon {
    width: 40px;
    height: 40px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(255,255,255,0.05);
    border-radius: 50%;
    border: 1px solid var(--glass-border);
    font-size: 18px;
}
.schedule-details {
    flex: 1;
}
.schedule-title {
    font-size: 14px;
    font-weight: 500;
    margin-bottom: 8px;
}
.schedule-bar-container {
    height: 8px;
    background: rgba(0,0,0,0.3);
    border-radius: 4px;
    position: relative;
    margin-bottom: 8px;
}
.schedule-bar {
    position: absolute;
    height: 100%;
    border-radius: 4px;
    box-shadow: 0 0 10px rgba(255,255,255,0.2);
}
.schedule-point {
    position: absolute;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    box-shadow: 0 0 10px rgba(6, 182, 212, 0.4);
}
.schedule-times {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    color: var(--text-muted);
}
.schedule-times.point-time {
    position: relative;
    height: 12px;
}
.schedule-times.point-time span {
    position: absolute;
    transform: translateX(-50%);
}

.mt-4 { margin-top: 24px; }

/* Terminal Layout (Logi) */
.terminal {
    max-width: none;
    width: 100%;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}
.terminal-header {
    background: rgba(0,0,0,0.5);
    padding: 12px 16px;
    display: flex;
    align-items: center;
    border-bottom: 1px solid var(--glass-border);
}
.terminal-dots {
    display: flex;
    gap: 6px;
}
.terminal-dots span {
    width: 12px;
    height: 12px;
    border-radius: 50%;
}
.terminal-title {
    flex: 1;
    text-align: center;
    font-size: 13px;
    color: var(--text-muted);
    font-family: monospace;
}
.terminal-body {
    background: rgba(10, 15, 25, 0.8);
    padding: 16px;
    height: 400px;
    overflow-y: auto;
    font-family: 'Consolas', 'Courier New', monospace;
    font-size: 13px;
    line-height: 1.6;
}
.terminal-body::-webkit-scrollbar {
    width: 8px;
}
.terminal-body::-webkit-scrollbar-track {
    background: rgba(0,0,0,0.2);
}
.terminal-body::-webkit-scrollbar-thumb {
    background: var(--glass-border);
    border-radius: 4px;
}
.log-line {
    margin-bottom: 4px;
    word-break: break-all;
}
.log-time { color: #888; }
.log-info { color: #4facfe; }
.log-warn { color: #f59e0b; }
.log-success { color: #10b981; }
.log-error { color: #ef4444; }
.log-msg { color: #e2e8f0; }

/* File Upload */
.file-upload-wrapper {
    width: 100%;
    margin-top: 20px;
}

.inline-icon {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 1em;
    height: 1em;
    flex: 0 0 auto;
}

.inline-icon svg {
    width: 100%;
    height: 100%;
    stroke: currentColor;
    stroke-width: 1.9;
    stroke-linecap: round;
    stroke-linejoin: round;
}

.topbar {
    gap: 24px;
    flex-wrap: wrap;
    align-items: flex-start;
}

.topbar-activity {
    display: flex;
    flex-direction: column;
    gap: 12px;
    min-width: 0;
    flex: 1 1 540px;
}

.topbar-label {
    font-size: 11px;
    font-weight: 700;
    color: var(--text-muted);
    letter-spacing: 1.4px;
}

.topbar-active-list {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
}

.status-badge-muted {
    background: rgba(148, 163, 184, 0.08);
    border-color: rgba(148, 163, 184, 0.18);
    color: #cbd5e1;
}

.status-badge-success {
    background: rgba(16, 185, 129, 0.12);
    border-color: rgba(16, 185, 129, 0.28);
    color: #6ee7b7;
}

.status-badge-cyan {
    background: rgba(34, 211, 238, 0.12);
    border-color: rgba(34, 211, 238, 0.28);
    color: #67e8f9;
}

.status-badge-blue {
    background: rgba(59, 130, 246, 0.12);
    border-color: rgba(59, 130, 246, 0.28);
    color: #93c5fd;
}

.status-badge-orange {
    background: rgba(249, 115, 22, 0.12);
    border-color: rgba(249, 115, 22, 0.28);
    color: #fdba74;
}

.pulse-dot-muted {
    background: #94a3b8;
    box-shadow: 0 0 10px rgba(148, 163, 184, 0.45);
}

.battery-pill {
    color: var(--text-main);
}

.battery-pill i {
    color: var(--accent-cyan);
}

.dashboard-card {
    min-height: 210px;
    gap: 16px;
}

.compact-header {
    margin-bottom: 2px;
}

.eyebrow {
    display: block;
    font-size: 11px;
    color: var(--text-muted);
    font-weight: 700;
    letter-spacing: 1.4px;
}

.metric-card .metric-value {
    display: flex;
    align-items: baseline;
    gap: 6px;
    font-size: 44px;
    font-weight: 700;
    line-height: 1;
    margin-top: 4px;
}

.metric-card .metric-value .unit {
    font-size: 24px;
    font-weight: 500;
    color: var(--text-muted);
}

.metric-subline {
    display: flex;
    justify-content: space-between;
    gap: 16px;
    flex-wrap: wrap;
    font-size: 13px;
    color: var(--text-muted);
    margin-top: auto;
}

.metric-subline strong {
    color: var(--text-main);
}

.battery-progress {
    height: 8px;
    border-radius: 999px;
}

.battery-fill {
    width: 0;
    border-radius: 999px;
    background: linear-gradient(90deg, #10b981, #34d399);
    box-shadow: 0 0 16px rgba(52, 211, 153, 0.4);
}

.network-card .network-status {
    font-size: 34px;
    line-height: 1;
}

.network-card.network-online .network-status {
    color: #6ee7b7;
}

.network-card.network-aponly .network-status {
    color: #67e8f9;
}

.network-card.network-offline .network-status {
    color: #e5e7eb;
}

.network-details {
    display: flex;
    flex-direction: column;
    gap: 12px;
    margin-top: auto;
}

.network-detail-row {
    display: flex;
    justify-content: space-between;
    gap: 14px;
    padding-top: 12px;
    border-top: 1px solid rgba(255, 255, 255, 0.08);
    font-size: 13px;
}

.network-detail-row span {
    color: var(--text-muted);
}

.network-detail-row strong {
    color: var(--text-main);
    text-align: right;
}

.card-heading {
    font-size: 16px;
    font-weight: 600;
    color: var(--text-main);
}

.card-chip {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: 30px;
    padding: 4px 12px;
    border-radius: 999px;
    font-size: 12px;
    color: var(--text-muted);
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.08);
}

.card-chip-accent {
    color: var(--accent-cyan);
    border-color: rgba(34, 211, 238, 0.22);
    background: rgba(34, 211, 238, 0.08);
}

.btn-pill {
    padding: 8px 16px;
    border-radius: 999px;
    font-size: 13px;
}

.relay-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
}

.relay-status {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 16px;
    border-radius: 18px;
    border: 1px solid rgba(255, 255, 255, 0.08);
    background: rgba(255, 255, 255, 0.03);
    transition: transform 0.2s ease, border-color 0.2s ease, background 0.2s ease;
}

.relay-status:hover {
    transform: translateY(-2px);
}

.relay-status-top {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
}

.relay-icon-shell {
    width: 42px;
    height: 42px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border-radius: 14px;
    background: rgba(255, 255, 255, 0.06);
    border: 1px solid rgba(255, 255, 255, 0.08);
    font-size: 18px;
}

.relay-state-badge {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: 28px;
    padding: 4px 10px;
    border-radius: 999px;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
}

.relay-name {
    font-size: 15px;
    font-weight: 600;
}

.relay-meta {
    font-size: 12px;
    color: var(--text-muted);
    line-height: 1.5;
}

.relay-on {
    background: linear-gradient(180deg, rgba(16, 185, 129, 0.18), rgba(16, 185, 129, 0.08));
    border-color: rgba(16, 185, 129, 0.32);
}

.relay-on .relay-icon-shell {
    color: #6ee7b7;
    background: rgba(16, 185, 129, 0.14);
    border-color: rgba(16, 185, 129, 0.3);
}

.relay-on .relay-state-badge {
    color: #6ee7b7;
    background: rgba(16, 185, 129, 0.16);
    border: 1px solid rgba(16, 185, 129, 0.26);
}

.relay-off {
    background: linear-gradient(180deg, rgba(239, 68, 68, 0.12), rgba(239, 68, 68, 0.05));
    border-color: rgba(239, 68, 68, 0.24);
}

.relay-off .relay-icon-shell {
    color: #fca5a5;
    background: rgba(239, 68, 68, 0.12);
    border-color: rgba(239, 68, 68, 0.24);
}

.relay-off .relay-state-badge {
    color: #fca5a5;
    background: rgba(239, 68, 68, 0.14);
    border: 1px solid rgba(239, 68, 68, 0.22);
}

.relay-standby {
    background: linear-gradient(180deg, rgba(148, 163, 184, 0.08), rgba(148, 163, 184, 0.03));
    border-color: rgba(148, 163, 184, 0.2);
}

.relay-standby .relay-icon-shell {
    color: #cbd5e1;
}

.relay-standby .relay-state-badge {
    color: #cbd5e1;
    background: rgba(148, 163, 184, 0.12);
    border: 1px solid rgba(148, 163, 184, 0.18);
}

.schedule-summary-list {
    display: flex;
    flex-direction: column;
    gap: 10px;
}

.schedule-summary-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 14px 16px;
    border-radius: 16px;
    border: 1px solid rgba(255, 255, 255, 0.08);
    background: rgba(255, 255, 255, 0.03);
    font-size: 13px;
}

.schedule-summary-item span {
    color: var(--text-main);
    font-weight: 500;
}

.schedule-summary-item strong {
    color: var(--text-muted);
    text-align: right;
}

.feeder-card {
    align-items: center;
    justify-content: flex-start;
}

.feeder-card .card-heading {
    align-self: flex-start;
}

.feeder-orb {
    width: 152px;
    height: 152px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 50%;
    background: radial-gradient(circle at 30% 30%, rgba(16, 185, 129, 0.18), rgba(5, 15, 12, 0.96));
    border: 1px solid rgba(16, 185, 129, 0.34);
    box-shadow: inset 0 0 18px rgba(0, 0, 0, 0.45), 0 0 30px rgba(16, 185, 129, 0.14);
}

.feed-now-btn {
    width: 116px;
    height: 116px;
    border: none;
    border-radius: 50%;
    background: linear-gradient(180deg, #16a34a, #15803d);
    color: #ecfdf5;
    font-size: 15px;
    font-weight: 700;
    cursor: pointer;
    transition: transform 0.2s ease, box-shadow 0.2s ease, filter 0.2s ease;
    box-shadow: 0 10px 30px rgba(21, 128, 61, 0.34);
}

.feed-now-btn:hover {
    transform: translateY(-2px) scale(1.02);
    filter: brightness(1.05);
    box-shadow: 0 12px 32px rgba(21, 128, 61, 0.42);
}

.feed-now-btn:active {
    transform: scale(0.98);
}

.feed-now-btn:disabled {
    cursor: wait;
    opacity: 0.8;
    filter: saturate(0.85);
}

.feeder-note {
    font-size: 13px;
    color: var(--text-main);
}

.feeder-subnote {
    font-size: 12px;
    color: var(--text-muted);
    text-align: center;
    line-height: 1.5;
}

.feeder-manage-btn {
    min-width: 132px;
    border-radius: 16px;
}

.chart-wide {
    grid-column: 1 / -1;
}

.temp-history-card {
    min-height: auto;
}

.temp-chart-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 16px;
    flex-wrap: wrap;
}

.temp-chart-shell {
    position: relative;
    min-height: 360px;
    padding: 18px;
    border-radius: 24px;
    background: linear-gradient(180deg, rgba(5, 11, 22, 0.68), rgba(8, 14, 28, 0.9));
    border: 1px solid rgba(255, 255, 255, 0.06);
}

.temp-chart-legend {
    display: flex;
    flex-wrap: wrap;
    gap: 18px;
    font-size: 12px;
    color: var(--text-muted);
    margin-bottom: 14px;
}

.legend-line {
    display: inline-block;
    width: 22px;
    margin-right: 8px;
    vertical-align: middle;
    border-top: 2px solid currentColor;
}

.legend-line-live {
    color: var(--accent-cyan);
}

.legend-line-target {
    color: #f8fafc;
    border-top-style: dashed;
}

.legend-line-hysteresis {
    color: var(--accent-orange);
    border-top-style: dashed;
}

.temp-chart-empty {
    position: absolute;
    inset: 64px 18px 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 18px;
    border: 1px dashed rgba(255, 255, 255, 0.12);
    color: var(--text-muted);
    font-size: 14px;
    text-align: center;
    padding: 16px;
}

.temp-line-chart {
    width: 100%;
    height: 270px;
    display: block;
}

.temp-chart-tooltip {
    position: absolute;
    min-width: 132px;
    max-width: 180px;
    padding: 10px 12px;
    border-radius: 14px;
    background: rgba(3, 7, 18, 0.94);
    border: 1px solid rgba(34, 211, 238, 0.2);
    color: var(--text-main);
    font-size: 12px;
    line-height: 1.45;
    box-shadow: 0 14px 30px rgba(0, 0, 0, 0.38);
    pointer-events: none;
    z-index: 3;
}

.temp-chart-tooltip strong {
    display: block;
    color: var(--accent-cyan);
    font-size: 13px;
    margin-bottom: 2px;
}

.chart-grid-line {
    stroke: rgba(148, 163, 184, 0.16);
    stroke-width: 1;
}

.chart-grid-label {
    fill: rgba(203, 213, 225, 0.72);
    font-size: 12px;
    font-weight: 500;
}

.chart-line {
    fill: none;
    stroke: var(--accent-cyan);
    stroke-width: 3;
    stroke-linecap: round;
    stroke-linejoin: round;
    filter: drop-shadow(0 0 10px rgba(34, 211, 238, 0.3));
}

.chart-area {
    fill: url(#chart-area-gradient);
    opacity: 0.95;
}

.chart-target-line {
    fill: none;
    stroke: rgba(248, 250, 252, 0.85);
    stroke-width: 2;
    stroke-dasharray: 10 8;
}

.chart-hysteresis-line {
    fill: none;
    stroke: rgba(251, 146, 60, 0.8);
    stroke-width: 2;
    stroke-dasharray: 6 8;
}

.chart-guide-label {
    fill: rgba(248, 250, 252, 0.85);
    font-size: 12px;
    font-weight: 600;
}

.chart-guide-label.hysteresis {
    fill: rgba(251, 191, 36, 0.95);
}

.chart-point {
    fill: #0f172a;
    stroke: var(--accent-cyan);
    stroke-width: 3;
}

.chart-point-hit {
    cursor: pointer;
    fill: transparent;
}

.temp-chart-axis {
    display: flex;
    justify-content: space-between;
    gap: 12px;
    margin-top: 10px;
    font-size: 12px;
    color: var(--text-muted);
}

@media (max-width: 1200px) {
    .dashboard-grid {
        grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .topbar-widgets {
        width: 100%;
        justify-content: space-between;
    }
}

@media (max-width: 900px) {
    .topbar {
        flex-direction: column;
    }

    .dashboard-grid {
        grid-template-columns: 1fr;
    }

    .relay-grid {
        grid-template-columns: 1fr;
    }

    .metric-card .metric-value {
        font-size: 38px;
    }

    .network-card .network-status {
        font-size: 30px;
    }

    .temp-chart-shell {
        min-height: 320px;
        padding: 14px;
    }

    .temp-line-chart {
        height: 230px;
    }
}

[data-target] { cursor: pointer; }

)AQSTYLE";

const char web_script_js[] PROGMEM = R"AQSCRIPT(
const API_STATUS = '/api/status';
const API_ACTION = '/api/action';
const API_LOGS = '/api/logs';
const API_OTA = '/update';

let backendConnected = false;
let activeLogType = 'normal';
let cachedLogs = { normal: [], critical: [] };
let deviceClockBaseDate = null;
let deviceClockSyncedAtMs = 0;
let lastStatusData = null;

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
    const points = rawHistory
        .map((item, index) => ({
            value: toFiniteNumber(item?.value),
            epoch: toFiniteNumber(item?.epoch),
            index
        }))
        .filter((item) => isValidTemperature(item.value))
        .slice(-20)
        .map((item, index, arr) => ({
            value: item.value,
            epoch: item.epoch,
            valueLabel: `${Number(item.value).toFixed(2)}°C`,
            timeLabel: formatEpoch(item.epoch, {
                fallback: `Pomiar ${index + 1} z ${arr.length}`,
                includeDate: false,
                includeSeconds: true
            })
        }));

    setText('temperature-chart-meta', `${points.length || 0} / 20 ostatnich pomiarów`);
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
    renderNetworkCard(data.network || {});
    renderRelays(data);
    renderTodaySchedule(data);
    renderFeederCard(data);
    renderTemperatureChart(data.temperature || {});
}

async function fetchStatus() {
    try {
        const response = await fetch(API_STATUS, { cache: 'no-store' });
        if (!response.ok) throw new Error('status http');
        const data = await response.json();
        setBackendState(true);
        syncClockFromController(data.clock);
        renderDashboard(data);
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

)AQSCRIPT";

#endif // WEB_ASSETS_H
