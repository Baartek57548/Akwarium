import { useState, useEffect } from "react";
import {
  Thermometer,
  Zap,
  Droplets,
  Wind,
  Lightbulb,
  Filter,
  Fish,
  Flame,
  Wifi,
  WifiOff,
  Battery,
  BatteryLow,
  BatteryMedium,
  BatteryFull,
  Clock,
  ChevronRight,
  Activity,
} from "lucide-react";
import {
  AreaChart,
  Area,
  XAxis,
  YAxis,
  Tooltip,
  ResponsiveContainer,
} from "recharts";
import { useDevice } from "../deviceContext";
import { tempHistory } from "../tempHistory";
import { Link } from "react-router";

function BatteryIcon({ pct }: { pct: number }) {
  if (pct > 70) return <BatteryFull size={16} className="text-green-400" />;
  if (pct > 35) return <BatteryMedium size={16} className="text-amber-400" />;
  return <BatteryLow size={16} className="text-red-400" />;
}

function RelayCard({
  label,
  icon: Icon,
  active,
  color,
  onToggle,
}: {
  label: string;
  icon: React.ElementType;
  active: boolean;
  color: string;
  onToggle: () => void;
}) {
  return (
    <button
      onClick={onToggle}
      className="relative flex flex-col items-center justify-center gap-2 rounded-2xl p-4 transition-all active:scale-95"
      style={{
        background: active
          ? `linear-gradient(135deg, ${color}22 0%, ${color}11 100%)`
          : "rgba(255,255,255,0.04)",
        border: `1px solid ${active ? color + "55" : "rgba(255,255,255,0.06)"}`,
        boxShadow: active ? `0 0 18px ${color}22` : "none",
      }}
    >
      <div
        className="rounded-full p-2.5"
        style={{
          background: active ? `${color}22` : "rgba(255,255,255,0.06)",
        }}
      >
        <Icon size={22} style={{ color: active ? color : "#4b5563" }} />
      </div>
      <span style={{ fontSize: 12, color: active ? "#e2e8f0" : "#6b7280" }} className="font-medium">
        {label}
      </span>
      <div
        className="rounded-full"
        style={{
          width: 6,
          height: 6,
          background: active ? color : "#374151",
          boxShadow: active ? `0 0 8px ${color}` : "none",
        }}
      />
    </button>
  );
}

const CustomTooltip = ({ active, payload }: any) => {
  if (active && payload && payload.length) {
    return (
      <div
        className="rounded-lg px-3 py-1.5 text-xs"
        style={{ background: "#1e2a3d", border: "1px solid rgba(6,182,212,0.3)", color: "#e2e8f0" }}
      >
        <span className="text-cyan-400">{payload[0].value.toFixed(1)}°C</span>
      </div>
    );
  }
  return null;
};

export function Dashboard() {
  const { state, toggleLight, toggleFilter, toggleHeater, setAeration, feedNow } = useDevice();
  const [time, setTime] = useState(new Date());
  const [feedFeedback, setFeedFeedback] = useState(false);

  useEffect(() => {
    const t = setInterval(() => setTime(new Date()), 1000);
    return () => clearInterval(t);
  }, []);

  const handleFeed = () => {
    feedNow();
    setFeedFeedback(true);
    setTimeout(() => setFeedFeedback(false), 3000);
  };

  const chartData = tempHistory.filter((_, i) => i % 2 === 0);
  const minTemp = Math.min(...chartData.map((d) => d.temp)) - 0.5;
  const maxTemp = Math.max(...chartData.map((d) => d.temp)) + 0.5;

  const tempColor =
    state.currentTemp > 27
      ? "#ef4444"
      : state.currentTemp < 23
      ? "#60a5fa"
      : "#22d3ee";

  const batColor =
    state.batteryPercent > 70
      ? "#4ade80"
      : state.batteryPercent > 35
      ? "#fbbf24"
      : "#f87171";

  const nextEvents = [
    { label: "Koniec światła", time: state.schedule.light.end, icon: Lightbulb, color: "#fbbf24" },
    { label: "Karmienie", time: state.schedule.feeding.time, icon: Fish, color: "#34d399" },
    { label: "Koniec filtra", time: state.schedule.filter.end, icon: Filter, color: "#60a5fa" },
  ];

  return (
    <div className="flex flex-col" style={{ minHeight: "100%", color: "#e2e8f0" }}>
      {/* Header */}
      <div
        className="relative overflow-hidden px-5 pt-12 pb-6"
        style={{
          background: "linear-gradient(160deg, #0c1a33 0%, #081020 100%)",
          borderBottom: "1px solid rgba(6,182,212,0.12)",
        }}
      >
        <div className="absolute inset-0 opacity-10"
          style={{
            backgroundImage: `url(https://images.unsplash.com/photo-1631300692439-42465e4949f4?crop=entropy&cs=tinysrgb&fit=max&fm=jpg&w=800)`,
            backgroundSize: "cover",
            backgroundPosition: "center",
          }}
        />
        <div className="relative">
          <div className="flex items-center justify-between mb-1">
            <div>
              <p style={{ fontSize: 11, color: "#64748b", letterSpacing: "0.12em" }} className="uppercase mb-0.5">
                Sterownik Akwarium PRO
              </p>
              <h1 style={{ fontSize: 22, color: "#e2e8f0" }}>Moje Akwarium</h1>
            </div>
            <div className="flex flex-col items-end gap-1">
              <div className="flex items-center gap-1.5">
                {state.wifi.connected ? (
                  <Wifi size={14} className="text-cyan-400" />
                ) : (
                  <WifiOff size={14} className="text-slate-500" />
                )}
                <span style={{ fontSize: 11, color: state.wifi.connected ? "#22d3ee" : "#6b7280" }}>
                  {state.wifi.connected ? state.wifi.ssid : "Offline"}
                </span>
              </div>
              <div className="flex items-center gap-1.5">
                <BatteryIcon pct={state.batteryPercent} />
                <span style={{ fontSize: 11, color: batColor }}>
                  {state.batteryPercent}% ({state.batteryVoltage.toFixed(2)}V)
                </span>
              </div>
            </div>
          </div>

          <div className="flex items-center gap-1.5 mt-2">
            <Clock size={12} style={{ color: "#64748b" }} />
            <span style={{ fontSize: 13, color: "#94a3b8" }}>
              {time.toLocaleTimeString("pl-PL", { hour: "2-digit", minute: "2-digit", second: "2-digit" })}
              {" · "}
              {time.toLocaleDateString("pl-PL", { weekday: "short", day: "numeric", month: "short" })}
            </span>
          </div>
        </div>
      </div>

      <div className="flex flex-col gap-4 px-4 pt-4 pb-2">
        {/* Temperature + Feed */}
        <div className="flex gap-3">
          {/* Temp card */}
          <div
            className="flex-1 rounded-2xl p-4"
            style={{
              background: "linear-gradient(135deg, rgba(6,182,212,0.12) 0%, rgba(8,14,28,0.8) 100%)",
              border: "1px solid rgba(6,182,212,0.25)",
            }}
          >
            <div className="flex items-center gap-1.5 mb-2">
              <Thermometer size={14} style={{ color: "#22d3ee" }} />
              <span style={{ fontSize: 11, color: "#64748b" }}>Temperatura</span>
            </div>
            <div className="flex items-end gap-1.5">
              <span style={{ fontSize: 36, color: tempColor, letterSpacing: "-0.02em" }}>
                {state.currentTemp.toFixed(1)}
              </span>
              <span style={{ fontSize: 18, color: "#64748b", paddingBottom: 4 }}>°C</span>
            </div>
            <div className="flex items-center gap-1.5 mt-1">
              <div
                className="rounded-full px-2 py-0.5"
                style={{
                  fontSize: 10,
                  background: "rgba(34,211,238,0.1)",
                  color: "#22d3ee",
                  border: "1px solid rgba(34,211,238,0.2)",
                }}
              >
                cel: {state.targetTemp.toFixed(1)}°C
              </div>
            </div>
          </div>

          {/* Feed button */}
          <div
            className="flex flex-col items-center justify-center rounded-2xl p-4 gap-2"
            style={{
              minWidth: 100,
              background: feedFeedback
                ? "linear-gradient(135deg, rgba(52,211,153,0.2) 0%, rgba(8,14,28,0.8) 100%)"
                : "linear-gradient(135deg, rgba(56,189,248,0.1) 0%, rgba(8,14,28,0.8) 100%)",
              border: `1px solid ${feedFeedback ? "rgba(52,211,153,0.4)" : "rgba(56,189,248,0.2)"}`,
              transition: "all 0.3s",
            }}
          >
            <button
              onClick={handleFeed}
              className="rounded-full flex items-center justify-center transition-all active:scale-90"
              style={{
                width: 52,
                height: 52,
                background: feedFeedback ? "rgba(52,211,153,0.25)" : "rgba(56,189,248,0.15)",
                border: `2px solid ${feedFeedback ? "#34d399" : "#38bdf8"}`,
                boxShadow: feedFeedback ? "0 0 20px rgba(52,211,153,0.4)" : "0 0 12px rgba(56,189,248,0.2)",
              }}
            >
              <Fish size={22} style={{ color: feedFeedback ? "#34d399" : "#38bdf8" }} />
            </button>
            <span style={{ fontSize: 11, color: feedFeedback ? "#34d399" : "#94a3b8" }}>
              {feedFeedback ? "Karmię..." : "Karm teraz"}
            </span>
          </div>
        </div>

        {/* Relay grid */}
        <div>
          <div className="flex items-center justify-between mb-2.5">
            <span style={{ fontSize: 12, color: "#64748b" }}>Przekaźniki</span>
            <span style={{ fontSize: 11, color: "#475569" }}>
              {[state.lightOn, state.filterOn, state.heaterOn, state.aerationAngle > 0].filter(Boolean).length} / 4 aktywnych
            </span>
          </div>
          <div className="grid grid-cols-4 gap-2.5">
            <RelayCard
              label="Światło"
              icon={Lightbulb}
              active={state.lightOn}
              color="#fbbf24"
              onToggle={toggleLight}
            />
            <RelayCard
              label="Filtr"
              icon={Filter}
              active={state.filterOn}
              color="#60a5fa"
              onToggle={toggleFilter}
            />
            <RelayCard
              label="Grzałka"
              icon={Flame}
              active={state.heaterOn}
              color="#fb923c"
              onToggle={toggleHeater}
            />
            <RelayCard
              label="Powietrze"
              icon={Wind}
              active={state.aerationAngle > 0}
              color="#34d399"
              onToggle={() => setAeration(state.aerationAngle > 0 ? 0 : 45)}
            />
          </div>
        </div>

        {/* Temperature chart */}
        <div
          className="rounded-2xl p-4"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.06)",
          }}
        >
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center gap-2">
              <Activity size={14} className="text-cyan-400" />
              <span style={{ fontSize: 12, color: "#94a3b8" }}>Temperatura (24h)</span>
            </div>
            <span style={{ fontSize: 11, color: "#475569" }}>
              min {Math.min(...chartData.map((d) => d.temp)).toFixed(1)}°C · max {Math.max(...chartData.map((d) => d.temp)).toFixed(1)}°C
            </span>
          </div>
          <ResponsiveContainer width="100%" height={100}>
            <AreaChart data={chartData} margin={{ top: 2, right: 2, bottom: 0, left: -20 }}>
              <defs>
                <linearGradient id="tempGrad" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="#22d3ee" stopOpacity={0.3} />
                  <stop offset="95%" stopColor="#22d3ee" stopOpacity={0} />
                </linearGradient>
              </defs>
              <XAxis
                dataKey="time"
                tick={{ fill: "#475569", fontSize: 9 }}
                tickLine={false}
                axisLine={false}
                interval={5}
              />
              <YAxis
                domain={[minTemp, maxTemp]}
                tick={{ fill: "#475569", fontSize: 9 }}
                tickLine={false}
                axisLine={false}
                tickFormatter={(v) => `${v.toFixed(0)}°`}
              />
              <Tooltip content={<CustomTooltip />} />
              <Area
                type="monotone"
                dataKey="temp"
                stroke="#22d3ee"
                strokeWidth={1.5}
                fill="url(#tempGrad)"
                dot={false}
              />
            </AreaChart>
          </ResponsiveContainer>
        </div>

        {/* Next events */}
        <div
          className="rounded-2xl overflow-hidden"
          style={{
            background: "rgba(255,255,255,0.03)",
            border: "1px solid rgba(255,255,255,0.06)",
          }}
        >
          <div className="flex items-center justify-between px-4 pt-3 pb-2">
            <span style={{ fontSize: 12, color: "#94a3b8" }}>Najbliższe zdarzenia</span>
            <Link to="/schedule">
              <span style={{ fontSize: 11, color: "#22d3ee" }} className="flex items-center gap-0.5">
                Edytuj <ChevronRight size={12} />
              </span>
            </Link>
          </div>
          {nextEvents.map(({ label, time, icon: Icon, color }) => (
            <div
              key={label}
              className="flex items-center gap-3 px-4 py-2.5"
              style={{ borderTop: "1px solid rgba(255,255,255,0.04)" }}
            >
              <div
                className="rounded-lg p-1.5"
                style={{ background: `${color}15` }}
              >
                <Icon size={14} style={{ color }} />
              </div>
              <span style={{ fontSize: 13, color: "#cbd5e1", flex: 1 }}>{label}</span>
              <span style={{ fontSize: 13, color: "#94a3b8" }}>{time}</span>
            </div>
          ))}
        </div>

        {/* Last feed + uptime */}
        <div className="grid grid-cols-2 gap-3 pb-2">
          <div
            className="rounded-2xl p-3.5"
            style={{
              background: "rgba(255,255,255,0.03)",
              border: "1px solid rgba(255,255,255,0.06)",
            }}
          >
            <div className="flex items-center gap-1.5 mb-1.5">
              <Fish size={13} style={{ color: "#34d399" }} />
              <span style={{ fontSize: 11, color: "#64748b" }}>Ostatnie karmienie</span>
            </div>
            <p style={{ fontSize: 13, color: "#e2e8f0" }}>{state.lastFeedTime}</p>
          </div>
          <div
            className="rounded-2xl p-3.5"
            style={{
              background: "rgba(255,255,255,0.03)",
              border: "1px solid rgba(255,255,255,0.06)",
            }}
          >
            <div className="flex items-center gap-1.5 mb-1.5">
              <Zap size={13} style={{ color: "#a78bfa" }} />
              <span style={{ fontSize: 11, color: "#64748b" }}>Czas pracy</span>
            </div>
            <p style={{ fontSize: 13, color: "#e2e8f0" }}>{state.uptime}</p>
          </div>
        </div>
      </div>
    </div>
  );
}
