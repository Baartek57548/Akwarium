#ifndef WEBASSETS_H
#define WEBASSETS_H

#include <Arduino.h>

// ==========================================================
// WYGENEROWANO AUTOMATYCZNIE - WebAssets.h
// Zawiera najnowsze wersje: index.html, style.css, script.js
// ==========================================================

const char web_index_html[] PROGMEM = R"WEBASSET(
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
            border-radius: 14px;
            padding: 7px 12px;
            font-size: 14px;
            cursor: pointer;
            font-family: inherit;
            outline: none;
            transition: background 0.2s, border-color 0.2s;
            text-align: center;
            min-width: 112px;
        }
        input[type="time"].time-pill:hover {
            background: rgba(0, 0, 0, 0.9);
            border-color: rgba(255, 255, 255, 0.3);
        }
        input[type="time"].time-pill::-webkit-calendar-picker-indicator {
            filter: invert(0.6) sepia(1) saturate(5) hue-rotate(175deg);
            cursor: pointer;
            width: 14px;
            height: 14px;
            padding: 0;
            margin-left: 4px;
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

        /* Mobile schedule overflow fix */
        @media (max-width: 768px) {
            .timeline-container {
                max-width: 100% !important;
                padding: 16px !important;
                overflow: hidden;
            }
            .schedule-bar-container {
                margin-bottom: 40px !important;
                overflow: visible;
            }
            /* Clamp time pills so they never overflow outside the container */
            input[type="time"].time-pill {
                max-width: 90px !important;
                font-size: 12px !important;
                padding: 5px 8px !important;
                width: 90px !important;
            }
            .schedule-item {
                gap: 10px;
            }
            /* Fix: make select dropdowns smaller on mobile */
            .schedule-details select.form-control {
                font-size: 11px;
                padding: 3px 6px;
                max-width: 130px;
            }
            /* Ensure dashboard grid stretches full width on mobile */
            #dashboard .dashboard-grid {
                grid-template-columns: 1fr !important;
            }
            /* Relay cards stack nicely */
            #dashboard .dashboard-grid > div:nth-child(4) > div:last-child {
                grid-template-columns: 1fr 1fr;
            }
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

        <!-- Mobile Sidebar Overlay -->
        <div class="mobile-overlay" id="mobile-overlay" onclick="closeMobileNav()"></div>

        <!-- Main Content -->
        <main class="main-content">
            <!-- Topbar -->
            <header class="topbar">
                <button class="hamburger-btn" id="hamburger-btn" onclick="toggleMobileNav()" aria-label="Menu">
                    <i class="fa-solid fa-bars"></i>
                </button>
                <div class="topbar-left" style="display: flex; gap: 10px; align-items: center;">
                    <span style="font-size: 11px; font-weight: 600; color: var(--text-muted); letter-spacing: 1px; margin-right: 5px;">AKTYWNE SIECI:</span>
                    <div class="status-badge" style="background: rgba(39, 201, 63, 0.1); border-color: rgba(39, 201, 63, 0.3); color: #27c93f; padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot" style="width: 8px; height: 8px;"></span>
                        Access Point (AP)
                    </div>
                    <div class="status-badge" style="background: rgba(39, 201, 63, 0.1); border-color: rgba(39, 201, 63, 0.3); color: #27c93f; padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot" style="width: 8px; height: 8px;"></span>
                        Station (STA)
                    </div>
                    <div class="status-badge" style="background: rgba(6, 182, 212, 0.1); border-color: rgba(6, 182, 212, 0.3); color: var(--accent-cyan); padding: 6px 14px; font-size: 13px;">
                        <span class="pulse-dot-cyan"></span>
                        Bluetooth (BLE)
                    </div>
                </div><!-- end topbar-left -->
                
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

                    <div class="card glass" style="padding: 20px;">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;">
                            <span style="font-size: 14px; font-weight: 600; color: var(--text-main);">Przekaźniki</span>
                            <span style="font-size: 11px; color: var(--text-muted); background: rgba(255,255,255,0.05); padding: 4px 10px; border-radius: 20px;">1 / 4 aktywnych</span>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px;">

                            <!-- Swiatlo - OFF -->
                            <div style="background: rgba(250, 204, 21, 0.04); border: 1px solid rgba(250, 204, 21, 0.15); border-radius: 18px; padding: 16px; position: relative; overflow: hidden; cursor: pointer; transition: 0.3s;" onmouseover="this.style.background='rgba(250,204,21,0.08)'" onmouseout="this.style.background='rgba(250,204,21,0.04)'">
                                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 14px;">
                                    <div style="width: 36px; height: 36px; border-radius: 10px; background: rgba(250, 204, 21, 0.1); display: flex; align-items: center; justify-content: center;">
                                        <i class="fa-solid fa-lightbulb" style="color: rgba(250,204,21,0.5); font-size: 16px;"></i>
                                    </div>
                                    <span style="font-size: 10px; font-weight: 700; letter-spacing: 1px; color: var(--text-muted); padding: 3px 8px; border-radius: 8px; background: rgba(255,255,255,0.05);">OFF</span>
                                </div>
                                <div style="font-size: 13px; font-weight: 600; color: var(--text-main);">Światło</div>
                                <div style="font-size: 11px; color: var(--text-muted); margin-top: 3px;">Wył. harmonogram</div>
                                <div style="position: absolute; bottom: 0; left: 0; right: 0; height: 3px; background: rgba(250,204,21,0.2); border-radius: 0 0 18px 18px;"></div>
                            </div>

                            <!-- Filtr - ON -->
                            <div style="background: rgba(6, 182, 212, 0.07); border: 1px solid rgba(6, 182, 212, 0.3); border-radius: 18px; padding: 16px; position: relative; overflow: hidden; cursor: pointer; transition: 0.3s; box-shadow: 0 0 18px rgba(6,182,212,0.1);" onmouseover="this.style.background='rgba(6,182,212,0.12)'" onmouseout="this.style.background='rgba(6,182,212,0.07)'">
                                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 14px;">
                                    <div style="width: 36px; height: 36px; border-radius: 10px; background: rgba(6, 182, 212, 0.15); display: flex; align-items: center; justify-content: center;">
                                        <i class="fa-solid fa-filter" style="color: var(--accent-cyan); font-size: 16px;"></i>
                                    </div>
                                    <span style="font-size: 10px; font-weight: 700; letter-spacing: 1px; color: var(--accent-cyan); padding: 3px 8px; border-radius: 8px; background: rgba(6,182,212,0.15);">ON</span>
                                </div>
                                <div style="font-size: 13px; font-weight: 600; color: var(--text-main);">Filtr</div>
                                <div style="font-size: 11px; color: var(--accent-cyan); margin-top: 3px;">Aktywny · 10:30–20:30</div>
                                <div style="position: absolute; bottom: 0; left: 0; right: 0; height: 3px; background: linear-gradient(90deg, var(--accent-cyan), transparent); border-radius: 0 0 18px 18px;"></div>
                            </div>

                            <!-- Grzalka - Standby -->
                            <div style="background: rgba(251, 146, 60, 0.04); border: 1px solid rgba(251, 146, 60, 0.15); border-radius: 18px; padding: 16px; position: relative; overflow: hidden; cursor: pointer; transition: 0.3s;" onmouseover="this.style.background='rgba(251,146,60,0.08)'" onmouseout="this.style.background='rgba(251,146,60,0.04)'">
                                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 14px;">
                                    <div style="width: 36px; height: 36px; border-radius: 10px; background: rgba(251, 146, 60, 0.1); display: flex; align-items: center; justify-content: center;">
                                        <i class="fa-solid fa-fire" style="color: rgba(251,146,60,0.6); font-size: 16px;"></i>
                                    </div>
                                    <span style="font-size: 10px; font-weight: 700; letter-spacing: 1px; color: rgba(251,146,60,0.7); padding: 3px 8px; border-radius: 8px; background: rgba(251,146,60,0.1);">STANDBY</span>
                                </div>
                                <div style="font-size: 13px; font-weight: 600; color: var(--text-main);">Grzałka</div>
                                <div style="font-size: 11px; color: var(--text-muted); margin-top: 3px;">Temp. OK · czeka</div>
                                <div style="position: absolute; bottom: 0; left: 0; right: 0; height: 3px; background: rgba(251,146,60,0.2); border-radius: 0 0 18px 18px;"></div>
                            </div>

                            <!-- Napowietrzanie - ON -->
                            <div style="background: rgba(16, 185, 129, 0.07); border: 1px solid rgba(16, 185, 129, 0.3); border-radius: 18px; padding: 16px; position: relative; overflow: hidden; cursor: pointer; transition: 0.3s; box-shadow: 0 0 18px rgba(16,185,129,0.08);" onmouseover="this.style.background='rgba(16,185,129,0.12)'" onmouseout="this.style.background='rgba(16,185,129,0.07)'">
                                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 14px;">
                                    <div style="width: 36px; height: 36px; border-radius: 10px; background: rgba(16, 185, 129, 0.15); display: flex; align-items: center; justify-content: center;">
                                        <i class="fa-solid fa-wind" style="color: var(--success-color); font-size: 16px;"></i>
                                    </div>
                                    <span style="font-size: 10px; font-weight: 700; letter-spacing: 1px; color: var(--success-color); padding: 3px 8px; border-radius: 8px; background: rgba(16,185,129,0.15);">ON</span>
                                </div>
                                <div style="font-size: 13px; font-weight: 600; color: var(--text-main);">Napowietrzanie</div>
                                <div style="font-size: 11px; color: var(--success-color); margin-top: 3px;">Aktywny · 10:00–19:00</div>
                                <div style="position: absolute; bottom: 0; left: 0; right: 0; height: 3px; background: linear-gradient(90deg, var(--success-color), transparent); border-radius: 0 0 18px 18px;"></div>
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

                    <!-- Time axis 0-24h -->
                    <div class="time-axis">
                        <span>0</span>
                        <span>6</span>
                        <span>12</span>
                        <span>18</span>
                        <span>24</span>
                    </div>
                    
                    <div class="schedule-item">
                        <div class="schedule-icon"><i class="fa-regular fa-lightbulb" style="color: var(--accent-yellow);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Oświetlenie Główne</div>
                                <select class="form-control" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączone</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączone</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" data-start="10:00" data-end="21:30">
                                <div class="schedule-bar" style="background: var(--accent-yellow);"></div>
                                <input type="time" class="time-pill start-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="10:00">
                                <input type="time" class="time-pill end-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="21:30">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4">
                        <div class="schedule-icon"><i class="fa-solid fa-wind" style="color: var(--accent-white);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Napowietrzanie</div>
                                <select class="form-control" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączone</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączone</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" data-start="10:00" data-end="19:00">
                                <div class="schedule-bar" style="background: var(--accent-white);"></div>
                                <input type="time" class="time-pill start-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="10:00">
                                <input type="time" class="time-pill end-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="19:00">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4">
                        <div class="schedule-icon"><i class="fa-solid fa-filter" style="color: var(--accent-blue);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Filtracja</div>
                                <select class="form-control" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="harmonogram" selected>Harmonogram</option>
                                    <option value="zawsze_wlaczone">Zawsze włączone</option>
                                    <option value="zawsze_wylaczone">Zawsze wyłączone</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" data-start="10:30" data-end="20:30">
                                <div class="schedule-bar" style="background: var(--accent-blue);"></div>
                                <input type="time" class="time-pill start-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="10:30">
                                <input type="time" class="time-pill end-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="20:30">
                            </div>
                        </div>
                    </div>

                    <div class="schedule-item mt-4">
                        <div class="schedule-icon"><i class="fa-solid fa-fish" style="color: var(--accent-cyan);"></i></div>
                        <div class="schedule-details">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: wrap; gap: 6px;">
                                <div class="schedule-title" style="margin-bottom: 0;">Karmienie (Automatyczne)</div>
                                <select class="form-control" style="width: auto; padding: 4px 8px; font-size: 12px; height: auto;">
                                    <option value="wylaczone">Wyłączone</option>
                                    <option value="codziennie" selected>Codziennie</option>
                                    <option value="co_2_dni">Co 2 dni</option>
                                    <option value="co_3_dni">Co 3 dni</option>
                                </select>
                            </div>
                            <div class="schedule-bar-container" data-start="18:00" data-point="true">
                                <div class="schedule-point" style="background: var(--accent-cyan);"></div>
                                <input type="time" class="time-pill start-pill" style="position: absolute; top: 26px; transform: translateX(-50%);" value="18:00">
                            </div>
                        </div>
                    </div>

                    <!-- Save button -->
                    <div style="margin-top: 32px; display: flex; justify-content: flex-end; align-items: center; gap: 16px; padding-top: 20px; border-top: 1px solid var(--glass-border);">
                        <span style="font-size: 12px; color: var(--text-muted);">Ostatni zapis: dzisiaj 18:00</span>
                        <button class="btn" onclick="saveSchedules()" style="background: rgba(34, 211, 238, 0.08); border: 1px solid rgba(34, 211, 238, 0.25); color: var(--accent-cyan); padding: 10px 28px; border-radius: 14px; font-size: 13px; font-weight: 600; letter-spacing: 0.5px; cursor: pointer; transition: 0.25s; display: flex; align-items: center; gap: 8px;" onmouseover="this.style.background='rgba(34,211,238,0.15)'; this.style.borderColor='rgba(34,211,238,0.5)';" onmouseout="this.style.background='rgba(34,211,238,0.08)'; this.style.borderColor='rgba(34,211,238,0.25)';">
                            <i class="fa-solid fa-floppy-disk"></i> Zapisz Harmonogramy
                        </button>
                    </div>
                </div><!-- end timeline-container -->
            </section>

            <!-- Logi View -->
            <section id="logi" class="view-section">
                <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 30px;">
                    <div class="view-header" style="margin-bottom: 0;">
                        <h2>Logi systemowe</h2>
                        <p>Biezace logi aplikacji, komendy i wpisy krytyczne.</p>
                    </div>
                    <div style="display: flex; gap: 15px;">
                        <button class="btn btn-secondary" style="border-radius: 12px; padding: 10px 24px; font-weight: 500; font-size: 13px; background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.1);">Wyczysc widok</button>
                        <button class="btn btn-primary" style="background: rgba(239, 68, 68, 0.15); color: #ef4444; border: 1px solid rgba(239, 68, 68, 0.3); border-radius: 12px; padding: 10px 24px; font-weight: 500; font-size: 13px; transition: 0.2s;" onmouseover="this.style.background='rgba(239, 68, 68, 0.25)'" onmouseout="this.style.background='rgba(239, 68, 68, 0.15)'">Usun krytyczne</button>
                    </div>
                </div>
                
                <div style="display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; margin-bottom: 30px;">
                    <div class="card glass" style="padding: 20px; border-radius: 16px;">
                        <div style="font-size: 11px; font-weight: 600; color: var(--text-muted); margin-bottom: 10px; letter-spacing: 1px;">INFO</div>
                        <div style="font-size: 28px; font-weight: 600; color: var(--accent-cyan);">2</div>
                    </div>
                    <div class="card glass" style="padding: 20px; border-radius: 16px;">
                        <div style="font-size: 11px; font-weight: 600; color: var(--text-muted); margin-bottom: 10px; letter-spacing: 1px;">KRYTYCZNYCH</div>
                        <div style="font-size: 28px; font-weight: 600; color: #ef4444;">0</div>
                    </div>
                </div>
            
                <div class="card glass" style="padding: 20px; border-radius: 16px; min-height: 500px;">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px;">
                        <div style="display: flex; gap: 10px; align-items: center;">
                            <button class="btn btn-secondary active" style="padding: 8px 16px; border-radius: 12px; border: 1px solid rgba(6, 182, 212, 0.4); color: var(--accent-cyan); font-size: 13px; font-weight: 600; background: rgba(6, 182, 212, 0.1);">Biezace</button>
                            <button class="btn btn-secondary" style="padding: 8px 16px; border-radius: 12px; border: 1px solid transparent; color: var(--text-main); font-size: 13px; font-weight: 600; transition: 0.2s;" onmouseover="this.style.background='rgba(239, 68, 68, 0.15)'; this.style.color='#ef4444';" onmouseout="this.style.background='transparent'; this.style.color='var(--text-main)';">Krytyczne</button>
                            <span style="font-size: 13px; color: var(--text-muted); margin-left: 10px;">Brak odpowiedzi sterownika.</span>
                        </div>
                        <div style="display: flex; gap: 10px; align-items: center;">
                            <input type="text" id="log-search" placeholder="Szukaj..." style="background: rgba(0,0,0,0.3); border: 1px solid var(--glass-border); border-radius: 8px; padding: 10px 16px; color: var(--text-main); font-size: 13px; width: 250px; outline: none;">
                            <button class="btn btn-secondary" onclick="exportLogsCSV()" style="padding: 10px 14px; border-radius: 8px; border: 1px solid var(--glass-border); background: rgba(255,255,255,0.03);" title="Pobierz logi jako CSV"><i class="fa-solid fa-download" style="color: var(--accent-cyan);"></i></button>
                        </div>
                    </div>
            
                    <div id="log-entries" style="display: flex; flex-direction: column; gap: 10px;">
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

    <!-- Mobile Bottom Navigation -->
    <nav class="mobile-bottom-nav" id="mobile-bottom-nav">
        <a href="#" class="mob-nav-item active" data-target="dashboard">
            <i class="fa-solid fa-gauge-high"></i>
            <span>Dashboard</span>
        </a>
        <a href="#" class="mob-nav-item" data-target="harmonogramy">
            <i class="fa-solid fa-calendar-days"></i>
            <span>Harmonogramy</span>
        </a>
        <a href="#" class="mob-nav-item" data-target="logi">
            <i class="fa-solid fa-terminal"></i>
            <span>Logi</span>
        </a>
        <a href="#" class="mob-nav-item" data-target="ustawienia">
            <i class="fa-solid fa-gear"></i>
            <span>Ustawienia</span>
        </a>
        <a href="#" class="mob-nav-item" data-target="ota">
            <i class="fa-solid fa-cloud-arrow-up"></i>
            <span>OTA</span>
        </a>
    </nav>

    <script src="script.js"></script>
    <script>
        function toggleMobileNav() {
            const sidebar = document.querySelector('.sidebar');
            const overlay = document.getElementById('mobile-overlay');
            sidebar.classList.toggle('mobile-open');
            overlay.classList.toggle('active');
        }
        function closeMobileNav() {
            const sidebar = document.querySelector('.sidebar');
            const overlay = document.getElementById('mobile-overlay');
            sidebar.classList.remove('mobile-open');
            overlay.classList.remove('active');
        }
        // Sync mobile bottom nav with sidebar nav
        document.querySelectorAll('.mob-nav-item').forEach(item => {
            item.addEventListener('click', function(e) {
                e.preventDefault();
                const target = this.dataset.target;
                // Trigger the main nav click logic
                const mainNavItem = document.querySelector(`.nav-item[data-target="${target}"]`);
                if (mainNavItem) mainNavItem.click();
                // Update mobile nav active state
                document.querySelectorAll('.mob-nav-item').forEach(i => i.classList.remove('active'));
                this.classList.add('active');
            });
        });
        // Keep bottom nav in sync when sidebar links are clicked
        document.querySelectorAll('.nav-item[data-target]').forEach(item => {
            item.addEventListener('click', function() {
                const target = this.dataset.target;
                document.querySelectorAll('.mob-nav-item').forEach(i => {
                    i.classList.toggle('active', i.dataset.target === target);
                });
                closeMobileNav();
            });
        });
    </script>
</body>
</html>

)WEBASSET";

const char web_style_css[] PROGMEM = R"WEBASSET(
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
    max-width: 100%;
}
.p-4 { padding: 36px; }
.mh-2 { font-size: 22px; font-weight: 700; letter-spacing: 0.3px; }

.schedule-item {
    display: flex;
    gap: 24px;
    align-items: center;
}
.schedule-icon {
    width: 56px;
    height: 56px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: rgba(255,255,255,0.06);
    border-radius: 16px;
    border: 1px solid var(--glass-border);
    font-size: 24px;
    flex-shrink: 0;
}
.schedule-details {
    flex: 1;
}
.schedule-title {
    font-size: 16px;
    font-weight: 600;
    margin-bottom: 12px;
    color: var(--text-main);
}

/* Time axis 0-24h */
.time-axis {
    display: flex;
    justify-content: space-between;
    padding: 0 0 8px 0;
    margin-bottom: 16px;
    border-bottom: 1px solid rgba(255,255,255,0.06);
}
.time-axis span {
    font-size: 11px;
    color: var(--text-muted);
    opacity: 0.6;
    font-weight: 500;
    font-family: 'Consolas', 'Courier New', monospace;
    min-width: 20px;
    text-align: center;
}

/* Schedule bar container — margin-bottom set by JS */
.schedule-bar-container {
    height: 20px;
    background: rgba(0,0,0,0.3);
    border-radius: 10px;
    position: relative;
    /* Subtle grid lines every 25% (6h intervals) */
    background-image:
        linear-gradient(90deg,
            rgba(255,255,255,0.08) 1px, transparent 1px);
    background-size: 25% 100%;
    background-position: 0 0;
}
.schedule-bar {
    position: absolute;
    height: 100%;
    border-radius: 10px;
    box-shadow: 0 0 18px rgba(255,255,255,0.3);
}
.schedule-point {
    position: absolute;
    width: 20px;
    height: 20px;
    border-radius: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    box-shadow: 0 0 14px rgba(6, 182, 212, 0.6);
}
.schedule-times {
    display: flex;
    justify-content: space-between;
    font-size: 13px;
    color: var(--text-muted);
}
.schedule-times.point-time {
    position: relative;
    height: 14px;
}
.schedule-times.point-time span {
    position: absolute;
    transform: translateX(-50%);
}

.mt-4 { margin-top: 36px; }

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

/* =============================================
   HAMBURGER BUTTON (hidden on desktop)
   ============================================= */
.hamburger-btn {
    display: none;
    background: var(--glass-bg);
    border: 1px solid var(--glass-border);
    color: var(--text-main);
    width: 40px;
    height: 40px;
    border-radius: 10px;
    font-size: 16px;
    cursor: pointer;
    align-items: center;
    justify-content: center;
    transition: background 0.2s;
    flex-shrink: 0;
}
.hamburger-btn:hover { background: rgba(255,255,255,0.1); }

/* Mobile dim overlay */
.mobile-overlay {
    display: none;
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,0.6);
    backdrop-filter: blur(4px);
    z-index: 99;
}
.mobile-overlay.active { display: block; }

/* =============================================
   MOBILE BOTTOM NAVIGATION
   ============================================= */
.mobile-bottom-nav {
    display: none;
}

/* =============================================
   RESPONSIVE BREAKPOINTS
   ============================================= */

/* Tablet: ≤1024px */
@media (max-width: 1024px) {
    :root { --sidebar-width: 240px; }
    .main-content { padding: 24px 28px; }
    .dashboard-grid { grid-template-columns: 1fr 1fr; }
    .card.span-2 { grid-column: span 2; }
}

/* Mobile: ≤768px */
@media (max-width: 768px) {
    /* Sidebar becomes a slide-in drawer */
    .sidebar {
        transform: translateX(-100%);
        transition: transform 0.3s ease;
        z-index: 200;
        width: 280px;
    }
    .sidebar.mobile-open {
        transform: translateX(0);
    }

    /* Hamburger visible */
    .hamburger-btn {
        display: flex;
    }

    /* Main content full width */
    .main-content {
        margin-left: 0;
        padding: 16px;
        padding-bottom: 90px; /* Space for bottom nav */
    }

    /* Topbar compact */
    .topbar {
        gap: 10px;
        margin-bottom: 20px;
        flex-wrap: nowrap;
    }
    .topbar-left {
        display: none !important; /* Hide network badges on mobile */
    }
    .topbar-widgets {
        margin-left: auto;
        gap: 8px;
    }
    .info-pill {
        padding: 6px 10px;
        font-size: 12px;
    }
    .time-widget #current-time { font-size: 16px; }
    .time-widget #current-date { font-size: 11px; }

    /* Single column grid */
    .dashboard-grid {
        grid-template-columns: 1fr;
        gap: 16px;
    }
    .card.span-2 { grid-column: span 1; }

    /* Smaller cards */
    .card { border-radius: 16px; }
    .card-header { padding: 16px 16px 0; }
    .card-body { padding: 16px; }

    /* Stats row single line */
    .stats-row {
        flex-direction: column;
        gap: 8px;
    }

    /* Temperature chart shorter */
    .temp-chart { height: 80px; }

    /* Grid 2 col → 1 col on mobile */
    .grid-2-col {
        grid-template-columns: 1fr;
        gap: 10px;
    }

    /* View header compact */
    .view-header h2 { font-size: 20px; }

    /* Bottom nav visible */
    .mobile-bottom-nav {
        display: flex;
        position: fixed;
        bottom: 0;
        left: 0;
        right: 0;
        height: 72px;
        background: rgba(8, 12, 28, 0.92);
        backdrop-filter: blur(20px);
        -webkit-backdrop-filter: blur(20px);
        border-top: 1px solid var(--glass-border);
        z-index: 150;
        align-items: stretch;
    }
    .mob-nav-item {
        flex: 1;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        gap: 4px;
        color: var(--text-muted);
        text-decoration: none;
        font-size: 10px;
        font-weight: 500;
        padding: 8px 4px;
        transition: color 0.2s, background 0.2s;
        border-radius: 0;
    }
    .mob-nav-item i {
        font-size: 18px;
        transition: transform 0.2s;
    }
    .mob-nav-item:hover,
    .mob-nav-item.active {
        color: var(--accent-cyan);
        background: rgba(34, 211, 238, 0.05);
    }
    .mob-nav-item.active i {
        transform: translateY(-2px);
        filter: drop-shadow(0 0 6px var(--accent-cyan));
    }

    /* Relay grid 1 col */
    .relay-grid {
        grid-template-columns: 1fr 1fr;
    }

    /* Timeline container full-width on mobile */
    .timeline-container {
        border-radius: 12px;
        margin-left: -16px;
        margin-right: -16px;
        width: calc(100% + 32px);
    }
    .timeline-container.p-4 {
        padding: 20px 16px;
    }

    /* Schedule items stack vertically on mobile */
    .schedule-item {
        flex-direction: column;
        align-items: flex-start;
        gap: 10px;
    }
    .schedule-icon { align-self: flex-start; }
    .schedule-details {
        width: 100%;
    }

    /* Switch to static time pills below bar */
    .schedule-bar-container { overflow: visible; }

    /* Terminal responsive */
    .terminal {
        max-width: 100%;
        border-radius: 16px;
    }
    .terminal-header {
        padding: 10px 12px;
    }
    .terminal-dots span {
        width: 10px;
        height: 10px;
    }
    .terminal-title {
        font-size: 12px;
    }
    .terminal-body {
        height: 300px;
        padding: 12px;
        font-size: 12px;
        line-height: 1.5;
    }
    .log-line {
        display: flex;
        flex-wrap: wrap;
        gap: 2px 6px;
        padding: 6px 0;
        border-bottom: 1px solid rgba(255,255,255,0.04);
        margin-bottom: 0;
    }
    .log-line:last-child {
        border-bottom: none;
    }
    .log-time {
        font-size: 11px;
    }
    .log-info, .log-warn, .log-success, .log-error {
        font-size: 11px;
        font-weight: 600;
    }
    .log-msg {
        flex-basis: 100%;
        font-size: 12px;
        padding-left: 4px;
        word-break: break-word;
    }

    /* OTA form compact */
    .file-upload-wrapper { margin-top: 12px; }
}

/* Small phones: ≤480px */
@media (max-width: 480px) {
    .main-content { padding: 12px; padding-bottom: 85px; }
    .card-body { padding: 12px; }

    /* Status badges in sidebar - icon only spacing */
    .status-badge { padding: 5px 10px; font-size: 12px; }

    /* Feeder circle smaller */
    .feed-btn {
        width: 100px;
        height: 100px;
        font-size: 13px;
    }
    .relay-grid { grid-template-columns: 1fr 1fr; gap: 8px; }

    /* Terminal even more compact on small phones */
    .terminal {
        border-radius: 12px;
    }
    .terminal-header {
        padding: 8px 10px;
    }
    .terminal-body {
        height: 260px;
        padding: 10px;
        font-size: 11px;
    }
    .log-time {
        font-size: 10px;
    }
    .log-info, .log-warn, .log-success, .log-error {
        font-size: 10px;
    }
    .log-msg {
        font-size: 11px;
    }
}

)WEBASSET";

const char web_script_js[] PROGMEM = R"WEBASSET(
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

// Export logs to CSV
function exportLogsCSV() {
    const logContainer = document.getElementById('log-entries');
    if (!logContainer) return;

    const entries = logContainer.querySelectorAll('div[style*="border-radius: 8px"]');
    if (!entries.length) {
        alert('Brak logów do eksportu.');
        return;
    }

    // CSV header with BOM for Excel UTF-8 support
    const BOM = '\uFEFF';
    const rows = ['Typ;Godzina;Wiadomość'];

    entries.forEach(entry => {
        const spans = entry.querySelectorAll('span');
        if (spans.length < 3) return;

        const tagText = spans[0].textContent.trim();
        const time = spans[1].textContent.trim();
        const message = spans[2].textContent.trim();

        // Classify: if tag contains "CRIT" or color is red → Krytyczny, else Info
        const tagColor = spans[0].style.color || '';
        const isCritical = tagText.toUpperCase().includes('CRIT') ||
                           tagText.toUpperCase().includes('ERROR') ||
                           tagText.toUpperCase().includes('WARN') ||
                           tagColor.includes('ef4444') ||
                           tagColor.includes('f59e0b');

        const typ = isCritical ? 'Krytyczny' : 'Informacyjny';

        // Escape quotes in message for CSV
        const safeMessage = '"' + message.replace(/"/g, '""') + '"';

        rows.push(`${typ};${time};${safeMessage}`);
    });

    const csvContent = BOM + rows.join('\n');

    // Generate filename with current date
    const now = new Date();
    const dateStr = now.toISOString().slice(0, 10);
    const filename = `logi_akwarium_${dateStr}.csv`;

    // Use data URI — more reliable for filename preservation than blob URLs
    const encoded = encodeURIComponent(csvContent);
    const link = document.createElement('a');
    link.href = 'data:text/csv;charset=utf-8,' + encoded;
    link.download = filename;
    link.style.display = 'none';
    document.body.appendChild(link);
    link.click();
    // Clean up after a short delay
    setTimeout(() => { document.body.removeChild(link); }, 200);
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

)WEBASSET";

#endif // WEBASSETS_H
