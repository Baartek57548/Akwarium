import { useState } from "react";
import {
  Fish,
  Clock,
  CheckCircle2,
  AlertCircle,
  Zap,
  Timer,
  Activity,
  ChevronDown,
  ChevronUp,
} from "lucide-react";
import { useDevice } from "../deviceContext";
import { motion } from "motion/react";

export function Feeder() {
  const { state, feedNow } = useDevice();
  const [feeding, setFeeding] = useState<"idle" | "running" | "done">("idle");
  const [progress, setProgress] = useState(0);

  const handleFeed = () => {
    if (feeding !== "idle") return;
    setFeeding("running");
    setProgress(0);
    feedNow();

    let p = 0;
    const interval = setInterval(() => {
      p += 7;
      setProgress(Math.min(p, 100));
      if (p >= 100) {
        clearInterval(interval);
        setFeeding("done");
        setTimeout(() => {
          setFeeding("idle");
          setProgress(0);
        }, 3000);
      }
    }, 100);
  };

  const feedMode = state.schedule.feeding.mode === 0 ? "Wyłączone" : "Codziennie";
  const feedTime = state.schedule.feeding.time;

  return (
    <div style={{ color: "#e2e8f0", minHeight: "100%" }}>
      {/* Header */}
      <div
        className="px-5 pt-12 pb-5"
        style={{
          background: "linear-gradient(160deg, #130d1f 0%, #080e1c 100%)",
          borderBottom: "1px solid rgba(244,114,182,0.12)",
        }}
      >
        <p style={{ fontSize: 11, color: "#64748b", letterSpacing: "0.12em" }} className="uppercase mb-1">
          Sterownik Akwarium PRO
        </p>
        <h1 style={{ fontSize: 22, color: "#e2e8f0" }}>Karmnik</h1>
        <p style={{ fontSize: 12, color: "#64748b", marginTop: 2 }}>
          Sterowanie ręczne i harmonogram
        </p>
      </div>

      <div className="flex flex-col gap-4 px-4 pt-4 pb-4">
        {/* Feed NOW button */}
        <div
          className="rounded-2xl p-6 flex flex-col items-center gap-4"
          style={{
            background: "linear-gradient(135deg, rgba(244,114,182,0.1) 0%, rgba(8,14,28,0.9) 100%)",
            border: "1px solid rgba(244,114,182,0.2)",
          }}
        >
          <p style={{ fontSize: 12, color: "#94a3b8" }}>Karmienie ręczne</p>

          <div className="relative flex items-center justify-center" style={{ width: 140, height: 140 }}>
            {/* Outer glow ring */}
            <div
              className="absolute inset-0 rounded-full"
              style={{
                background:
                  feeding === "running"
                    ? "conic-gradient(from 0deg, #f472b6 0%, transparent 100%)"
                    : feeding === "done"
                    ? "conic-gradient(from 0deg, #4ade80 100%, transparent 100%)"
                    : "transparent",
                padding: 3,
                borderRadius: "50%",
                transition: "all 0.3s",
              }}
            />
            <div
              className="absolute rounded-full"
              style={{
                inset: 3,
                background: "#080e1c",
                borderRadius: "50%",
              }}
            />

            {/* Progress ring SVG */}
            {feeding === "running" && (
              <svg
                className="absolute inset-0"
                style={{ width: 140, height: 140, transform: "rotate(-90deg)" }}
                viewBox="0 0 140 140"
              >
                <circle
                  cx="70"
                  cy="70"
                  r="64"
                  fill="none"
                  stroke="rgba(244,114,182,0.15)"
                  strokeWidth="6"
                />
                <circle
                  cx="70"
                  cy="70"
                  r="64"
                  fill="none"
                  stroke="#f472b6"
                  strokeWidth="6"
                  strokeLinecap="round"
                  strokeDasharray={`${2 * Math.PI * 64}`}
                  strokeDashoffset={`${2 * Math.PI * 64 * (1 - progress / 100)}`}
                  style={{ transition: "stroke-dashoffset 0.1s linear" }}
                />
              </svg>
            )}
            {feeding === "done" && (
              <svg
                className="absolute inset-0"
                style={{ width: 140, height: 140, transform: "rotate(-90deg)" }}
                viewBox="0 0 140 140"
              >
                <circle cx="70" cy="70" r="64" fill="none" stroke="#4ade80" strokeWidth="6" />
              </svg>
            )}

            <button
              onClick={handleFeed}
              disabled={feeding !== "idle"}
              className="relative z-10 flex flex-col items-center justify-center rounded-full transition-all"
              style={{
                width: 110,
                height: 110,
                background:
                  feeding === "done"
                    ? "rgba(74,222,128,0.15)"
                    : feeding === "running"
                    ? "rgba(244,114,182,0.15)"
                    : "rgba(244,114,182,0.1)",
                border: `2px solid ${
                  feeding === "done"
                    ? "#4ade80"
                    : feeding === "running"
                    ? "#f472b6"
                    : "rgba(244,114,182,0.4)"
                }`,
                boxShadow:
                  feeding === "done"
                    ? "0 0 30px rgba(74,222,128,0.3)"
                    : feeding === "running"
                    ? "0 0 30px rgba(244,114,182,0.3)"
                    : "0 0 20px rgba(244,114,182,0.1)",
                cursor: feeding !== "idle" ? "default" : "pointer",
              }}
            >
              {feeding === "done" ? (
                <>
                  <CheckCircle2 size={30} style={{ color: "#4ade80" }} />
                  <span style={{ fontSize: 11, color: "#4ade80", marginTop: 6 }}>Gotowe!</span>
                </>
              ) : feeding === "running" ? (
                <>
                  <Activity size={30} style={{ color: "#f472b6" }} />
                  <span style={{ fontSize: 11, color: "#f472b6", marginTop: 6 }}>
                    {Math.round(progress)}%
                  </span>
                </>
              ) : (
                <>
                  <Fish size={30} style={{ color: "#f472b6" }} />
                  <span style={{ fontSize: 11, color: "#f472b6", marginTop: 6 }}>Karm teraz</span>
                </>
              )}
            </button>
          </div>

          <p style={{ fontSize: 12, color: "#64748b", textAlign: "center" }}>
            {feeding === "running"
              ? "Karmnik w trakcie pracy… timeout bezpieczeństwa: 15s"
              : feeding === "done"
              ? "Karmienie zakończone pomyślnie"
              : "Naciśnij, aby uruchomić karmnik jednorazowo"}
          </p>
        </div>

        {/* Last feed */}
        <div
          className="flex items-center gap-3 rounded-2xl px-4 py-3.5"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="rounded-xl p-2" style={{ background: "rgba(244,114,182,0.12)" }}>
            <Clock size={16} style={{ color: "#f472b6" }} />
          </div>
          <div className="flex-1">
            <p style={{ fontSize: 11, color: "#64748b" }}>Ostatnie karmienie</p>
            <p style={{ fontSize: 14, color: "#e2e8f0" }}>{state.lastFeedTime}</p>
          </div>
        </div>

        {/* Schedule info */}
        <div
          className="rounded-2xl p-4 flex flex-col gap-3"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.07)",
          }}
        >
          <div className="flex items-center gap-2 mb-1">
            <Timer size={14} style={{ color: "#94a3b8" }} />
            <span style={{ fontSize: 13, color: "#94a3b8" }}>Harmonogram karmienia</span>
          </div>

          <div className="grid grid-cols-2 gap-3">
            <div
              className="rounded-xl p-3"
              style={{
                background: "rgba(244,114,182,0.08)",
                border: "1px solid rgba(244,114,182,0.15)",
              }}
            >
              <p style={{ fontSize: 10, color: "#64748b" }} className="uppercase mb-1">Tryb</p>
              <p style={{ fontSize: 14, color: "#f472b6" }}>{feedMode}</p>
            </div>
            <div
              className="rounded-xl p-3"
              style={{
                background: "rgba(244,114,182,0.08)",
                border: "1px solid rgba(244,114,182,0.15)",
              }}
            >
              <p style={{ fontSize: 10, color: "#64748b" }} className="uppercase mb-1">Godzina</p>
              <p style={{ fontSize: 14, color: "#f472b6" }}>
                {state.schedule.feeding.mode === 0 ? "—" : feedTime}
              </p>
            </div>
          </div>
        </div>

        {/* Safety info */}
        <div
          className="rounded-2xl p-4 flex flex-col gap-2.5"
          style={{
            background: "rgba(251,191,36,0.06)",
            border: "1px solid rgba(251,191,36,0.15)",
          }}
        >
          <div className="flex items-center gap-2">
            <AlertCircle size={15} style={{ color: "#fbbf24" }} />
            <span style={{ fontSize: 13, color: "#fbbf24" }}>Parametry karmnika</span>
          </div>
          <ul className="flex flex-col gap-1.5" style={{ paddingLeft: 4 }}>
            {[
              "Timeout bezpieczeństwa: 15 sekund",
              "Czujnik potwierdzenia zakończenia cyklu",
              "Logika wykrywania zacięcia",
              "Silnik: GPIO5",
              "Czujnik: GPIO12",
            ].map((item) => (
              <li key={item} className="flex items-start gap-2">
                <div
                  style={{
                    width: 4,
                    height: 4,
                    background: "#fbbf24",
                    borderRadius: "50%",
                    marginTop: 6,
                    flexShrink: 0,
                  }}
                />
                <span style={{ fontSize: 12, color: "#94a3b8" }}>{item}</span>
              </li>
            ))}
          </ul>
        </div>
      </div>
    </div>
  );
}
