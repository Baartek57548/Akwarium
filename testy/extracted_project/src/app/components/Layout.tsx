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
    <div className="flex items-start justify-center min-h-screen" style={{ background: "#060b18" }}>
      <div
        className="relative flex flex-col overflow-hidden"
        style={{
          width: "100%",
          maxWidth: 430,
          minHeight: "100svh",
          background: "#080e1c",
        }}
      >
        {/* Page content */}
        <main className="flex-1 overflow-y-auto pb-20">
          <Outlet />
        </main>

        {/* Bottom nav */}
        <nav
          className="fixed bottom-0 left-1/2 z-50 flex items-center"
          style={{
            transform: "translateX(-50%)",
            width: "100%",
            maxWidth: 430,
            background: "rgba(8,14,28,0.96)",
            borderTop: "1px solid rgba(6,182,212,0.15)",
            backdropFilter: "blur(12px)",
            WebkitBackdropFilter: "blur(12px)",
          }}
        >
          {tabs.map(({ to, label, icon: Icon }) => (
            <NavLink
              key={to}
              to={to}
              end={to === "/"}
              className={({ isActive }) =>
                `flex flex-col items-center justify-center flex-1 py-3 gap-0.5 transition-all ${
                  isActive ? "text-cyan-400" : "text-slate-500"
                }`
              }
            >
              {({ isActive }) => (
                <>
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
