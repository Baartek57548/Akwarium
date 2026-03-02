// Generate mock 24-hour temperature history
export interface TempPoint {
  time: string;
  temp: number;
}

export function generateTempHistory(): TempPoint[] {
  const base = 24.8;
  const points: TempPoint[] = [];
  for (let i = 0; i < 48; i++) {
    const h = Math.floor(i / 2);
    const m = i % 2 === 0 ? "00" : "30";
    const noise = (Math.random() - 0.5) * 0.6;
    const dayVariation = Math.sin((i / 48) * Math.PI * 2 - Math.PI / 2) * 0.4;
    const temp = Math.round((base + dayVariation + noise) * 10) / 10;
    points.push({ time: `${String(h).padStart(2, "0")}:${m}`, temp });
  }
  return points;
}

export const tempHistory = generateTempHistory();
