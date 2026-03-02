import { useState } from "react";
import {
  Thermometer,
  Wifi,
  WifiOff,
  Radio,
  Sliders,
  Monitor,
  Upload,
  RefreshCw,
  Save,
  Check,
  ChevronRight,
  Moon,
  Sun,
  AlertCircle,
  Info,
  Cpu,
  Wind,
} from "lucide-react";
import { useDevice } from "../deviceContext";

function SectionHeader({ icon: Icon, title, color }: { icon: React.ElementType; title: string; color: string }) {
  return (
    <div className="flex items-center gap-2 mb-3">
      <div className="rounded-xl p-1.5" style={{ background: `${color}18` }}>
        <Icon size={15} style={{ color }} />
      </div>
      <span style={{ fontSize: 13, color: "#94a3b8" }}>{title}</span>
    </div>
  );
}

function RowItem({
  label,
  sublabel,
  value,
  onClick,
}: {
  label: string;
  sublabel?: string;
  value?: React.ReactNode;
  onClick?: () => void;
}) {
  return (
    <div
      className="flex items-center px-4 py-3 transition-colors"
      style={{ borderTop: "1px solid rgba(255,255,255,0.05)", cursor: onClick ? "pointer" : "default" }}
      onClick={onClick}
    >
      <div className="flex-1">
        <p style={{ fontSize: 13, color: "#e2e8f0" }}>{label}</p>
        {sublabel && <p style={{ fontSize: 11, color: "#64748b" }}>{sublabel}</p>}
      </div>
      <div className="flex items-center gap-1.5">
        {value && <span style={{ fontSize: 13, color: "#94a3b8" }}>{value}</span>}
        {onClick && <ChevronRight size={14} style={{ color: "#475569" }} />}
      </div>
    </div>
  );
}

export function Settings() {
  const {
    state,
    setTemp,
    toggleAP,
    setAlwaysScreenOn,
    setServoPreOffMins,
    setManualServo,
    addLog,
  } = useDevice();

  const [targetTemp, setTargetTempLocal] = useState(state.targetTemp.toFixed(1));
  const [hysteresis, setHysteresisLocal] = useState(state.hysteresis.toFixed(1));
  const [tempSaved, setTempSaved] = useState(false);

  const [servoAngle, setServoAngleLocal] = useState(45);
  const [servoPreOff, setServoPreOffLocal] = useState(state.servoPreOffMins);
  const [servoSaved, setServoSaved] = useState(false);

  const [syncingTime, setSyncingTime] = useState(false);
  const [timeSynced, setTimeSynced] = useState(false);

  const handleTempSave = () => {
    const t = parseFloat(targetTemp);
    const h = parseFloat(hysteresis);
    if (!isNaN(t) && !isNaN(h)) {
      setTemp(t, h);
      setTempSaved(true);
      setTimeout(() => setTempSaved(false), 2000);
    }
  };

  const handleServoSave = () => {
    setServoPreOffMins(servoPreOff);
    setServoSaved(true);
    setTimeout(() => setServoSaved(false), 2000);
  };

  const handleTimeSync = () => {
    setSyncingTime(true);
    setTimeout(() => {
      setSyncingTime(false);
      setTimeSynced(true);
      addLog("INFO", "Czas zsynchronizowany z urządzeniem klienta");
      setTimeout(() => setTimeSynced(false), 3000);
    }, 1500);
  };

  const rssiColor = state.wifi.rssi > -60 ? "#4ade80" : state.wifi.rssi > -75 ? "#fbbf24" : "#f87171";

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
      {/* Header */}
      <div
        className="px-5 pt-12 pb-5"
        style={{
          background: "linear-gradient(160deg, #0c1527 0%, #080e1c 100%)",
          borderBottom: "1px solid rgba(6,182,212,0.1)",
        }}
      >
        <p style={{ fontSize: 11, color: "#64748b", letterSpacing: "0.12em" }} className="uppercase mb-1">
          Sterownik Akwarium PRO
        </p>
        <h1 style={{ fontSize: 22, color: "#e2e8f0" }}>Ustawienia</h1>
        <p style={{ fontSize: 12, color: "#64748b", marginTop: 2 }}>
          Konfiguracja urządzenia i sieci
        </p>
      </div>

      <div className="flex flex-col gap-4 px-4 pt-4 pb-4">
        {/* Temperature */}
        <div
          className="rounded-2xl p-4"
          style={{
            background: "linear-gradient(135deg, rgba(251,113,133,0.08) 0%, rgba(8,14,28,0.9) 100%)",
            border: "1px solid rgba(251,113,133,0.2)",
          }}
        >
          <SectionHeader icon={Thermometer} title="Temperatura" color="#fb7185" />

          <div className="grid grid-cols-2 gap-3 mb-3">
            <div className="flex flex-col gap-2">
              <label style={{ fontSize: 11, color: "#94a3b8" }}>Temperatura docelowa (°C)</label>
              <input
                type="number"
                step="0.1"
                min="18"
                max="30"
                value={targetTemp}
                onChange={(e) => setTargetTempLocal(e.target.value)}
                className="rounded-xl px-3 py-2.5 outline-none"
                style={{
                  background: "rgba(255,255,255,0.06)",
                  border: "1px solid rgba(255,255,255,0.1)",
                  color: "#fb7185",
                  fontSize: 16,
                  colorScheme: "dark",
                }}
              />
            </div>
            <div className="flex flex-col gap-2">
              <label style={{ fontSize: 11, color: "#94a3b8" }}>Histereza (°C)</label>
              <input
                type="number"
                step="0.1"
                min="0.1"
                max="2"
                value={hysteresis}
                onChange={(e) => setHysteresisLocal(e.target.value)}
                className="rounded-xl px-3 py-2.5 outline-none"
                style={{
                  background: "rgba(255,255,255,0.06)",
                  border: "1px solid rgba(255,255,255,0.1)",
                  color: "#fb7185",
                  fontSize: 16,
                  colorScheme: "dark",
                }}
              />
            </div>
          </div>

          <div
            className="rounded-xl px-3 py-2 mb-3"
            style={{
              background: "rgba(239,68,68,0.08)",
              border: "1px solid rgba(239,68,68,0.15)",
            }}
          >
            <p style={{ fontSize: 11, color: "#94a3b8" }}>
              <span style={{ color: "#ef4444" }}>Limit awaryjny: 28.0°C</span> — grzałka wycinana natychmiast
            </p>
          </div>

          <button
            onClick={handleTempSave}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: tempSaved ? "rgba(74,222,128,0.15)" : "rgba(251,113,133,0.12)",
              border: `1px solid ${tempSaved ? "rgba(74,222,128,0.3)" : "rgba(251,113,133,0.25)"}`,
              color: tempSaved ? "#4ade80" : "#fb7185",
            }}
          >
            {tempSaved ? <Check size={15} /> : <Save size={15} />}
            <span style={{ fontSize: 13 }}>{tempSaved ? "Zapisano!" : "Zapisz temperaturę"}</span>
          </button>
        </div>

        {/* Servo / Aeration */}
        <div
          className="rounded-2xl p-4"
          style={{
            background: "linear-gradient(135deg, rgba(52,211,153,0.08) 0%, rgba(8,14,28,0.9) 100%)",
            border: "1px solid rgba(52,211,153,0.2)",
          }}
        >
          <SectionHeader icon={Wind} title="Serwo napowietrzania" color="#34d399" />

          <div className="flex flex-col gap-3 mb-3">
            <div className="flex flex-col gap-2">
              <label style={{ fontSize: 11, color: "#94a3b8" }}>
                Ręczny kąt serwa: <span style={{ color: "#34d399" }}>{servoAngle}°</span>
              </label>
              <input
                type="range"
                min="0"
                max="90"
                value={servoAngle}
                onChange={(e) => setServoAngleLocal(Number(e.target.value))}
                className="w-full"
                style={{ accentColor: "#34d399" }}
              />
              <div className="flex justify-between">
                <span style={{ fontSize: 10, color: "#475569" }}>0° (wył)</span>
                <span style={{ fontSize: 10, color: "#475569" }}>90° (max)</span>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-2">
              <button
                onClick={() => setManualServo(servoAngle)}
                className="flex items-center justify-center gap-1.5 rounded-xl py-2.5 transition-all active:scale-95"
                style={{
                  background: "rgba(52,211,153,0.12)",
                  border: "1px solid rgba(52,211,153,0.3)",
                  color: "#34d399",
                  fontSize: 13,
                }}
              >
                <Sliders size={13} />
                Ustaw {servoAngle}°
              </button>
              <button
                onClick={() => setManualServo(null)}
                className="flex items-center justify-center gap-1.5 rounded-xl py-2.5 transition-all active:scale-95"
                style={{
                  background: "rgba(255,255,255,0.04)",
                  border: "1px solid rgba(255,255,255,0.08)",
                  color: "#6b7280",
                  fontSize: 13,
                }}
              >
                Wyczyść override
              </button>
            </div>

            <div className="flex flex-col gap-2">
              <label style={{ fontSize: 11, color: "#94a3b8" }}>
                Pre-off przed końcem harmonogramu: <span style={{ color: "#34d399" }}>{servoPreOff} min</span>
              </label>
              <input
                type="range"
                min="5"
                max="60"
                step="5"
                value={servoPreOff}
                onChange={(e) => setServoPreOffLocal(Number(e.target.value))}
                style={{ accentColor: "#34d399" }}
                className="w-full"
              />
            </div>
          </div>

          <button
            onClick={handleServoSave}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: servoSaved ? "rgba(74,222,128,0.15)" : "rgba(52,211,153,0.1)",
              border: `1px solid ${servoSaved ? "rgba(74,222,128,0.3)" : "rgba(52,211,153,0.2)"}`,
              color: servoSaved ? "#4ade80" : "#34d399",
            }}
          >
            {servoSaved ? <Check size={15} /> : <Save size={15} />}
            <span style={{ fontSize: 13 }}>{servoSaved ? "Zapisano!" : "Zapisz ustawienia serwa"}</span>
          </button>
        </div>

        {/* Display */}
        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="px-4 pt-3 pb-2">
            <SectionHeader icon={Monitor} title="Wyświetlacz OLED" color="#a78bfa" />
          </div>
          <div
            className="flex items-center px-4 py-3"
            style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}
          >
            <div className="flex-1">
              <p style={{ fontSize: 13, color: "#e2e8f0" }}>Zawsze włączony ekran</p>
              <p style={{ fontSize: 11, color: "#64748b" }}>Wyłącz, by oszczędzać energię (power-save 4 min)</p>
            </div>
            <button
              onClick={() => setAlwaysScreenOn(!state.alwaysScreenOn)}
              className="relative flex items-center rounded-full transition-all"
              style={{
                width: 44,
                height: 24,
                background: state.alwaysScreenOn ? "rgba(167,139,250,0.3)" : "rgba(255,255,255,0.08)",
                border: `1px solid ${state.alwaysScreenOn ? "rgba(167,139,250,0.5)" : "rgba(255,255,255,0.1)"}`,
                padding: "0 3px",
              }}
            >
              <div
                className="rounded-full transition-all"
                style={{
                  width: 18,
                  height: 18,
                  background: state.alwaysScreenOn ? "#a78bfa" : "#475569",
                  transform: state.alwaysScreenOn ? "translateX(18px)" : "translateX(0)",
                  boxShadow: state.alwaysScreenOn ? "0 0 8px rgba(167,139,250,0.6)" : "none",
                }}
              />
            </button>
          </div>
          <div
            className="px-4 py-3"
            style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}
          >
            <p style={{ fontSize: 12, color: "#64748b" }}>
              {state.alwaysScreenOn ? (
                <span className="flex items-center gap-1.5">
                  <Sun size={12} style={{ color: "#fbbf24" }} /> Ekran zawsze aktywny — Deep Sleep wyłączony
                </span>
              ) : (
                <span className="flex items-center gap-1.5">
                  <Moon size={12} style={{ color: "#94a3b8" }} /> Power-save po 4 min, Deep Sleep po 5 min
                </span>
              )}
            </p>
          </div>
        </div>

        {/* WiFi */}
        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="px-4 pt-3 pb-2">
            <SectionHeader icon={Wifi} title="Sieć WiFi" color="#38bdf8" />
          </div>

          <div
            className="flex items-center px-4 py-3"
            style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}
          >
            <div className="flex-1">
              <p style={{ fontSize: 13, color: "#e2e8f0" }}>
                {state.wifi.connected ? state.wifi.ssid : "Rozłączono"}
              </p>
              <p style={{ fontSize: 11, color: "#64748b" }}>
                {state.wifi.connected ? `IP: ${state.wifi.ip}` : "Tryb offline"}
              </p>
            </div>
            <div className="flex items-center gap-2">
              {state.wifi.connected ? (
                <>
                  <span style={{ fontSize: 11, color: rssiColor }}>{state.wifi.rssi} dBm</span>
                  <Wifi size={16} style={{ color: "#38bdf8" }} />
                </>
              ) : (
                <WifiOff size={16} style={{ color: "#6b7280" }} />
              )}
            </div>
          </div>

          <div
            className="flex items-center px-4 py-3"
            style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}
          >
            <div className="flex-1">
              <p style={{ fontSize: 13, color: "#e2e8f0" }}>Access Point (AP)</p>
              <p style={{ fontSize: 11, color: "#64748b" }}>
                {state.wifi.apActive ? `${state.wifi.apSsid} · ${state.wifi.apIp}` : "Wyłączony — uruchom ręcznie"}
              </p>
            </div>
            <button
              onClick={toggleAP}
              className="flex items-center gap-1.5 rounded-lg px-3 py-1.5 transition-all active:scale-95"
              style={{
                background: state.wifi.apActive ? "rgba(248,113,113,0.12)" : "rgba(56,189,248,0.1)",
                border: `1px solid ${state.wifi.apActive ? "rgba(248,113,113,0.3)" : "rgba(56,189,248,0.25)"}`,
                color: state.wifi.apActive ? "#f87171" : "#38bdf8",
                fontSize: 12,
              }}
            >
              <Radio size={12} />
              {state.wifi.apActive ? "Wyłącz AP" : "Uruchom AP"}
            </button>
          </div>
        </div>

        {/* Time sync */}
        <div
          className="rounded-2xl p-4"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <SectionHeader icon={RefreshCw} title="Synchronizacja czasu" color="#fbbf24" />
          <p style={{ fontSize: 12, color: "#64748b", marginBottom: 12 }}>
            Synchronizuje RTC urządzenia z czasem przeglądarki przez{" "}
            <span style={{ color: "#fbbf24" }}>POST /settime?epoch=&lt;unix&gt;</span>
          </p>
          <button
            onClick={handleTimeSync}
            disabled={syncingTime}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: timeSynced ? "rgba(74,222,128,0.12)" : "rgba(251,191,36,0.1)",
              border: `1px solid ${timeSynced ? "rgba(74,222,128,0.3)" : "rgba(251,191,36,0.25)"}`,
              color: timeSynced ? "#4ade80" : "#fbbf24",
            }}
          >
            {timeSynced ? (
              <Check size={15} />
            ) : (
              <RefreshCw size={15} className={syncingTime ? "animate-spin" : ""} />
            )}
            <span style={{ fontSize: 13 }}>
              {timeSynced ? "Czas zsynchronizowany!" : syncingTime ? "Synchronizuję..." : "Synchronizuj czas RTC"}
            </span>
          </button>
        </div>

        {/* OTA */}
        <div
          className="rounded-2xl p-4"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <SectionHeader icon={Upload} title="Aktualizacja OTA" color="#c084fc" />
          <p style={{ fontSize: 12, color: "#64748b", marginBottom: 12 }}>
            Prześlij plik <span style={{ color: "#c084fc" }}>.bin</span> skompilowany przez PlatformIO.
            Upload przez <span style={{ color: "#c084fc" }}>POST /update</span>
          </p>
          <label
            className="flex items-center justify-center gap-2 rounded-xl py-2.5 cursor-pointer transition-all active:scale-95"
            style={{
              background: "rgba(192,132,252,0.1)",
              border: "1px dashed rgba(192,132,252,0.3)",
              color: "#c084fc",
            }}
          >
            <Upload size={15} />
            <span style={{ fontSize: 13 }}>Wybierz plik firmware.bin</span>
            <input type="file" accept=".bin" className="hidden" />
          </label>
          <div
            className="mt-3 rounded-xl px-3 py-2 flex items-start gap-2"
            style={{
              background: "rgba(251,191,36,0.07)",
              border: "1px solid rgba(251,191,36,0.15)",
            }}
          >
            <AlertCircle size={12} style={{ color: "#fbbf24", marginTop: 2, flexShrink: 0 }} />
            <p style={{ fontSize: 11, color: "#94a3b8" }}>
              OTA blokuje Deep Sleep. Urządzenie restartuje się po udanym uploaddzie.
            </p>
          </div>
        </div>

        {/* System info */}
        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.02)",
            border: "1px solid rgba(255,255,255,0.06)",
          }}
        >
          <div className="px-4 pt-3 pb-2">
            <SectionHeader icon={Cpu} title="Informacje systemowe" color="#64748b" />
          </div>
          {[
            { label: "Firmware", value: "ESP32-S3 · v5.1" },
            { label: "Framework", value: "Arduino / FreeRTOS" },
            { label: "Czas pracy", value: state.uptime },
            { label: "Platforma", value: "esp32-s3-devkitc-1" },
            { label: "API", value: "/api/status · /api/logs · /api/action" },
          ].map(({ label, value }) => (
            <div
              key={label}
              className="flex items-center px-4 py-2.5"
              style={{ borderTop: "1px solid rgba(255,255,255,0.04)" }}
            >
              <span style={{ fontSize: 12, color: "#64748b", flex: 1 }}>{label}</span>
              <span style={{ fontSize: 12, color: "#94a3b8" }}>{value}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
