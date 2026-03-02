import React, { createContext, useContext, useState, useCallback } from "react";

export interface LogEntry {
  id: string;
  timestamp: string;
  level: "INFO" | "WARN" | "ERROR";
  message: string;
}

export interface DeviceState {
  currentTemp: number;
  targetTemp: number;
  hysteresis: number;
  lightOn: boolean;
  filterOn: boolean;
  heaterOn: boolean;
  aerationAngle: number; // 0 = off, >0 = on (angle)
  batteryVoltage: number;
  batteryPercent: number;
  schedule: {
    light: { start: string; end: string; enabled: boolean };
    aeration: { start: string; end: string; enabled: boolean };
    filter: { start: string; end: string; enabled: boolean };
    feeding: { time: string; mode: number }; // 0=off, 1=daily
  };
  wifi: {
    connected: boolean;
    ssid: string;
    ip: string;
    rssi: number;
    apActive: boolean;
    apSsid: string;
    apIp: string;
  };
  logs: LogEntry[];
  criticalLogs: LogEntry[];
  uptime: string;
  lastFeedTime: string;
  alwaysScreenOn: boolean;
  servoPreOffMins: number;
  manualServoAngle: number | null;
}

export interface DeviceContextType {
  state: DeviceState;
  setTemp: (target: number, hyst: number) => void;
  toggleLight: () => void;
  toggleFilter: () => void;
  toggleHeater: () => void;
  setAeration: (angle: number) => void;
  feedNow: () => void;
  saveSchedule: (schedule: DeviceState["schedule"]) => void;
  clearCriticalLogs: () => void;
  toggleAP: () => void;
  setAlwaysScreenOn: (val: boolean) => void;
  setServoPreOffMins: (val: number) => void;
  setManualServo: (angle: number | null) => void;
  addLog: (level: LogEntry["level"], message: string) => void;
}

const genId = () => Math.random().toString(36).slice(2, 9);
const now = () => {
  const d = new Date();
  return d.toLocaleTimeString("pl-PL", { hour: "2-digit", minute: "2-digit", second: "2-digit" });
};

const initialLogs: LogEntry[] = [
  { id: genId(), timestamp: "08:00:01", level: "INFO", message: "System uruchomiony" },
  { id: genId(), timestamp: "08:00:03", level: "INFO", message: "WiFi połączono: HomeNetwork (192.168.1.105)" },
  { id: genId(), timestamp: "08:01:00", level: "INFO", message: "DS18B20: 24.8°C" },
  { id: genId(), timestamp: "10:00:00", level: "INFO", message: "Światło WŁĄCZONE (harmonogram)" },
  { id: genId(), timestamp: "10:00:00", level: "INFO", message: "Napowietrzanie WŁĄCZONE (harmonogram)" },
  { id: genId(), timestamp: "10:30:00", level: "INFO", message: "Filtr WŁĄCZONY (harmonogram)" },
  { id: genId(), timestamp: "14:22:05", level: "WARN", message: "Temperatura powyżej 26°C, sprawdź grzałkę" },
  { id: genId(), timestamp: "18:00:00", level: "INFO", message: "Karmienie automatyczne uruchomione" },
  { id: genId(), timestamp: "18:00:14", level: "INFO", message: "Karmienie zakończone (czujnik)" },
  { id: genId(), timestamp: "20:30:00", level: "INFO", message: "Filtr WYŁĄCZONY (harmonogram)" },
];

const initialCriticalLogs: LogEntry[] = [
  { id: genId(), timestamp: "2026-02-10 03:12:00", level: "ERROR", message: "DS18B20: brak odczytu – fail-safe, grzałka OFF" },
  { id: genId(), timestamp: "2026-02-18 17:45:22", level: "ERROR", message: "Temperatura >= 28°C – awaryjne wyłączenie grzałki" },
  { id: genId(), timestamp: "2026-02-25 09:03:11", level: "WARN", message: "Bateria RTC: 2.7V – niski poziom" },
];

const initialState: DeviceState = {
  currentTemp: 24.8,
  targetTemp: 25.0,
  hysteresis: 0.5,
  lightOn: true,
  filterOn: true,
  heaterOn: false,
  aerationAngle: 45,
  batteryVoltage: 3.05,
  batteryPercent: 72,
  schedule: {
    light: { start: "10:00", end: "21:30", enabled: true },
    aeration: { start: "10:00", end: "21:00", enabled: true },
    filter: { start: "10:30", end: "20:30", enabled: true },
    feeding: { time: "18:00", mode: 1 },
  },
  wifi: {
    connected: true,
    ssid: "HomeNetwork",
    ip: "192.168.1.105",
    rssi: -58,
    apActive: false,
    apSsid: "AkwariumPRO-AP",
    apIp: "192.168.4.1",
  },
  logs: initialLogs,
  criticalLogs: initialCriticalLogs,
  uptime: "3d 14h 22m",
  lastFeedTime: "Dziś 18:00",
  alwaysScreenOn: false,
  servoPreOffMins: 30,
  manualServoAngle: null,
};

const DeviceContext = createContext<DeviceContextType | null>(null);

export function DeviceProvider({ children }: { children: React.ReactNode }) {
  const [state, setState] = useState<DeviceState>(initialState);

  const addLog = useCallback((level: LogEntry["level"], message: string) => {
    setState((s) => ({
      ...s,
      logs: [
        { id: genId(), timestamp: now(), level, message },
        ...s.logs.slice(0, 19),
      ],
    }));
  }, []);

  const setTemp = useCallback((target: number, hyst: number) => {
    setState((s) => ({ ...s, targetTemp: target, hysteresis: hyst }));
    addLog("INFO", `Temperatura docelowa: ${target}°C, histereza: ${hyst}°C`);
  }, [addLog]);

  const toggleLight = useCallback(() => {
    setState((s) => {
      const next = !s.lightOn;
      return { ...s, lightOn: next };
    });
    setState((s) => {
      addLog("INFO", `Światło ${s.lightOn ? "WYŁĄCZONE" : "WŁĄCZONE"} (ręcznie)`);
      return s;
    });
  }, [addLog]);

  const toggleFilter = useCallback(() => {
    setState((s) => ({ ...s, filterOn: !s.filterOn }));
    addLog("INFO", "Filtr przełączony ręcznie");
  }, [addLog]);

  const toggleHeater = useCallback(() => {
    setState((s) => ({ ...s, heaterOn: !s.heaterOn }));
    addLog("INFO", "Grzałka przełączona ręcznie");
  }, [addLog]);

  const setAeration = useCallback((angle: number) => {
    setState((s) => ({ ...s, aerationAngle: angle }));
    addLog("INFO", `Napowietrzanie: kąt serwa ${angle}°`);
  }, [addLog]);

  const feedNow = useCallback(() => {
    const t = now();
    setState((s) => ({ ...s, lastFeedTime: `Dziś ${t}` }));
    addLog("INFO", "Karmienie ręczne uruchomione");
    setTimeout(() => addLog("INFO", "Karmienie zakończone (czujnik)"), 2000);
  }, [addLog]);

  const saveSchedule = useCallback((schedule: DeviceState["schedule"]) => {
    setState((s) => ({ ...s, schedule }));
    addLog("INFO", "Harmonogram zapisany");
  }, [addLog]);

  const clearCriticalLogs = useCallback(() => {
    setState((s) => ({ ...s, criticalLogs: [] }));
  }, []);

  const toggleAP = useCallback(() => {
    setState((s) => {
      const next = !s.wifi.apActive;
      addLog("INFO", `AP ${next ? "uruchomiony" : "zatrzymany"}`);
      return { ...s, wifi: { ...s.wifi, apActive: next } };
    });
  }, [addLog]);

  const setAlwaysScreenOn = useCallback((val: boolean) => {
    setState((s) => ({ ...s, alwaysScreenOn: val }));
  }, []);

  const setServoPreOffMins = useCallback((val: number) => {
    setState((s) => ({ ...s, servoPreOffMins: val }));
  }, []);

  const setManualServo = useCallback((angle: number | null) => {
    setState((s) => ({ ...s, manualServoAngle: angle }));
    if (angle !== null) addLog("INFO", `Ręczny override serwa: ${angle}°`);
    else addLog("INFO", "Ręczny override serwa wyczyszczony");
  }, [addLog]);

  return (
    <DeviceContext.Provider
      value={{
        state,
        setTemp,
        toggleLight,
        toggleFilter,
        toggleHeater,
        setAeration,
        feedNow,
        saveSchedule,
        clearCriticalLogs,
        toggleAP,
        setAlwaysScreenOn,
        setServoPreOffMins,
        setManualServo,
        addLog,
      }}
    >
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  const ctx = useContext(DeviceContext);
  if (!ctx) throw new Error("useDevice must be used within DeviceProvider");
  return ctx;
}
