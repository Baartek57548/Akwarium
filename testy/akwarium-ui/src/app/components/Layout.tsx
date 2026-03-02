import { NavLink, Outlet } from "react-router";
import { LayoutDashboard, CalendarClock, Fish, ScrollText, Settings2 } from "lucide-react";

const tabs = [
  { to: "/", label: "Panel", icon: LayoutDashboard },
  { to: "/schedule", label: "Harmonogram", icon: CalendarClock },
  { to: "/feeder", label: "Karmnik", icon: Fish },
  { to: "/logs", label: "Logi", icon: ScrollText },
  { to: "/settings", label: "Ustawienia", icon: Settings2 },
];

export function Layout() {
  return (
    <div className="app-shell">
      <div className="app-phone-frame">
        <main className="app-content">
          <Outlet />
        </main>

        <nav className="app-nav">
          {tabs.map(({ to, label, icon: Icon }) => (
            <NavLink
              key={to}
              to={to}
              end={to === "/"}
              className={({ isActive }) =>
                `relative flex flex-col items-center justify-center flex-1 py-3 gap-0.5 transition-all ${
                  isActive ? "text-cyan-400" : "text-slate-500"
                }`
              }
            >
              {({ isActive }) => (
                <>
                  <div
                    className="absolute top-0 h-[2px] rounded-full transition-all"
                    style={{
                      width: isActive ? 24 : 0,
                      background: "var(--ui-accent)",
                      boxShadow: isActive ? "0 0 10px rgba(34,211,238,0.8)" : "none",
                    }}
                  />
                  <Icon
                    size={20}
                    className={isActive ? "drop-shadow-[0_0_6px_rgba(34,211,238,0.7)]" : ""}
                  />
                  <span style={{ fontSize: 10 }} className={isActive ? "font-medium" : ""}>
                    {label}
                  </span>
                </>
              )}
            </NavLink>
          ))}
        </nav>
      </div>
    </div>
  );
}
