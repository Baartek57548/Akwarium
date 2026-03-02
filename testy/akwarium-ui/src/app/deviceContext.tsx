import React, { useCallback, useEffect, useMemo, useState } from "react";
import {
  DeviceContext,
  type DeviceContextType,
  type DeviceSchedule,
  type DeviceState,
  type LogEntry,
  type TempPoint,
} from "./deviceStore";

interface StatusResponse {
  temperature?: {
    current?: number;
    target?: number;
    hysteresis?: number;
    min?: number;
    minTimeEpoch?: number;
    heaterEnabled?: boolean;
  };
  battery?: {
    voltage?: number;
    percent?: number;
  };
  relays?: {
    light?: boolean;
    pump?: boolean;
    heater?: boolean;
  };
  servo?: {
    angle?: number;
  };
  schedule?: {
    dayStartHour?: number;
    dayStartMin?: number;
    dayEndHour?: number;
    dayEndMin?: number;
    airStartHour?: number;
    airStartMin?: number;
    airEndHour?: number;
    airEndMin?: number;
    filterStartHour?: number;
    filterStartMin?: number;
    filterEndHour?: number;
    filterEndMin?: number;
    servoPreOffMins?: number;
  };
  feeding?: {
    hour?: number;
    minute?: number;
    freq?: number;
    lastFeedEpoch?: number;
  };
  network?: {
    ip?: string;
    apMode?: boolean;
  };
  settings?: {
    alwaysScreenOn?: boolean;
  };
  manual?: {
    lightOverride?: boolean;
    lightState?: boolean;
    filterOverride?: boolean;
    filterState?: boolean;
  };
}

interface LogsResponse {
  normal?: string[];
  critical?: string[];
}

const configuredBase = (import.meta.env.VITE_API_BASE_URL as string | undefined)?.trim() ?? "";
const API_BASE = configuredBase.replace(/\/+$/, "");

const DEFAULT_SCHEDULE: DeviceSchedule = {
  light: { start: "10:00", end: "21:30" },
  aeration: { start: "10:00", end: "21:00" },
  filter: { start: "10:30", end: "20:30" },
  feeding: { time: "18:00", mode: 1 },
  servoPreOffMins: 30,
};

const initialState: DeviceState = {
  loading: true,
  online: false,
  lastError: null,
  lastSyncAt: null,

  currentTemp: 0,
  targetTemp: 25,
  hysteresis: 0.5,
  minTemp: null,
  minTempTimeEpoch: null,

  relays: {
    light: false,
    pump: false,
    heater: false,
  },
  aerationAngle: 0,
  manualServoAngle: null,

  batteryVoltage: 0,
  batteryPercent: 0,

  schedule: DEFAULT_SCHEDULE,

  wifi: {
    connected: false,
    ip: "-",
    apMode: false,
    modeLabel: "Offline",
  },

  settings: {
    alwaysScreenOn: false,
    heaterEnabled: true,
  },

  manual: {
    lightOverride: false,
    lightState: false,
    filterOverride: false,
    filterState: false,
  },

  logs: [],
  criticalLogs: [],

  lastFeedEpoch: 0,
  lastFeedTime: "Brak danych",

  tempHistory: [],
};

function apiUrl(path: string): string {
  return API_BASE ? `${API_BASE}${path}` : path;
}

function genId() {
  if (typeof crypto !== "undefined" && typeof crypto.randomUUID === "function") {
    return crypto.randomUUID();
  }
  return Math.random().toString(36).slice(2, 11);
}

function toClockTime(value: Date = new Date()) {
  return value.toLocaleTimeString("pl-PL", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}

function pad2(value: number): string {
  return String(Math.max(0, Math.floor(value))).padStart(2, "0");
}

function hhmm(hour: number | undefined, minute: number | undefined, fallback: string): string {
  if (!Number.isFinite(hour) || !Number.isFinite(minute)) {
    return fallback;
  }
  return `${pad2(hour ?? 0)}:${pad2(minute ?? 0)}`;
}

function safeNumber(value: unknown, fallback: number): number {
  return typeof value === "number" && Number.isFinite(value) ? value : fallback;
}

function inferLogLevel(line: string, fallback: LogEntry["level"]): LogEntry["level"] {
  const upper = line.toUpperCase();
  if (
    upper.includes("ERROR") ||
    upper.includes("ERR") ||
    upper.includes("CRITICAL") ||
    upper.includes("BLAD") ||
    upper.includes("BLED")
  ) {
    return "ERROR";
  }
  if (upper.includes("WARN") || upper.includes("UWAGA") || upper.includes("OSTRZEZ")) {
    return "WARN";
  }
  return fallback;
}

function extractTimestamp(line: string): string {
  const dateTimeMatch = line.match(/\b\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}\b/);
  if (dateTimeMatch) {
    return dateTimeMatch[0].replace("T", " ");
  }

  const timeMatch = line.match(/\b\d{2}:\d{2}:\d{2}\b/);
  if (timeMatch) {
    return timeMatch[0];
  }

  return "--:--:--";
}

function toLogEntries(lines: string[], fallback: LogEntry["level"]): LogEntry[] {
  return lines.map((raw, index) => {
    const message = raw.trim();
    return {
      id: `${index}-${genId()}`,
      timestamp: extractTimestamp(message),
      level: inferLogLevel(message, fallback),
      message,
    };
  });
}

function formatLastFeed(lastFeedEpoch: number): string {
  if (!Number.isFinite(lastFeedEpoch) || lastFeedEpoch <= 0) {
    return "Brak danych";
  }

  const date = new Date(lastFeedEpoch * 1000);
  return date.toLocaleString("pl-PL", {
    day: "2-digit",
    month: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function pushTempPoint(history: TempPoint[], temp: number, epochMs: number): TempPoint[] {
  if (!Number.isFinite(temp)) {
    return history;
  }

  const roundedTemp = Math.round(temp * 10) / 10;
  const point: TempPoint = {
    time: new Date(epochMs).toLocaleTimeString("pl-PL", {
      hour: "2-digit",
      minute: "2-digit",
    }),
    temp: roundedTemp,
    epochMs,
  };

  if (history.length > 0) {
    const last = history[history.length - 1];
    if (epochMs - last.epochMs < 60_000) {
      const replaced = [...history];
      replaced[replaced.length - 1] = point;
      return replaced;
    }
  }

  const next = [...history, point];
  const maxPoints = 240;
  if (next.length > maxPoints) {
    return next.slice(next.length - maxPoints);
  }
  return next;
}

function mapStatusToPatch(status: StatusResponse, prev: DeviceState) {
  const now = Date.now();

  const currentTemp = safeNumber(status.temperature?.current, prev.currentTemp);
  const targetTemp = safeNumber(status.temperature?.target, prev.targetTemp);
  const hysteresis = safeNumber(status.temperature?.hysteresis, prev.hysteresis);

  const dayStart = hhmm(
    status.schedule?.dayStartHour,
    status.schedule?.dayStartMin,
    prev.schedule.light.start,
  );
  const dayEnd = hhmm(
    status.schedule?.dayEndHour,
    status.schedule?.dayEndMin,
    prev.schedule.light.end,
  );
  const airOn = hhmm(
    status.schedule?.airStartHour,
    status.schedule?.airStartMin,
    prev.schedule.aeration.start,
  );
  const airOff = hhmm(
    status.schedule?.airEndHour,
    status.schedule?.airEndMin,
    prev.schedule.aeration.end,
  );
  const filterOn = hhmm(
    status.schedule?.filterStartHour,
    status.schedule?.filterStartMin,
    prev.schedule.filter.start,
  );
  const filterOff = hhmm(
    status.schedule?.filterEndHour,
    status.schedule?.filterEndMin,
    prev.schedule.filter.end,
  );

  const feedTime = hhmm(status.feeding?.hour, status.feeding?.minute, prev.schedule.feeding.time);
  const feedMode = safeNumber(status.feeding?.freq, prev.schedule.feeding.mode);

  const ip = status.network?.ip?.trim() || "-";
  const apMode = Boolean(status.network?.apMode);

  return {
    loading: false,
    online: true,
    lastError: null,
    lastSyncAt: now,

    currentTemp,
    targetTemp,
    hysteresis,
    minTemp: Number.isFinite(status.temperature?.min as number)
      ? (status.temperature?.min as number)
      : prev.minTemp,
    minTempTimeEpoch:
      Number.isFinite(status.temperature?.minTimeEpoch as number) &&
      (status.temperature?.minTimeEpoch as number) > 0
        ? (status.temperature?.minTimeEpoch as number)
        : prev.minTempTimeEpoch,

    relays: {
      light: Boolean(status.relays?.light),
      pump: Boolean(status.relays?.pump),
      heater: Boolean(status.relays?.heater),
    },
    aerationAngle: Math.max(0, Math.min(90, safeNumber(status.servo?.angle, prev.aerationAngle))),

    batteryVoltage: safeNumber(status.battery?.voltage, prev.batteryVoltage),
    batteryPercent: Math.max(0, Math.min(100, safeNumber(status.battery?.percent, prev.batteryPercent))),

    schedule: {
      light: { start: dayStart, end: dayEnd },
      aeration: { start: airOn, end: airOff },
      filter: { start: filterOn, end: filterOff },
      feeding: { time: feedTime, mode: Math.max(0, Math.min(3, Math.round(feedMode))) },
      servoPreOffMins: Math.max(
        0,
        Math.min(255, Math.round(safeNumber(status.schedule?.servoPreOffMins, prev.schedule.servoPreOffMins))),
      ),
    },

    wifi: {
      connected: ip !== "-" && ip !== "0.0.0.0",
      ip,
      apMode,
      modeLabel: apMode ? "Access Point" : "Station",
    },

    settings: {
      alwaysScreenOn:
        typeof status.settings?.alwaysScreenOn === "boolean"
          ? status.settings.alwaysScreenOn
          : prev.settings.alwaysScreenOn,
      heaterEnabled:
        typeof status.temperature?.heaterEnabled === "boolean"
          ? status.temperature.heaterEnabled
          : prev.settings.heaterEnabled,
    },

    manual: {
      lightOverride:
        typeof status.manual?.lightOverride === "boolean"
          ? status.manual.lightOverride
          : prev.manual.lightOverride,
      lightState:
        typeof status.manual?.lightState === "boolean"
          ? status.manual.lightState
          : prev.manual.lightState,
      filterOverride:
        typeof status.manual?.filterOverride === "boolean"
          ? status.manual.filterOverride
          : prev.manual.filterOverride,
      filterState:
        typeof status.manual?.filterState === "boolean"
          ? status.manual.filterState
          : prev.manual.filterState,
    },

    lastFeedEpoch: Math.max(0, Math.round(safeNumber(status.feeding?.lastFeedEpoch, prev.lastFeedEpoch))),
    lastFeedTime: formatLastFeed(safeNumber(status.feeding?.lastFeedEpoch, prev.lastFeedEpoch)),

    tempHistory: pushTempPoint(prev.tempHistory, currentTemp, now),
  };
}

function errorMessage(err: unknown, fallback: string): string {
  if (err instanceof Error && err.message) {
    return err.message;
  }
  return fallback;
}

function boolToArg(value: boolean): "1" | "0" {
  return value ? "1" : "0";
}

export function DeviceProvider({ children }: { children: React.ReactNode }) {
  const [state, setState] = useState<DeviceState>(initialState);

  const addLog = useCallback((level: LogEntry["level"], message: string) => {
    setState((prev) => ({
      ...prev,
      logs: [
        {
          id: genId(),
          timestamp: toClockTime(),
          level,
          message,
        },
        ...prev.logs,
      ].slice(0, 120),
    }));
  }, []);

  const fetchJson = useCallback(async <T,>(path: string): Promise<T> => {
    const response = await fetch(apiUrl(path), {
      method: "GET",
      cache: "no-store",
    });

    const text = await response.text();
    if (!response.ok) {
      throw new Error(`${path} failed (HTTP ${response.status})`);
    }

    try {
      return JSON.parse(text) as T;
    } catch {
      const preview = text.slice(0, 32).replace(/\s+/g, " ").trim();
      if (preview.startsWith("<")) {
        throw new Error(
          `API zwrocilo HTML zamiast JSON dla ${path}. Ustaw VITE_API_PROXY_TARGET na IP ESP32.`,
        );
      }
      throw new Error(`Nieprawidlowy JSON z ${path}: ${preview}`);
    }
  }, []);

  const fetchStatus = useCallback(async () => {
    const payload = await fetchJson<StatusResponse>("/api/status");

    setState((prev) => ({
      ...prev,
      ...mapStatusToPatch(payload, prev),
    }));
  }, [fetchJson]);

  const fetchLogs = useCallback(async () => {
    const payload = await fetchJson<LogsResponse>("/api/logs");
    const normalLogs = Array.isArray(payload.normal) ? payload.normal : [];
    const criticalLogs = Array.isArray(payload.critical) ? payload.critical : [];

    setState((prev) => ({
      ...prev,
      logs: toLogEntries(normalLogs, "INFO"),
      criticalLogs: toLogEntries(criticalLogs, "ERROR"),
    }));
  }, [fetchJson]);

  const refresh = useCallback(async () => {
    try {
      await fetchStatus();
    } catch (err) {
      const msg = errorMessage(err, "Brak polaczenia z /api/status");
      setState((prev) => ({
        ...prev,
        loading: false,
        online: false,
        lastError: msg,
      }));
    }

    try {
      await fetchLogs();
    } catch (err) {
      const msg = errorMessage(err, "Brak polaczenia z /api/logs");
      setState((prev) => ({ ...prev, lastError: prev.lastError ?? msg }));
    }
  }, [fetchLogs, fetchStatus]);

  const postAction = useCallback(async (params: Record<string, string | number | boolean>) => {
    const body = new URLSearchParams();
    for (const [key, value] of Object.entries(params)) {
      body.set(key, String(value));
    }

    const response = await fetch(apiUrl("/api/action"), {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: body.toString(),
    });

    const text = (await response.text()).trim();
    if (!response.ok) {
      throw new Error(text || `Action failed (HTTP ${response.status})`);
    }

    return text || "OK";
  }, []);

  const setTemp = useCallback(
    async (target: number, hyst: number) => {
      try {
        const result = await postAction({
          action: "save_schedule",
          targetTemp: target.toFixed(1),
          tempHyst: hyst.toFixed(1),
        });

        addLog("INFO", `Temperatura zapisana (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad zapisu temperatury"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setLight = useCallback(
    async (enabled: boolean) => {
      try {
        const result = await postAction({ action: "set_light", state: boolToArg(enabled) });
        addLog("INFO", `Swiatlo ustawione na ${enabled ? "ON" : "OFF"} (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad sterowania swiatlem"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setFilter = useCallback(
    async (enabled: boolean) => {
      try {
        const result = await postAction({ action: "set_filter", state: boolToArg(enabled) });
        addLog("INFO", `Filtr ustawiony na ${enabled ? "ON" : "OFF"} (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad sterowania filtrem"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setAPMode = useCallback(
    async (enabled: boolean) => {
      try {
        const result = await postAction({ action: "set_ap", state: boolToArg(enabled) });
        addLog("INFO", `AP ${enabled ? "uruchomiony" : "wylaczony"} (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad sterowania AP"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const feedNow = useCallback(async () => {
    try {
      const result = await postAction({ action: "feed_now" });
      addLog("INFO", `Karmienie uruchomione (${result})`);
      await refresh();
      return true;
    } catch (err) {
      addLog("ERROR", errorMessage(err, "Blad karmienia"));
      return false;
    }
  }, [addLog, postAction, refresh]);

  const saveSchedule = useCallback(
    async (schedule: DeviceSchedule) => {
      try {
        const result = await postAction({
          action: "save_schedule",
          dayStart: schedule.light.start,
          dayEnd: schedule.light.end,
          airOn: schedule.aeration.start,
          airOff: schedule.aeration.end,
          filterOn: schedule.filter.start,
          filterOff: schedule.filter.end,
          servoPreOffMins: schedule.servoPreOffMins,
          feedTime: schedule.feeding.time,
          feedFreq: schedule.feeding.mode,
          targetTemp: state.targetTemp.toFixed(1),
          tempHyst: state.hysteresis.toFixed(1),
          alwaysScreenOn: boolToArg(state.settings.alwaysScreenOn),
          heaterEnabled: boolToArg(state.settings.heaterEnabled),
        });

        addLog("INFO", `Harmonogram zapisany (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad zapisu harmonogramu"));
        return false;
      }
    },
    [addLog, postAction, refresh, state.hysteresis, state.settings.alwaysScreenOn, state.settings.heaterEnabled, state.targetTemp],
  );

  const clearCriticalLogs = useCallback(async () => {
    try {
      const result = await postAction({ action: "clear_critical_logs" });
      addLog("INFO", `Logi krytyczne wyczyszczone (${result})`);
      await refresh();
      return true;
    } catch (err) {
      addLog("ERROR", errorMessage(err, "Blad czyszczenia logow krytycznych"));
      return false;
    }
  }, [addLog, postAction, refresh]);

  const setAlwaysScreenOn = useCallback(
    async (enabled: boolean) => {
      try {
        const result = await postAction({
          action: "set_always_screen",
          state: boolToArg(enabled),
        });
        addLog("INFO", `alwaysScreenOn=${enabled ? "1" : "0"} (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad zapisu alwaysScreenOn"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setHeaterEnabled = useCallback(
    async (enabled: boolean) => {
      try {
        const result = await postAction({
          action: "set_heater_enabled",
          state: boolToArg(enabled),
        });
        addLog("INFO", `heaterEnabled=${enabled ? "1" : "0"} (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad zapisu heaterEnabled"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setServoPreOffMins = useCallback(
    async (val: number) => {
      const safe = Math.max(0, Math.min(255, Math.round(val)));
      try {
        const result = await postAction({
          action: "save_schedule",
          servoPreOffMins: safe,
        });
        addLog("INFO", `Servo pre-off zapisane (${result})`);
        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad zapisu servo pre-off"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const setManualServo = useCallback(
    async (angle: number | null) => {
      try {
        if (angle === null) {
          const result = await postAction({ action: "clear_servo" });
          setState((prev) => ({ ...prev, manualServoAngle: null }));
          addLog("INFO", `Manual servo override skasowany (${result})`);
        } else {
          const safe = Math.max(0, Math.min(90, Math.round(angle)));
          const result = await postAction({
            action: "set_servo",
            angle: safe,
          });
          setState((prev) => ({ ...prev, manualServoAngle: safe }));
          addLog("INFO", `Manual servo ustawiony na ${safe}deg (${result})`);
        }

        await refresh();
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad ustawiania serwa"));
        return false;
      }
    },
    [addLog, postAction, refresh],
  );

  const syncTime = useCallback(async () => {
    const epoch = Math.floor(Date.now() / 1000);

    try {
      const response = await fetch(apiUrl(`/settime?epoch=${epoch}`), {
        method: "POST",
      });

      const text = (await response.text()).trim();
      if (!response.ok) {
        throw new Error(text || `Time sync failed (HTTP ${response.status})`);
      }

      addLog("INFO", `RTC zsynchronizowany (${text || "OK"})`);
      await refresh();
      return true;
    } catch (err) {
      addLog("ERROR", errorMessage(err, "Blad synchronizacji czasu"));
      return false;
    }
  }, [addLog, refresh]);

  const uploadFirmware = useCallback(
    async (file: File) => {
      try {
        const formData = new FormData();
        formData.append("update", file);

        const response = await fetch(apiUrl("/update"), {
          method: "POST",
          body: formData,
        });

        if (!response.ok) {
          throw new Error(`OTA upload failed (HTTP ${response.status})`);
        }

        addLog("INFO", "Plik OTA wyslany, urzadzenie moze sie restartowac");
        return true;
      } catch (err) {
        addLog("ERROR", errorMessage(err, "Blad OTA upload"));
        return false;
      }
    },
    [addLog],
  );

  useEffect(() => {
    let timer: ReturnType<typeof setInterval> | undefined;
    let isStopped = false;

    const start = async () => {
      await refresh();
      if (!isStopped) {
        timer = setInterval(() => {
          void refresh();
        }, 5000);
      }
    };

    void start();

    return () => {
      isStopped = true;
      if (timer) {
        clearInterval(timer);
      }
    };
  }, [refresh]);

  const value: DeviceContextType = useMemo(
    () => ({
      state,
      refresh,
      setTemp,
      setLight,
      setFilter,
      setAPMode,
      feedNow,
      saveSchedule,
      clearCriticalLogs,
      setAlwaysScreenOn,
      setHeaterEnabled,
      setServoPreOffMins,
      setManualServo,
      syncTime,
      uploadFirmware,
      addLog,
    }),
    [
      addLog,
      clearCriticalLogs,
      feedNow,
      refresh,
      saveSchedule,
      setAPMode,
      setAlwaysScreenOn,
      setFilter,
      setHeaterEnabled,
      setLight,
      setManualServo,
      setServoPreOffMins,
      setTemp,
      state,
      syncTime,
      uploadFirmware,
    ],
  );

  return <DeviceContext.Provider value={value}>{children}</DeviceContext.Provider>;
}
