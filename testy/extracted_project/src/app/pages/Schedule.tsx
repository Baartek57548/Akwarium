import { useState } from "react";
import { Lightbulb, Filter, Wind, Fish, Save, Check, Clock, ToggleLeft, ToggleRight } from "lucide-react";
import { useDevice, DeviceState } from "../deviceContext";

type ScheduleKey = "light" | "aeration" | "filter";

const tabs: { key: ScheduleKey | "feeding"; label: string; icon: React.ElementType; color: string }[] = [
  { key: "light", label: "Światło", icon: Lightbulb, color: "#fbbf24" },
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
  onChange: (v: string) => void;
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
        onChange={(e) => onChange(e.target.value)}
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

export function Schedule() {
  const { state, saveSchedule } = useDevice();
  const [activeTab, setActiveTab] = useState<typeof tabs[0]["key"]>("light");
  const [localSchedule, setLocalSchedule] = useState(state.schedule);
  const [saved, setSaved] = useState(false);

  const handleSave = () => {
    saveSchedule(localSchedule);
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  const updateDeviceSchedule = (
    key: ScheduleKey,
    field: "start" | "end" | "enabled",
    value: string | boolean
  ) => {
    setLocalSchedule((prev) => ({
      ...prev,
      [key]: { ...prev[key], [field]: value },
    }));
  };

  const activeTabConfig = tabs.find((t) => t.key === activeTab)!;

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
      {/* Header */}
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
          Ustaw okna czasowe dla każdego urządzenia
        </p>
      </div>

      {/* Tab selector */}
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
                border: `1px solid ${activeTab === key ? color + "40" : "transparent"}`,
              }}
            >
              <Icon size={16} style={{ color: activeTab === key ? color : "#6b7280" }} />
              <span
                style={{
                  fontSize: 10,
                  color: activeTab === key ? "#e2e8f0" : "#6b7280",
                }}
              >
                {label}
              </span>
            </button>
          ))}
        </div>
      </div>

      {/* Content */}
      <div className="px-4 pt-2 pb-4 flex flex-col gap-4">
        {activeTab !== "feeding" ? (
          <div
            className="rounded-2xl p-4 flex flex-col gap-4"
            style={{
              background: `linear-gradient(135deg, ${activeTabConfig.color}10 0%, rgba(8,14,28,0.8) 100%)`,
              border: `1px solid ${activeTabConfig.color}25`,
            }}
          >
            <div className="flex items-center justify-between">
              <div className="flex items-center gap-2">
                <div
                  className="rounded-xl p-2"
                  style={{ background: `${activeTabConfig.color}20` }}
                >
                  <activeTabConfig.icon size={18} style={{ color: activeTabConfig.color }} />
                </div>
                <div>
                  <h3 style={{ fontSize: 15, color: "#e2e8f0" }}>{activeTabConfig.label}</h3>
                  <p style={{ fontSize: 11, color: "#64748b" }}>Okno czasowe aktywności</p>
                </div>
              </div>
              <button
                onClick={() =>
                  updateDeviceSchedule(
                    activeTab as ScheduleKey,
                    "enabled",
                    !localSchedule[activeTab as ScheduleKey].enabled
                  )
                }
                className="flex items-center gap-1.5"
              >
                {localSchedule[activeTab as ScheduleKey].enabled ? (
                  <ToggleRight size={28} style={{ color: activeTabConfig.color }} />
                ) : (
                  <ToggleLeft size={28} style={{ color: "#4b5563" }} />
                )}
                <span
                  style={{
                    fontSize: 12,
                    color: localSchedule[activeTab as ScheduleKey].enabled
                      ? activeTabConfig.color
                      : "#6b7280",
                  }}
                >
                  {localSchedule[activeTab as ScheduleKey].enabled ? "Aktywny" : "Wyłączony"}
                </span>
              </button>
            </div>

            {localSchedule[activeTab as ScheduleKey].enabled && (
              <>
                <div
                  className="rounded-xl p-3"
                  style={{
                    background: "rgba(255,255,255,0.04)",
                    border: "1px solid rgba(255,255,255,0.06)",
                  }}
                >
                  <div
                    className="flex items-center justify-center gap-3"
                    style={{ marginBottom: 4 }}
                  >
                    <span style={{ fontSize: 13, color: "#64748b" }}>Aktywny od</span>
                    <span
                      style={{
                        fontSize: 28,
                        color: activeTabConfig.color,
                        letterSpacing: "-0.02em",
                        fontVariantNumeric: "tabular-nums",
                      }}
                    >
                      {localSchedule[activeTab as ScheduleKey].start}
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
                      {localSchedule[activeTab as ScheduleKey].end}
                    </span>
                  </div>
                </div>

                <div className="grid grid-cols-2 gap-3">
                  <TimeInput
                    label="Godzina startu"
                    value={localSchedule[activeTab as ScheduleKey].start}
                    onChange={(v) => updateDeviceSchedule(activeTab as ScheduleKey, "start", v)}
                  />
                  <TimeInput
                    label="Godzina końca"
                    value={localSchedule[activeTab as ScheduleKey].end}
                    onChange={(v) => updateDeviceSchedule(activeTab as ScheduleKey, "end", v)}
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
                    Czas aktywności:{" "}
                    {(() => {
                      const [sh, sm] = localSchedule[activeTab as ScheduleKey].start.split(":").map(Number);
                      const [eh, em] = localSchedule[activeTab as ScheduleKey].end.split(":").map(Number);
                      const mins = (eh * 60 + em) - (sh * 60 + sm);
                      if (mins <= 0) return "—";
                      const h = Math.floor(mins / 60);
                      const m = mins % 60;
                      return h > 0 ? `${h}h ${m}min` : `${m}min`;
                    })()}
                  </span>
                </div>
              </>
            )}
          </div>
        ) : (
          /* Feeding tab */
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
                <p style={{ fontSize: 11, color: "#64748b" }}>Harmonogram karmnika</p>
              </div>
            </div>

            {/* Mode */}
            <div className="flex flex-col gap-2">
              <span style={{ fontSize: 12, color: "#94a3b8" }}>Tryb karmienia</span>
              <div className="flex gap-2">
                {[
                  { mode: 0, label: "Wyłączone" },
                  { mode: 1, label: "Codziennie" },
                ].map(({ mode, label }) => (
                  <button
                    key={mode}
                    onClick={() =>
                      setLocalSchedule((prev) => ({
                        ...prev,
                        feeding: { ...prev.feeding, mode },
                      }))
                    }
                    className="flex-1 py-2.5 rounded-xl transition-all"
                    style={{
                      background:
                        localSchedule.feeding.mode === mode
                          ? "rgba(244,114,182,0.2)"
                          : "rgba(255,255,255,0.04)",
                      border: `1px solid ${
                        localSchedule.feeding.mode === mode
                          ? "rgba(244,114,182,0.4)"
                          : "rgba(255,255,255,0.06)"
                      }`,
                      color:
                        localSchedule.feeding.mode === mode ? "#f472b6" : "#6b7280",
                      fontSize: 13,
                    }}
                  >
                    {label}
                  </button>
                ))}
              </div>
            </div>

            {localSchedule.feeding.mode !== 0 && (
              <TimeInput
                label="Godzina karmienia"
                value={localSchedule.feeding.time}
                onChange={(v) =>
                  setLocalSchedule((prev) => ({
                    ...prev,
                    feeding: { ...prev.feeding, time: v },
                  }))
                }
              />
            )}

            <div
              className="rounded-xl px-3 py-2.5"
              style={{
                background: "rgba(244,114,182,0.08)",
                border: "1px solid rgba(244,114,182,0.15)",
              }}
            >
              <p style={{ fontSize: 12, color: "#94a3b8" }}>
                Karmnik ma timeout bezpieczeństwa (15s) oraz czujnik potwierdzenia zakończenia cyklu.
              </p>
            </div>
          </div>
        )}

        {/* Save button */}
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
          <span style={{ fontSize: 15 }}>{saved ? "Zapisano!" : "Zapisz harmonogram"}</span>
        </button>

        {/* All schedules overview */}
        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.06)",
          }}
        >
          <div className="px-4 py-3" style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}>
            <span style={{ fontSize: 12, color: "#94a3b8" }}>Przegląd harmonogramów</span>
          </div>
          {[
            {
              label: "Światło",
              icon: Lightbulb,
              color: "#fbbf24",
              value: localSchedule.light.enabled
                ? `${localSchedule.light.start} – ${localSchedule.light.end}`
                : "Wyłączone",
              active: localSchedule.light.enabled,
            },
            {
              label: "Filtr",
              icon: Filter,
              color: "#60a5fa",
              value: localSchedule.filter.enabled
                ? `${localSchedule.filter.start} – ${localSchedule.filter.end}`
                : "Wyłączony",
              active: localSchedule.filter.enabled,
            },
            {
              label: "Napowietrzanie",
              icon: Wind,
              color: "#34d399",
              value: localSchedule.aeration.enabled
                ? `${localSchedule.aeration.start} – ${localSchedule.aeration.end}`
                : "Wyłączone",
              active: localSchedule.aeration.enabled,
            },
            {
              label: "Karmienie",
              icon: Fish,
              color: "#f472b6",
              value:
                localSchedule.feeding.mode === 0
                  ? "Wyłączone"
                  : `Codziennie ${localSchedule.feeding.time}`,
              active: localSchedule.feeding.mode !== 0,
            },
          ].map(({ label, icon: Icon, color, value, active }) => (
            <div
              key={label}
              className="flex items-center gap-3 px-4 py-3"
              style={{ borderTop: "1px solid rgba(255,255,255,0.04)" }}
            >
              <div
                className="rounded-lg p-1.5"
                style={{ background: `${color}15` }}
              >
                <Icon size={14} style={{ color: active ? color : "#4b5563" }} />
              </div>
              <span style={{ fontSize: 13, color: "#cbd5e1", flex: 1 }}>{label}</span>
              <span style={{ fontSize: 12, color: active ? "#94a3b8" : "#4b5563" }}>{value}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
