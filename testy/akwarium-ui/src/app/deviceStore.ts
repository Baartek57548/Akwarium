import { createContext } from "react";

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
  aerationAngle: number; // 0 = off, >0 = on
  batteryVoltage: number;
  batteryPercent: number;
  schedule: {
    light: { start: string; end: string; enabled: boolean };
    aeration: { start: string; end: string; enabled: boolean };
    filter: { start: string; end: string; enabled: boolean };
    feeding: { time: string; mode: number };
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

export const DeviceContext = createContext<DeviceContextType | null>(null);
