import { useCallback, useState, type ElementType } from "react";
import {
  Thermometer,
  Sliders,
  Upload,
  RefreshCw,
  Save,
  Check,
  AlertCircle,
  Cpu,
  Wind,
  Wifi,
  WifiOff,
  Clock3,
  Monitor,
  Flame,
  Radio,
  Link2,
  Plug,
  Unplug,
} from "lucide-react";
import { useDevice } from "../useDevice";

function SectionHeader({ icon: Icon, title, color }: { icon: ElementType; title: string; color: string }) {
  return (
    <div className="flex items-center gap-2 mb-3">
      <div className="rounded-xl p-1.5" style={{ background: `${color}18` }}>
        <Icon size={15} style={{ color }} />
      </div>
      <span style={{ fontSize: 13, color: "#94a3b8" }}>{title}</span>
    </div>
  );
}

export function Settings() {
  const {
    state,
    apiBaseUrl,
    apiBaseSource,
    setApiBaseUrl,
    resetApiBaseUrl,
    setTemp,
    setServoPreOffMins,
    setManualServo,
    syncTime,
    uploadFirmware,
    refresh,
    setAPMode,
    setAlwaysScreenOn,
    setHeaterEnabled,
  } = useDevice();

  const [targetTemp, setTargetTemp] = useState(state.targetTemp.toFixed(1));
  const [hysteresis, setHysteresis] = useState(state.hysteresis.toFixed(1));
  const [tempSaved, setTempSaved] = useState(false);

  const [servoAngle, setServoAngle] = useState(state.aerationAngle);
  const [servoPreOff, setServoPreOff] = useState(state.schedule.servoPreOffMins);
  const [servoSaved, setServoSaved] = useState(false);

  const [syncingTime, setSyncingTime] = useState(false);
  const [timeSynced, setTimeSynced] = useState(false);

  const [otaFile, setOtaFile] = useState<File | null>(null);
  const [uploadingOta, setUploadingOta] = useState(false);
  const [otaResult, setOtaResult] = useState<"idle" | "ok" | "error">("idle");

  const [togglingAp, setTogglingAp] = useState(false);
  const [togglingDisplay, setTogglingDisplay] = useState(false);
  const [togglingHeater, setTogglingHeater] = useState(false);
  const [actionInfo, setActionInfo] = useState<{ type: "ok" | "error"; msg: string } | null>(null);
  const [deviceUrlInput, setDeviceUrlInput] = useState(apiBaseSource === "manual" ? apiBaseUrl : "");

  const showInfo = useCallback((type: "ok" | "error", msg: string) => {
    setActionInfo({ type, msg });
    setTimeout(() => setActionInfo(null), 3500);
  }, []);

  const handleDeviceConnect = () => {
    const result = setApiBaseUrl(deviceUrlInput);
    if (!result.ok) {
      showInfo("error", result.error);
      return;
    }
    setDeviceUrlInput(result.normalized);
    showInfo("ok", `Ustawiono target API: ${result.normalized}.`);
  };

  const handleDeviceReset = () => {
    resetApiBaseUrl();
    setDeviceUrlInput("");
    showInfo("ok", "Przywrocono domyslne polaczenie API.");
  };

  const handleTempSave = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    const parsedTemp = parseFloat(targetTemp);
    const parsedHyst = parseFloat(hysteresis);

    if (!Number.isFinite(parsedTemp) || !Number.isFinite(parsedHyst)) {
      return;
    }

    const ok = await setTemp(parsedTemp, parsedHyst);
    if (ok) {
      setTempSaved(true);
      setTimeout(() => setTempSaved(false), 2000);
      showInfo("ok", "Temperatura zapisana w sterowniku.");
    } else {
      showInfo("error", "Nie udalo sie zapisac temperatury.");
    }
  };

  const handleServoPreOffSave = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    const ok = await setServoPreOffMins(servoPreOff);
    if (ok) {
      setServoSaved(true);
      setTimeout(() => setServoSaved(false), 2000);
      showInfo("ok", "Ustawienia serwa zapisane.");
    } else {
      showInfo("error", "Nie udalo sie zapisac ustawien serwa.");
    }
  };

  const handleTimeSync = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    setSyncingTime(true);
    const ok = await syncTime();
    setSyncingTime(false);
    if (ok) {
      setTimeSynced(true);
      setTimeout(() => setTimeSynced(false), 3000);
      showInfo("ok", "Czas RTC zsynchronizowany.");
    } else {
      showInfo("error", "Synchronizacja czasu nieudana.");
    }
  };

  const handleOtaUpload = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    if (!otaFile) {
      return;
    }

    setUploadingOta(true);
    setOtaResult("idle");
    const ok = await uploadFirmware(otaFile);
    setUploadingOta(false);
    setOtaResult(ok ? "ok" : "error");
    showInfo(ok ? "ok" : "error", ok ? "Plik OTA wyslany." : "Upload OTA nieudany.");
  };

  const handleApToggle = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    setTogglingAp(true);
    const ok = await setAPMode(!state.wifi.apMode);
    setTogglingAp(false);
    showInfo(ok ? "ok" : "error", ok ? "Tryb AP zmieniony." : "Nie udalo sie zmienic AP.");
  };

  const handleDisplayToggle = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    setTogglingDisplay(true);
    const ok = await setAlwaysScreenOn(!state.settings.alwaysScreenOn);
    setTogglingDisplay(false);
    showInfo(ok ? "ok" : "error", ok ? "Ustawienie wyswietlacza zapisane." : "Nie udalo sie zapisac ustawienia wyswietlacza.");
  };

  const handleHeaterToggle = async () => {
    if (!state.online) {
      showInfo("error", "Brak polaczenia z API sterownika.");
      return;
    }

    setTogglingHeater(true);
    const ok = await setHeaterEnabled(!state.settings.heaterEnabled);
    setTogglingHeater(false);
    showInfo(ok ? "ok" : "error", ok ? "Opcja grzalki zapisana." : "Nie udalo sie zapisac opcji grzalki.");
  };

  const apiModeLabel =
    apiBaseSource === "manual"
      ? "Reczny adres urzadzenia"
      : apiBaseSource === "env"
        ? "VITE_API_BASE_URL"
        : "Vite proxy (/api...)";
  const apiTargetLabel = apiBaseSource === "proxy" ? "Proxy do hosta UI" : apiBaseUrl;

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
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
        <p style={{ fontSize: 12, color: "#64748b", marginTop: 2 }}>Konfiguracja API i sterownika</p>
      </div>

      <div className="flex flex-col gap-4 px-4 pt-4 pb-4">
        {!state.online && (
          <div
            className="rounded-2xl px-4 py-3"
            style={{
              background: "rgba(248,113,113,0.10)",
              border: "1px solid rgba(248,113,113,0.25)",
            }}
          >
            <p style={{ fontSize: 12, color: "#fecaca" }}>
              Brak polaczenia z API sterownika. Wybierz IP/URL ESP32 ponizej albo uzyj proxy Vite.
            </p>
          </div>
        )}

        {actionInfo && (
          <div
            className="rounded-2xl px-4 py-2.5"
            style={{
              background: actionInfo.type === "ok" ? "rgba(74,222,128,0.12)" : "rgba(248,113,113,0.12)",
              border: `1px solid ${actionInfo.type === "ok" ? "rgba(74,222,128,0.3)" : "rgba(248,113,113,0.3)"}`,
            }}
          >
            <p style={{ fontSize: 12, color: actionInfo.type === "ok" ? "#86efac" : "#fecaca" }}>{actionInfo.msg}</p>
          </div>
        )}

        <div
          className="rounded-2xl p-4"
          style={{
            background: "linear-gradient(135deg, rgba(56,189,248,0.09) 0%, rgba(8,14,28,0.92) 100%)",
            border: "1px solid rgba(56,189,248,0.24)",
          }}
        >
          <SectionHeader icon={Link2} title="Polaczenie z urzadzeniem" color="#38bdf8" />
          <p style={{ fontSize: 12, color: "#94a3b8", marginBottom: 10 }}>
            Wpisz IP lub URL sterownika. Ustawienie zapamietuje sie lokalnie i pozostaje aktywne do recznego odlaczenia.
          </p>

          <label style={{ fontSize: 11, color: "#94a3b8", display: "block", marginBottom: 6 }}>Adres ESP32</label>
          <input
            type="text"
            value={deviceUrlInput}
            onChange={(event) => setDeviceUrlInput(event.target.value)}
            placeholder="np. 192.168.1.105 albo http://192.168.4.1"
            className="w-full rounded-xl px-3 py-2.5 outline-none"
            style={{
              background: "rgba(255,255,255,0.06)",
              border: "1px solid rgba(255,255,255,0.1)",
              color: "#e2e8f0",
              fontSize: 14,
            }}
          />

          <div className="grid grid-cols-2 gap-2 mt-3">
            <button
              onClick={handleDeviceConnect}
              disabled={state.loading || !deviceUrlInput.trim()}
              className="flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
              style={{
                background: "rgba(56,189,248,0.12)",
                border: "1px solid rgba(56,189,248,0.3)",
                color: "#38bdf8",
                opacity: state.loading || !deviceUrlInput.trim() ? 0.55 : 1,
              }}
            >
              {state.loading ? <RefreshCw size={14} className="animate-spin" /> : <Plug size={14} />}
              Polacz
            </button>

            <button
              onClick={handleDeviceReset}
              disabled={state.loading || apiBaseSource !== "manual"}
              className="flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
              style={{
                background: "rgba(71,85,105,0.14)",
                border: "1px solid rgba(71,85,105,0.35)",
                color: "#94a3b8",
                opacity: state.loading || apiBaseSource !== "manual" ? 0.55 : 1,
              }}
            >
              <Unplug size={14} />
              Przywroc domyslne
            </button>
          </div>

          <div
            className="mt-3 rounded-xl px-3 py-2.5"
            style={{
              background: "rgba(2,6,23,0.5)",
              border: "1px solid rgba(148,163,184,0.2)",
            }}
          >
            <p style={{ fontSize: 12, color: "#cbd5e1" }}>
              Aktywny target: <span style={{ color: "#e2e8f0" }}>{apiTargetLabel}</span>
            </p>
            <p style={{ fontSize: 11, color: "#94a3b8", marginTop: 3 }}>Tryb: {apiModeLabel}</p>
          </div>
        </div>

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
              <label style={{ fontSize: 11, color: "#94a3b8" }}>Docelowa (C)</label>
              <input
                type="number"
                step="0.1"
                min="15"
                max="35"
                value={targetTemp}
                onChange={(event) => setTargetTemp(event.target.value)}
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
              <label style={{ fontSize: 11, color: "#94a3b8" }}>Histereza (C)</label>
              <input
                type="number"
                step="0.1"
                min="0.1"
                max="5"
                value={hysteresis}
                onChange={(event) => setHysteresis(event.target.value)}
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

          <button
            onClick={handleTempSave}
            disabled={!state.online}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: tempSaved ? "rgba(74,222,128,0.15)" : "rgba(251,113,133,0.12)",
              border: `1px solid ${tempSaved ? "rgba(74,222,128,0.3)" : "rgba(251,113,133,0.25)"}`,
              color: tempSaved ? "#4ade80" : "#fb7185",
              opacity: state.online ? 1 : 0.5,
            }}
          >
            {tempSaved ? <Check size={15} /> : <Save size={15} />}
            <span style={{ fontSize: 13 }}>{tempSaved ? "Zapisano" : "Zapisz temperature"}</span>
          </button>

          <button
            onClick={handleHeaterToggle}
            disabled={togglingHeater || !state.online}
            className="w-full mt-3 flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: state.settings.heaterEnabled ? "rgba(251,146,60,0.12)" : "rgba(71,85,105,0.14)",
              border: `1px solid ${state.settings.heaterEnabled ? "rgba(251,146,60,0.35)" : "rgba(71,85,105,0.35)"}`,
              color: state.settings.heaterEnabled ? "#fb923c" : "#94a3b8",
              opacity: state.online ? 1 : 0.5,
            }}
          >
            {togglingHeater ? <RefreshCw size={14} className="animate-spin" /> : <Flame size={14} />}
            {state.settings.heaterEnabled ? "Wylacz opcje grzalki" : "Wlacz opcje grzalki"}
          </button>
        </div>

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
                Reczny kat serwa: <span style={{ color: "#34d399" }}>{servoAngle}deg</span>
              </label>
              <input
                type="range"
                min="0"
                max="90"
                value={servoAngle}
                onChange={(event) => setServoAngle(Number(event.target.value))}
                className="w-full"
                style={{ accentColor: "#34d399" }}
              />
              <div className="grid grid-cols-2 gap-2">
                <button
                  onClick={() => void setManualServo(servoAngle)}
                  disabled={!state.online}
                  className="flex items-center justify-center gap-1.5 rounded-xl py-2.5 transition-all active:scale-95"
                  style={{
                    background: "rgba(52,211,153,0.12)",
                    border: "1px solid rgba(52,211,153,0.3)",
                    color: "#34d399",
                    fontSize: 13,
                    opacity: state.online ? 1 : 0.5,
                  }}
                >
                  <Sliders size={13} />
                  Ustaw
                </button>
                <button
                  onClick={() => void setManualServo(null)}
                  disabled={!state.online}
                  className="flex items-center justify-center gap-1.5 rounded-xl py-2.5 transition-all active:scale-95"
                  style={{
                    background: "rgba(255,255,255,0.04)",
                    border: "1px solid rgba(255,255,255,0.08)",
                    color: "#6b7280",
                    fontSize: 13,
                    opacity: state.online ? 1 : 0.5,
                  }}
                >
                  Wyczysc override
                </button>
              </div>
            </div>

            <div className="flex flex-col gap-2">
              <label style={{ fontSize: 11, color: "#94a3b8" }}>
                Pre-off przed koncem harmonogramu: <span style={{ color: "#34d399" }}>{servoPreOff} min</span>
              </label>
              <input
                type="range"
                min="0"
                max="120"
                step="5"
                value={servoPreOff}
                onChange={(event) => setServoPreOff(Number(event.target.value))}
                style={{ accentColor: "#34d399" }}
                className="w-full"
              />
            </div>
          </div>

          <button
            onClick={handleServoPreOffSave}
            disabled={!state.online}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: servoSaved ? "rgba(74,222,128,0.15)" : "rgba(52,211,153,0.1)",
              border: `1px solid ${servoSaved ? "rgba(74,222,128,0.3)" : "rgba(52,211,153,0.2)"}`,
              color: servoSaved ? "#4ade80" : "#34d399",
              opacity: state.online ? 1 : 0.5,
            }}
          >
            {servoSaved ? <Check size={15} /> : <Save size={15} />}
            <span style={{ fontSize: 13 }}>{servoSaved ? "Zapisano" : "Zapisz servo pre-off"}</span>
          </button>
        </div>

        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="px-4 pt-3 pb-2">
            <SectionHeader icon={Wifi} title="Siec i AP" color="#38bdf8" />
          </div>

          <div className="flex items-center px-4 py-3" style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}>
            <div className="flex-1">
              <p style={{ fontSize: 13, color: "#e2e8f0" }}>Tryb: {state.wifi.modeLabel}</p>
              <p style={{ fontSize: 11, color: "#64748b" }}>IP: {state.wifi.ip}</p>
            </div>
            {state.online ? <Wifi size={16} style={{ color: "#38bdf8" }} /> : <WifiOff size={16} style={{ color: "#6b7280" }} />}
          </div>

          <div className="px-4 py-3" style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}>
            <button
              onClick={handleApToggle}
              disabled={togglingAp || !state.online}
              className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
              style={{
                background: state.wifi.apMode ? "rgba(239,68,68,0.12)" : "rgba(56,189,248,0.1)",
                border: `1px solid ${state.wifi.apMode ? "rgba(239,68,68,0.3)" : "rgba(56,189,248,0.25)"}`,
                color: state.wifi.apMode ? "#ef4444" : "#38bdf8",
                opacity: state.online ? 1 : 0.5,
              }}
            >
              {togglingAp ? <RefreshCw size={14} className="animate-spin" /> : <Radio size={14} />}
              {state.wifi.apMode ? "Wylacz AP" : "Uruchom AP"}
            </button>

            <button
              onClick={() => void refresh()}
              className="w-full mt-2 flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
              style={{
                background: "rgba(56,189,248,0.1)",
                border: "1px solid rgba(56,189,248,0.25)",
                color: "#38bdf8",
              }}
            >
              <RefreshCw size={14} />
              Odswiez status
            </button>
          </div>
        </div>

        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="px-4 pt-3 pb-2">
            <SectionHeader icon={Monitor} title="Wyswietlacz" color="#a78bfa" />
          </div>
          <div className="px-4 py-3" style={{ borderTop: "1px solid rgba(255,255,255,0.05)" }}>
            <button
              onClick={handleDisplayToggle}
              disabled={togglingDisplay || !state.online}
              className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
              style={{
                background: state.settings.alwaysScreenOn ? "rgba(167,139,250,0.2)" : "rgba(71,85,105,0.14)",
                border: `1px solid ${state.settings.alwaysScreenOn ? "rgba(167,139,250,0.45)" : "rgba(71,85,105,0.35)"}`,
                color: state.settings.alwaysScreenOn ? "#a78bfa" : "#94a3b8",
                opacity: state.online ? 1 : 0.5,
              }}
            >
              {togglingDisplay ? <RefreshCw size={14} className="animate-spin" /> : <Monitor size={14} />}
              {state.settings.alwaysScreenOn ? "Wylacz always screen" : "Wlacz always screen"}
            </button>
          </div>
        </div>

        <div
          className="rounded-2xl p-4"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <SectionHeader icon={Clock3} title="Synchronizacja czasu" color="#fbbf24" />
          <p style={{ fontSize: 12, color: "#64748b", marginBottom: 12 }}>
            Wysyla POST /settime?epoch=&lt;unix&gt; do urzadzenia.
          </p>
          <button
            onClick={handleTimeSync}
            disabled={syncingTime || !state.online}
            className="w-full flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: timeSynced ? "rgba(74,222,128,0.12)" : "rgba(251,191,36,0.1)",
              border: `1px solid ${timeSynced ? "rgba(74,222,128,0.3)" : "rgba(251,191,36,0.25)"}`,
              color: timeSynced ? "#4ade80" : "#fbbf24",
              opacity: state.online ? 1 : 0.5,
            }}
          >
            {timeSynced ? <Check size={15} /> : <RefreshCw size={15} className={syncingTime ? "animate-spin" : ""} />}
            <span style={{ fontSize: 13 }}>
              {timeSynced ? "Czas zsynchronizowany" : syncingTime ? "Synchronizacja..." : "Synchronizuj RTC"}
            </span>
          </button>
        </div>

        <div
          className="rounded-2xl p-4"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <SectionHeader icon={Upload} title="Aktualizacja OTA" color="#c084fc" />
          <p style={{ fontSize: 12, color: "#64748b", marginBottom: 12 }}>
            Upload firmware `.bin` przez POST /update.
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
            <span style={{ fontSize: 13 }}>{otaFile ? otaFile.name : "Wybierz plik firmware.bin"}</span>
            <input
              type="file"
              accept=".bin"
              className="hidden"
              onChange={(event) => setOtaFile(event.target.files?.[0] ?? null)}
            />
          </label>

          <button
            onClick={handleOtaUpload}
            disabled={!otaFile || uploadingOta || !state.online}
            className="w-full mt-3 flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all active:scale-95"
            style={{
              background: otaResult === "ok" ? "rgba(74,222,128,0.12)" : "rgba(192,132,252,0.1)",
              border: `1px solid ${otaResult === "ok" ? "rgba(74,222,128,0.35)" : "rgba(192,132,252,0.3)"}`,
              color: otaResult === "ok" ? "#4ade80" : "#c084fc",
              opacity: !otaFile || uploadingOta || !state.online ? 0.6 : 1,
            }}
          >
            {uploadingOta ? <RefreshCw size={14} className="animate-spin" /> : <Upload size={14} />}
            {uploadingOta ? "Wgrywanie..." : otaResult === "ok" ? "Upload OK" : "Wyslij OTA"}
          </button>

          {otaResult === "error" && (
            <div
              className="mt-3 rounded-xl px-3 py-2 flex items-start gap-2"
              style={{
                background: "rgba(248,113,113,0.08)",
                border: "1px solid rgba(248,113,113,0.2)",
              }}
            >
              <AlertCircle size={12} style={{ color: "#f87171", marginTop: 2, flexShrink: 0 }} />
              <p style={{ fontSize: 11, color: "#fecaca" }}>
                Upload nieudany. Sprawdz polaczenie i format pliku `.bin`.
              </p>
            </div>
          )}
        </div>

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
            { label: "Firmware", value: "Sterownik Akwarium PRO v5.1" },
            { label: "API", value: "/api/status /api/logs /api/action" },
            { label: "Dodatkowe", value: "/settime /update" },
            { label: "API target", value: apiTargetLabel },
            { label: "Tryb polaczenia", value: apiModeLabel },
            {
              label: "Ostatnia synchronizacja",
              value: state.lastSyncAt
                ? new Date(state.lastSyncAt).toLocaleString("pl-PL", {
                    day: "2-digit",
                    month: "2-digit",
                    hour: "2-digit",
                    minute: "2-digit",
                    second: "2-digit",
                  })
                : "Brak",
            },
            { label: "Status API", value: state.online ? "Online" : "Offline" },
            { label: "Ostatni blad", value: state.lastError ?? "Brak" },
          ].map(({ label, value }) => (
            <div key={label} className="flex items-center px-4 py-2.5" style={{ borderTop: "1px solid rgba(255,255,255,0.04)" }}>
              <span style={{ fontSize: 12, color: "#64748b", flex: 1 }}>{label}</span>
              <span style={{ fontSize: 12, color: "#94a3b8", textAlign: "right", maxWidth: "62%" }}>{value}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
