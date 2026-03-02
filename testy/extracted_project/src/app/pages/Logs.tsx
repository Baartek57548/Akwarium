import { useState } from "react";
import {
  ScrollText,
  AlertTriangle,
  Info,
  XCircle,
  Trash2,
  CheckCircle2,
  ChevronDown,
} from "lucide-react";
import { useDevice, LogEntry } from "../deviceContext";

function LevelBadge({ level }: { level: LogEntry["level"] }) {
  const config = {
    INFO: { color: "#22d3ee", bg: "rgba(34,211,238,0.12)", border: "rgba(34,211,238,0.25)" },
    WARN: { color: "#fbbf24", bg: "rgba(251,191,36,0.12)", border: "rgba(251,191,36,0.25)" },
    ERROR: { color: "#f87171", bg: "rgba(248,113,113,0.12)", border: "rgba(248,113,113,0.25)" },
  }[level];

  const Icon = level === "INFO" ? Info : level === "WARN" ? AlertTriangle : XCircle;

  return (
    <div
      className="flex items-center gap-1 rounded-lg px-1.5 py-0.5 shrink-0"
      style={{ background: config.bg, border: `1px solid ${config.border}` }}
    >
      <Icon size={10} style={{ color: config.color }} />
      <span style={{ fontSize: 9, color: config.color, letterSpacing: "0.04em" }}>{level}</span>
    </div>
  );
}

function LogItem({ entry }: { entry: LogEntry }) {
  return (
    <div
      className="flex flex-col gap-1.5 px-4 py-3"
      style={{ borderBottom: "1px solid rgba(255,255,255,0.04)" }}
    >
      <div className="flex items-center gap-2">
        <LevelBadge level={entry.level} />
        <span style={{ fontSize: 10, color: "#475569", fontVariantNumeric: "tabular-nums" }}>
          {entry.timestamp}
        </span>
      </div>
      <p style={{ fontSize: 12, color: "#cbd5e1", lineHeight: 1.5 }}>{entry.message}</p>
    </div>
  );
}

export function Logs() {
  const { state, clearCriticalLogs } = useDevice();
  const [activeTab, setActiveTab] = useState<"normal" | "critical">("normal");
  const [cleared, setCleared] = useState(false);

  const handleClear = () => {
    clearCriticalLogs();
    setCleared(true);
    setTimeout(() => setCleared(false), 2000);
  };

  const logs = activeTab === "normal" ? state.logs : state.criticalLogs;

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
      {/* Header */}
      <div
        className="px-5 pt-12 pb-5"
        style={{
          background: "linear-gradient(160deg, #0d1527 0%, #080e1c 100%)",
          borderBottom: "1px solid rgba(6,182,212,0.1)",
        }}
      >
        <p style={{ fontSize: 11, color: "#64748b", letterSpacing: "0.12em" }} className="uppercase mb-1">
          Sterownik Akwarium PRO
        </p>
        <h1 style={{ fontSize: 22, color: "#e2e8f0" }}>Logi systemowe</h1>
        <p style={{ fontSize: 12, color: "#64748b", marginTop: 2 }}>
          Ring buffer: 20 wpisów (RAM + Preferences)
        </p>
      </div>

      {/* Tab selector */}
      <div className="px-4 pt-4 pb-3">
        <div
          className="flex rounded-2xl p-1 gap-1"
          style={{ background: "rgba(255,255,255,0.04)", border: "1px solid rgba(255,255,255,0.06)" }}
        >
          <button
            onClick={() => setActiveTab("normal")}
            className="flex-1 flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all"
            style={{
              background: activeTab === "normal" ? "rgba(34,211,238,0.12)" : "transparent",
              border: `1px solid ${activeTab === "normal" ? "rgba(34,211,238,0.3)" : "transparent"}`,
            }}
          >
            <ScrollText size={14} style={{ color: activeTab === "normal" ? "#22d3ee" : "#6b7280" }} />
            <span style={{ fontSize: 13, color: activeTab === "normal" ? "#e2e8f0" : "#6b7280" }}>
              Logi bieżące
            </span>
            <span
              className="rounded-full px-1.5"
              style={{
                fontSize: 10,
                background: activeTab === "normal" ? "rgba(34,211,238,0.2)" : "rgba(255,255,255,0.08)",
                color: activeTab === "normal" ? "#22d3ee" : "#64748b",
              }}
            >
              {state.logs.length}
            </span>
          </button>
          <button
            onClick={() => setActiveTab("critical")}
            className="flex-1 flex items-center justify-center gap-2 rounded-xl py-2.5 transition-all"
            style={{
              background: activeTab === "critical" ? "rgba(248,113,113,0.12)" : "transparent",
              border: `1px solid ${activeTab === "critical" ? "rgba(248,113,113,0.3)" : "transparent"}`,
            }}
          >
            <AlertTriangle size={14} style={{ color: activeTab === "critical" ? "#f87171" : "#6b7280" }} />
            <span style={{ fontSize: 13, color: activeTab === "critical" ? "#e2e8f0" : "#6b7280" }}>
              Krytyczne
            </span>
            {state.criticalLogs.length > 0 && (
              <span
                className="rounded-full px-1.5"
                style={{
                  fontSize: 10,
                  background: activeTab === "critical" ? "rgba(248,113,113,0.2)" : "rgba(248,113,113,0.15)",
                  color: "#f87171",
                }}
              >
                {state.criticalLogs.length}
              </span>
            )}
          </button>
        </div>
      </div>

      {/* Log list */}
      <div
        className="mx-4 rounded-2xl overflow-hidden mb-4"
        style={{
          background: "rgba(255,255,255,0.02)",
          border: "1px solid rgba(255,255,255,0.06)",
        }}
      >
        {/* Header */}
        <div
          className="flex items-center justify-between px-4 py-3"
          style={{ borderBottom: "1px solid rgba(255,255,255,0.06)" }}
        >
          <span style={{ fontSize: 12, color: "#64748b" }}>
            {activeTab === "normal" ? "Ostatnie zdarzenia" : "Logi krytyczne (trwałe)"} · {logs.length} wpisów
          </span>
          {activeTab === "critical" && logs.length > 0 && (
            <button
              onClick={handleClear}
              className="flex items-center gap-1.5 rounded-lg px-2.5 py-1 transition-all active:scale-95"
              style={{
                background: cleared ? "rgba(74,222,128,0.12)" : "rgba(248,113,113,0.1)",
                border: `1px solid ${cleared ? "rgba(74,222,128,0.3)" : "rgba(248,113,113,0.25)"}`,
                color: cleared ? "#4ade80" : "#f87171",
                fontSize: 12,
              }}
            >
              {cleared ? <CheckCircle2 size={12} /> : <Trash2 size={12} />}
              {cleared ? "Wyczyszczono" : "Wyczyść"}
            </button>
          )}
        </div>

        {logs.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-12 gap-3">
            <CheckCircle2 size={32} style={{ color: "#4ade80", opacity: 0.5 }} />
            <p style={{ fontSize: 13, color: "#475569" }}>Brak logów krytycznych</p>
          </div>
        ) : (
          <div>
            {logs.map((entry) => (
              <LogItem key={entry.id} entry={entry} />
            ))}
          </div>
        )}
      </div>

      {/* Stats */}
      {activeTab === "normal" && (
        <div className="px-4 pb-4">
          <div className="grid grid-cols-3 gap-3">
            {[
              {
                label: "INFO",
                count: state.logs.filter((l) => l.level === "INFO").length,
                color: "#22d3ee",
                bg: "rgba(34,211,238,0.08)",
              },
              {
                label: "WARN",
                count: state.logs.filter((l) => l.level === "WARN").length,
                color: "#fbbf24",
                bg: "rgba(251,191,36,0.08)",
              },
              {
                label: "ERROR",
                count: state.logs.filter((l) => l.level === "ERROR").length,
                color: "#f87171",
                bg: "rgba(248,113,113,0.08)",
              },
            ].map(({ label, count, color, bg }) => (
              <div
                key={label}
                className="rounded-2xl p-3 flex flex-col items-center gap-1"
                style={{ background: bg, border: `1px solid ${color}20` }}
              >
                <span style={{ fontSize: 22, color, letterSpacing: "-0.02em" }}>{count}</span>
                <span style={{ fontSize: 11, color: "#64748b" }}>{label}</span>
              </div>
            ))}
          </div>
        </div>
      )}

      {activeTab === "critical" && (
        <div className="px-4 pb-4">
          <div
            className="rounded-2xl px-4 py-3"
            style={{
              background: "rgba(248,113,113,0.06)",
              border: "1px solid rgba(248,113,113,0.15)",
            }}
          >
            <p style={{ fontSize: 12, color: "#94a3b8" }}>
              Logi krytyczne są przechowywane trwale w Preferences (Flash) i przeżywają restart.
              Można je wyczyścić przez panel lub API: <span style={{ color: "#f87171" }}>POST /api/action</span>{" "}
              z <span style={{ color: "#fbbf24" }}>action=clear_critical_logs</span>
            </p>
          </div>
        </div>
      )}
    </div>
  );
}
