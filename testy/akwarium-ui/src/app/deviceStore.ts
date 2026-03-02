import { createContext } from "react";

export interface LogEntry {
  id: string;
  timestamp: string;
  level: "INFO" | "WARN" | "ERROR";
  message: string;
}

export interface TempPoint {
  time: string;
  temp: number;
  epochMs: number;
}

export interface DeviceSchedule {
  light: { start: string; end: string };
  aeration: { start: string; end: string };
  filter: { start: string; end: string };
  feeding: { time: string; mode: number };
  servoPreOffMins: number;
}

export interface DeviceState {
  loading: boolean;
  online: boolean;
  lastError: string | null;
  lastSyncAt: number | null;

  currentTemp: number;
  targetTemp: number;
  hysteresis: number;
  minTemp: number | null;
  minTempTimeEpoch: number | null;

  relays: {
    light: boolean;
    pump: boolean;
    heater: boolean;
  };
  aerationAngle: number;
  manualServoAngle: number | null;

  batteryVoltage: number;
  batteryPercent: number;

  schedule: DeviceSchedule;

  wifi: {
    connected: boolean;
    ip: string;
    apMode: boolean;
    modeLabel: string;
  };

  settings: {
    alwaysScreenOn: boolean;
    heaterEnabled: boolean;
  };

  manual: {
    lightOverride: boolean;
    lightState: boolean;
    filterOverride: boolean;
    filterState: boolean;
  };

  logs: LogEntry[];
  criticalLogs: LogEntry[];

  lastFeedEpoch: number;
  lastFeedTime: string;

  tempHistory: TempPoint[];
}

export interface DeviceContextType {
  state: DeviceState;
  apiBaseUrl: string;
  apiBaseSource: "proxy" | "env" | "manual";
  refresh: () => Promise<void>;
  setApiBaseUrl: (raw: string) => { ok: true; normalized: string } | { ok: false; error: string };
  resetApiBaseUrl: () => void;
  setTemp: (target: number, hyst: number) => Promise<boolean>;
  setLight: (enabled: boolean) => Promise<boolean>;
  setFilter: (enabled: boolean) => Promise<boolean>;
  setAPMode: (enabled: boolean) => Promise<boolean>;
  feedNow: () => Promise<boolean>;
  saveSchedule: (schedule: DeviceSchedule) => Promise<boolean>;
  clearCriticalLogs: () => Promise<boolean>;
  setAlwaysScreenOn: (enabled: boolean) => Promise<boolean>;
  setHeaterEnabled: (enabled: boolean) => Promise<boolean>;
  setServoPreOffMins: (val: number) => Promise<boolean>;
  setManualServo: (angle: number | null) => Promise<boolean>;
  syncTime: () => Promise<boolean>;
  uploadFirmware: (file: File) => Promise<boolean>;
  addLog: (level: LogEntry["level"], message: string) => void;
}

export const DeviceContext = createContext<DeviceContextType | null>(null);
