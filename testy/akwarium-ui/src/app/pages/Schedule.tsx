import { useState, type ElementType } from "react";
import { Lightbulb, Filter, Wind, Fish, Save, Check, Clock } from "lucide-react";
import { useDevice } from "../useDevice";
import type { DeviceSchedule } from "../deviceStore";

type ScheduleKey = "light" | "aeration" | "filter";

const tabs: { key: ScheduleKey | "feeding"; label: string; icon: ElementType; color: string }[] = [
  { key: "light", label: "Swiatlo", icon: Lightbulb, color: "#fbbf24" },
  { key: "filter", label: "Filtr", icon: Filter, color: "#60a5fa" },
  { key: "aeration", label: "Powietrze", icon: Wind, color: "#34d399" },
  { key: "feeding", label: "Karmienie", icon: Fish, color: "#f472b6" },
];

function TimeInput({
  label,
  value,
  onChange,
}: {
  label: string;
  value: string;
  onChange: (value: string) => void;
}) {
  return (
    <div className="flex flex-col gap-2">
      <div className="flex items-center gap-1.5">
        <Clock size={13} style={{ color: "#64748b" }} />
        <span style={{ fontSize: 12, color: "#94a3b8" }}>{label}</span>
      </div>
      <input
        type="time"
        value={value}
        onChange={(event) => onChange(event.target.value)}
        className="rounded-xl px-4 py-3 outline-none transition-all"
        style={{
          background: "rgba(255,255,255,0.05)",
          border: "1px solid rgba(255,255,255,0.1)",
          color: "#e2e8f0",
          fontSize: 16,
          colorScheme: "dark",
        }}
      />
    </div>
  );
}

function durationLabel(start: string, end: string): string {
  const [startH, startM] = start.split(":").map(Number);
  const [endH, endM] = end.split(":").map(Number);

  if (!Number.isFinite(startH) || !Number.isFinite(startM) || !Number.isFinite(endH) || !Number.isFinite(endM)) {
    return "-";
  }

  const startMinutes = startH * 60 + startM;
  const endMinutes = endH * 60 + endM;
  let diff = endMinutes - startMinutes;
  if (diff <= 0) {
    diff += 24 * 60;
  }

  const hours = Math.floor(diff / 60);
  const minutes = diff % 60;
  if (hours > 0) {
    return `${hours}h ${minutes}min`;
  }
  return `${minutes}min`;
}

export function Schedule() {
  const { state, saveSchedule } = useDevice();
  const [activeTab, setActiveTab] = useState<(typeof tabs)[0]["key"]>("light");
  const [localSchedule, setLocalSchedule] = useState<DeviceSchedule>(state.schedule);
  const [saved, setSaved] = useState(false);
  const [dirty, setDirty] = useState(false);

  const effectiveSchedule = dirty ? localSchedule : state.schedule;

  const handleSave = async () => {
    const ok = await saveSchedule(effectiveSchedule);
    if (ok) {
      setSaved(true);
      setDirty(false);
      setTimeout(() => setSaved(false), 2000);
    }
  };

  const updateDeviceSchedule = (key: ScheduleKey, field: "start" | "end", value: string) => {
    const base = dirty ? localSchedule : state.schedule;
    setDirty(true);
    setLocalSchedule({
      ...base,
      [key]: { ...base[key], [field]: value },
    });
  };

  const updateFeedingMode = (mode: number) => {
    const base = dirty ? localSchedule : state.schedule;
    setDirty(true);
    setLocalSchedule({
      ...base,
      feeding: { ...base.feeding, mode },
    });
  };

  const updateFeedingTime = (value: string) => {
    const base = dirty ? localSchedule : state.schedule;
    setDirty(true);
    setLocalSchedule({
      ...base,
      feeding: { ...base.feeding, time: value },
    });
  };

  const activeTabConfig = tabs.find((tab) => tab.key === activeTab)!;

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
      <div
        className="px-5 pt-12 pb-5"
        style={{
          background: "linear-gradient(160deg, #0c1a33 0%, #080e1c 100%)",
          borderBottom: "1px solid rgba(6,182,212,0.1)",
        }}
      >
        <p style={{ fontSize: 11, color: "#64748b", letterSpacing: "0.12em" }} className="uppercase mb-1">
          Sterownik Akwarium PRO
        </p>
        <h1 style={{ fontSize: 22, color: "#e2e8f0" }}>Harmonogram</h1>
        <p style={{ fontSize: 12, color: "#64748b", marginTop: 2 }}>
          Ustaw godziny dla swiatla, filtra, napowietrzania i karmienia
        </p>
      </div>

      <div className="px-4 pt-4 pb-2">
        <div
          className="flex rounded-2xl p-1 gap-1"
          style={{ background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.06)" }}
        >
          {tabs.map(({ key, label, icon: Icon, color }) => (
            <button
              key={key}
              onClick={() => setActiveTab(key)}
              className="flex-1 flex flex-col items-center gap-1 rounded-xl py-2.5 transition-all"
              style={{
                background: activeTab === key ? `${color}18` : "transparent",
                border: `1px solid ${activeTab === key ? `${color}40` : "transparent"}`,
              }}
            >
              <Icon size={16} style={{ color: activeTab === key ? color : "#6b7280" }} />
              <span style={{ fontSize: 10, color: activeTab === key ? "#e2e8f0" : "#6b7280" }}>{label}</span>
            </button>
          ))}
        </div>
      </div>

      <div className="px-4 pt-2 pb-4 flex flex-col gap-4">
        {activeTab !== "feeding" ? (
          <div
            className="rounded-2xl p-4 flex flex-col gap-4"
            style={{
              background: `linear-gradient(135deg, ${activeTabConfig.color}10 0%, rgba(8,14,28,0.8) 100%)`,
              border: `1px solid ${activeTabConfig.color}25`,
            }}
          >
            <div className="flex items-center gap-2">
              <div className="rounded-xl p-2" style={{ background: `${activeTabConfig.color}20` }}>
                <activeTabConfig.icon size={18} style={{ color: activeTabConfig.color }} />
              </div>
              <div>
                <h3 style={{ fontSize: 15, color: "#e2e8f0" }}>{activeTabConfig.label}</h3>
                <p style={{ fontSize: 11, color: "#64748b" }}>Okno czasowe aktywnosci</p>
              </div>
            </div>

            <div
              className="rounded-xl p-3"
              style={{
                background: "rgba(255,255,255,0.04)",
                border: "1px solid rgba(255,255,255,0.06)",
              }}
            >
              <div className="flex items-center justify-center gap-3" style={{ marginBottom: 4 }}>
                <span style={{ fontSize: 13, color: "#64748b" }}>Aktywny od</span>
                <span
                  style={{
                    fontSize: 28,
                    color: activeTabConfig.color,
                    letterSpacing: "-0.02em",
                    fontVariantNumeric: "tabular-nums",
                  }}
                >
                  {effectiveSchedule[activeTab as ScheduleKey].start}
                </span>
                <span style={{ fontSize: 13, color: "#64748b" }}>do</span>
                <span
                  style={{
                    fontSize: 28,
                    color: activeTabConfig.color,
                    letterSpacing: "-0.02em",
                    fontVariantNumeric: "tabular-nums",
                  }}
                >
                  {effectiveSchedule[activeTab as ScheduleKey].end}
                </span>
              </div>
            </div>

            <div className="grid grid-cols-2 gap-3">
              <TimeInput
                label="Godzina startu"
                value={effectiveSchedule[activeTab as ScheduleKey].start}
                onChange={(value) => updateDeviceSchedule(activeTab as ScheduleKey, "start", value)}
              />
              <TimeInput
                label="Godzina konca"
                value={effectiveSchedule[activeTab as ScheduleKey].end}
                onChange={(value) => updateDeviceSchedule(activeTab as ScheduleKey, "end", value)}
              />
            </div>

            <div
              className="rounded-xl px-3 py-2.5 flex items-center gap-2"
              style={{
                background: `${activeTabConfig.color}10`,
                border: `1px solid ${activeTabConfig.color}20`,
              }}
            >
              <Clock size={13} style={{ color: activeTabConfig.color }} />
              <span style={{ fontSize: 12, color: "#94a3b8" }}>
                Czas aktywnosci: {durationLabel(effectiveSchedule[activeTab as ScheduleKey].start, effectiveSchedule[activeTab as ScheduleKey].end)}
              </span>
            </div>
          </div>
        ) : (
          <div
            className="rounded-2xl p-4 flex flex-col gap-4"
            style={{
              background: "linear-gradient(135deg, rgba(244,114,182,0.1) 0%, rgba(8,14,28,0.8) 100%)",
              border: "1px solid rgba(244,114,182,0.25)",
            }}
          >
            <div className="flex items-center gap-2">
              <div className="rounded-xl p-2" style={{ background: "rgba(244,114,182,0.2)" }}>
                <Fish size={18} style={{ color: "#f472b6" }} />
              </div>
              <div>
                <h3 style={{ fontSize: 15, color: "#e2e8f0" }}>Karmienie automatyczne</h3>
                <p style={{ fontSize: 11, color: "#64748b" }}>Tryb i godzina karmienia</p>
              </div>
            </div>

            <div className="flex flex-col gap-2">
              <span style={{ fontSize: 12, color: "#94a3b8" }}>Tryb karmienia</span>
              <div className="grid grid-cols-2 gap-2">
                {[
                  { mode: 0, label: "Wylaczone" },
                  { mode: 1, label: "Codziennie" },
                  { mode: 2, label: "Co 2 dni" },
                  { mode: 3, label: "Co 3 dni" },
                ].map(({ mode, label }) => (
                  <button
                    key={mode}
                    onClick={() => updateFeedingMode(mode)}
                    className="py-2.5 rounded-xl transition-all"
                    style={{
                      background: effectiveSchedule.feeding.mode === mode ? "rgba(244,114,182,0.2)" : "rgba(255,255,255,0.04)",
                      border: `1px solid ${effectiveSchedule.feeding.mode === mode ? "rgba(244,114,182,0.4)" : "rgba(255,255,255,0.06)"}`,
                      color: effectiveSchedule.feeding.mode === mode ? "#f472b6" : "#6b7280",
                      fontSize: 13,
                    }}
                  >
                    {label}
                  </button>
                ))}
              </div>
            </div>

            {effectiveSchedule.feeding.mode !== 0 && (
              <TimeInput label="Godzina karmienia" value={effectiveSchedule.feeding.time} onChange={updateFeedingTime} />
            )}

            <div
              className="rounded-xl px-3 py-2.5"
              style={{
                background: "rgba(244,114,182,0.08)",
                border: "1px solid rgba(244,114,182,0.15)",
              }}
            >
              <p style={{ fontSize: 12, color: "#94a3b8" }}>
                Firmware wspiera tryby: 0 (off), 1 (codziennie), 2 (co 2 dni), 3 (co 3 dni).
              </p>
            </div>
          </div>
        )}

        <button
          onClick={handleSave}
          className="flex items-center justify-center gap-2 rounded-2xl py-3.5 transition-all active:scale-95"
          style={{
            background: saved
              ? "linear-gradient(135deg, #059669, #047857)"
              : "linear-gradient(135deg, #0891b2, #0e7490)",
            boxShadow: saved ? "0 0 20px rgba(5,150,105,0.4)" : "0 0 20px rgba(8,145,178,0.3)",
            color: "#fff",
          }}
        >
          {saved ? <Check size={18} /> : <Save size={18} />}
          <span style={{ fontSize: 15 }}>{saved ? "Zapisano" : "Zapisz harmonogram"}</span>
        </button>

        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.06)",
          }}
        >
          <div className="px-4 py-3" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
            <span style={{ fontSize: 12, color: "#94a3b8" }}>Podglad zapisanych godzin</span>
          </div>
          {[
            {
              label: "Swiatlo",
              icon: Lightbulb,
              color: "#fbbf24",
              value: `${effectiveSchedule.light.start} - ${effectiveSchedule.light.end}`,
            },
            {
              label: "Filtr",
              icon: Filter,
              color: "#60a5fa",
              value: `${effectiveSchedule.filter.start} - ${effectiveSchedule.filter.end}`,
            },
            {
              label: "Napowietrzanie",
              icon: Wind,
              color: "#34d399",
              value: `${effectiveSchedule.aeration.start} - ${effectiveSchedule.aeration.end}`,
            },
            {
              label: "Karmienie",
              icon: Fish,
              color: "#f472b6",
              value:
                effectiveSchedule.feeding.mode === 0
                  ? "Wylaczone"
                  : `${effectiveSchedule.feeding.time} (tryb ${effectiveSchedule.feeding.mode})`,
            },
          ].map(({ label, icon: Icon, color, value }) => (
            <div
              key={label}
              className="flex items-center gap-3 px-4 py-3"
              style={{ borderTop: "1px solid rgba(255,255,255,0.04)" }}
            >
              <div className="rounded-lg p-1.5" style={{ background: `${color}15` }}>
                <Icon size={14} style={{ color }} />
              </div>
              <span style={{ fontSize: 13, color: "#cbd5e1", flex: 1 }}>{label}</span>
              <span style={{ fontSize: 12, color: "#94a3b8" }}>{value}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
