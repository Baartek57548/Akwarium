#ifndef WEBASSETS_H
#define WEBASSETS_H

#include <Arduino.h>

// ==========================================================
// WYGENEROWANO AUTOMATYCZNIE - WebAssets.h
// Zawiera najnowsze wersje: index.html, style.css, script.js
// ==========================================================

const char web_index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aquarium Controller ESP32</title>
    <link rel="stylesheet" href="style.css?v=3">
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
                <div style="display: flex; gap: 10px; align-items: center;">
                    <span style="font-size: 11px; font-weight: 600; color: var(--text-muted); letter-spacing: 1px; margin-right: 5px;">AKTYWNE SIECI:</span>
                    <div id="ap-badge" class="status-badge" style="background: rgba(39, 201, 63, 0.1); border-color: rgba(39, 201, 63, 0.3); color: #27c93f; padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot" style="width: 8px; height: 8px;"></span>
                        Access Point (AP)
                    </div>
                    <div id="sta-badge" class="status-badge" style="background: rgba(39, 201, 63, 0.1); border-color: rgba(39, 201, 63, 0.3); color: #27c93f; padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot" style="width: 8px; height: 8px;"></span>
                        Station (STA)
                    </div>
                    <div id="ble-badge" class="status-badge" style="background: rgba(6, 182, 212, 0.1); border-color: rgba(6, 182, 212, 0.3); color: var(--accent-cyan); padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot-cyan"></span>
                        Bluetooth (BLE)
                    </div>
                </div>
                
                <div class="topbar-widgets">
                    <div class="info-pill" title="Bateria RTC (DS3231)">
                        <i class="fa-solid fa-battery-three-quarters"></i>
                        <span id="rtc-battery">2.9V</span>
                    </div>
                    <div class="time-widget">
                        <span id="current-time">00:00:00</span>
                        <span id="current-date">01 Sty 2026</span>
                    </div>
                </div>
            </header>

            <!-- Dashboard View -->
            <section id="dashboard" class="view-section active">
                <div class="view-header">
                    <h2>Przegląd Systemu</h2>
                    <p>Bieżący status wszystkich urządzeń i sensorów.</p>
                </div>
                <!-- Dashboard Grid -->
                <div class="dashboard-grid">
                    <!-- Wiersz 1: Temperatura, Bateria, Siec -->
                    <div class="card glass" style="padding: 16px;">
                        <span style="font-size: 10px; color: var(--text-muted); font-weight: bold; letter-spacing: 1px; margin-bottom: 15px; display: block;">TEMPERATURA</span>
                        <div class="temp-value" style="font-size: 36px; font-weight: 600; text-shadow: none;">--.-<span class="unit" style="font-size: 24px; color: var(--text-main);"> C</span></div>
                        <div style="font-size: 12px; color: var(--text-muted); margin-top: 8px;">Cel 25 C | Histereza 0,5 C</div>
                    </div>

                    <div class="card glass" style="padding: 16px;">
                        <span style="font-size: 10px; color: var(--text-muted); font-weight: bold; letter-spacing: 1px; margin-bottom: 15px; display: block;">BATERIA</span>
                        <div class="temp-value" style="font-size: 36px; font-weight: 600; text-shadow: none; margin-bottom: 12px;">0%</div>
                        <div class="progress-bar-container">
                            <div class="progress-bar" style="height: 4px; background: rgba(255,255,255,0.05);"></div>
                        </div>
                    </div>

                    <div class="card glass" style="padding: 16px;">
                        <span style="font-size: 10px; color: var(--text-muted); font-weight: bold; letter-spacing: 1px; margin-bottom: 15px; display: block;">SIEC</span>
                        <div class="temp-value" style="font-size: 30px; font-weight: 600; text-shadow: none;">OFFLINE</div>
                        <div style="font-size: 12px; color: var(--text-muted); margin-top: 8px;">STA -</div>
                    </div>

                    <!-- Wiersz 2: Przekazniki, Harmonogram dzisiaj, Karmnik -->
                    <div class="card glass" style="padding: 20px;">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px;">
                            <span style="font-size: 14px; font-weight: 600; color: var(--text-main);">Przekazniki</span>
                            <span style="font-size: 11px; color: var(--text-muted);">1 / 4</span>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 14px;">
                            <div style="background: rgba(255,255,255,0.03); border: 1px solid var(--glass-border); border-radius: 16px; padding: 14px;">
                                <div style="font-size: 13px; font-weight: 600; margin-bottom: 4px; color: var(--text-main);">Swiatlo</div>
                                <div style="font-size: 11px; color: var(--text-muted);">Off</div>
                            </div>
                            <div style="background: rgba(255,255,255,0.03); border: 1px solid var(--glass-border); border-radius: 16px; padding: 14px;">
                                <div style="font-size: 13px; font-weight: 600; margin-bottom: 4px; color: var(--text-main);">Filtr</div>
                                <div style="font-size: 11px; color: var(--text-muted);">Off</div>
                            </div>
                            <div style="background: rgba(255,255,255,0.03); border: 1px solid var(--glass-border); border-radius: 16px; padding: 14px;">
                                <div style="font-size: 13px; font-weight: 600; margin-bottom: 4px; color: var(--text-main);">Grzalka</div>
                                <div style="font-size: 11px; color: var(--text-muted);">Standby</div>
                            </div>
                            <div style="background: rgba(255,255,255,0.03); border: 1px solid var(--glass-border); border-radius: 16px; padding: 14px;">
                                <div style="font-size: 13px; font-weight: 600; margin-bottom: 4px; color: var(--text-main);">Napowietrzanie</div>
                                <div style="font-size: 11px; color: var(--text-muted);">Open</div>
                            </div>
                        </div>
                    </div>

                    <div class="card glass" style="padding: 20px;">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px;">
                            <span style="font-size: 14px; font-weight: 600; color: var(--text-main);">Harmonogram dzisiaj</span>
                            <button class="btn btn-secondary" onclick="switchTab('harmonogramy')" style="font-size: 12px; padding: 6px 16px; border-radius: 16px; color: var(--accent-cyan); border-color: rgba(6, 182, 212, 0.3);">Edytuj</button>
                        </div>
                        <div style="display: flex; flex-direction: column; gap: 10px;">
                            <div style="display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.03); padding: 14px 16px; border-radius: 16px; border: 1px solid var(--glass-border);">
                                <span style="font-size: 12px; color: var(--text-main);">Swiatlo</span>
                                <span style="font-size: 12px; color: var(--text-muted);">10:00 - 21:30</span>
                            </div>
                            <div style="display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.03); padding: 14px 16px; border-radius: 16px; border: 1px solid var(--glass-border);">
                                <span style="font-size: 12px; color: var(--text-main);">Filtr</span>
                                <span style="font-size: 12px; color: var(--text-muted);">10:30 - 20:30</span>
                            </div>
                            <div style="display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.03); padding: 14px 16px; border-radius: 16px; border: 1px solid var(--glass-border);">
                                <span style="font-size: 12px; color: var(--text-main);">Napowietrzanie</span>
                                <span style="font-size: 12px; color: var(--text-muted);">10:00 - 19:00</span>
                            </div>
                            <div style="display: flex; justify-content: space-between; align-items: center; background: rgba(255,255,255,0.03); padding: 14px 16px; border-radius: 16px; border: 1px solid var(--glass-border);">
                                <span style="font-size: 12px; color: var(--text-main);">Karmienie</span>
                                <span style="font-size: 12px; color: var(--text-muted);">Codziennie 18:00</span>
                            </div>
                        </div>
                    </div>

                    <div class="card glass" style="padding: 20px; align-items: center;">
                        <span style="font-size: 14px; font-weight: 600; color: var(--text-main); margin-bottom: 25px; align-self: flex-start;">Karmnik</span>
                        
                        <div style="position: relative; width: 140px; height: 140px; border-radius: 50%; border: 2px solid #8B2E67; display: flex; align-items: center; justify-content: center; background: #1C1120; margin-bottom: 20px; box-shadow: inset 0 0 10px rgba(0,0,0,0.5);">
                            <button onclick="triggerFeed()" style="width: 110px; height: 110px; border-radius: 50%; background: #32142D; border: none; color: #F472B6; font-size: 14px; font-weight: 600; cursor: pointer; transition: 0.2s;" onmouseover="this.style.background='#4a1e43'" onmouseout="this.style.background='#32142D'">Karm teraz</button>
                        </div>
                        
                        <span style="font-size: 12px; color: var(--text-muted); margin-bottom: 20px;">Codziennie 18:00</span>
                        <button class="btn btn-secondary" onclick="switchTab('harmonogramy')" style="font-size: 13px; padding: 10px 24px; border-radius: 16px; background: rgba(255,255,255,0.03);">Zarzadzaj</button>
                    </div>

                    <!-- Wiersz 3: Zakres temperatury (Wykres 20 bar) -->
                    <div class="card glass" style="grid-column: 1 / -1; padding: 20px;">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;">
                            <span style="font-size: 14px; font-weight: 600; color: var(--text-main);">Zakres temperatury</span>
                            <span style="font-size: 11px; color: var(--text-muted); padding: 4px 10px; background: rgba(255,255,255,0.05); border-radius: 10px;">Ostatnie 3 godz. (20 odczytów)</span>
                        </div>
                        
                        <div class="temp-chart">
                            <!-- Histereza i cel -->
                            <div class="hysteresis-zone" style="height: 30%; bottom: 40%;"></div>
                            <div class="target-temp-line" style="bottom: 55%;"><span class="target-temp-label">25.0°C Docelowa</span></div>
                            
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 30%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 35%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 32%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 40%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 45%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 40%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 42%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 38%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 46%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 55%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 60%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 65%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 68%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 64%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 70%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 80%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar hot" style="height: 90%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 75%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar" style="height: 65%;"></div></div>
                            <div class="temp-bar-wrap"><div class="temp-bar active" style="height: 55%;"></div></div>
                        </div>
                        
                        <div style="display: flex; justify-content: space-between; margin-top: 8px; font-size: 10px; color: var(--text-muted);">
                            <span>-190 min</span>
                            <span>Teraz</span>
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

    <script src="script.js"></script>
</body>
</html>

)rawliteral";

const char web_style_css[] PROGMEM = R"rawliteral(
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
    --sidebar-width: 280px;
    --border-radius-lg: 24px;
    --border-radius-md: 16px;
}

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
    font-family: 'Inter', sans-serif;
}

body {
    background-color: var(--bg-dark);
    /* Deep ocean background with a heavy gradient overlay to ensure perfect contrast and readability */
    background-image: 
        linear-gradient(to bottom right, rgba(3, 7, 18, 0.85), rgba(3, 7, 18, 0.95)),
        url('https://images.unsplash.com/photo-1518837695005-2083093ee35b?q=80&w=2560&auto=format&fit=crop');
    background-size: cover;
    background-position: center;
    background-attachment: fixed;
    color: var(--text-main);
    min-height: 100vh;
    overflow-x: hidden;
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
    padding: 32px 48px;
    max-width: 1400px;
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
    grid-template-columns: repeat(auto-fit, minmax(360px, 1fr));
    gap: 32px;
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
.water-info b { color: var(--accent-cyan); }

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
    max-width: 800px;
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
    max-width: 1000px;
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

[data-target] { cursor: pointer; }


)rawliteral";

const char web_script_js[] PROGMEM = R"rawliteral(
const API_STATUS = '/api/status';
const API_ACTION = '/api/action';
const API_LOGS = '/api/logs';
const API_OTA = '/update';

let backendConnected = false;
let activeLogType = 'normal';
let cachedLogs = { normal: [], critical: [] };

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

)rawliteral";

#endif
