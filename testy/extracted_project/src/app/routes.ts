import { createBrowserRouter } from "react-router";
import { Layout } from "./components/Layout";
import { Dashboard } from "./pages/Dashboard";
import { Schedule } from "./pages/Schedule";
import { Feeder } from "./pages/Feeder";
import { Logs } from "./pages/Logs";
import { Settings } from "./pages/Settings";

export const router = createBrowserRouter([
  {
    path: "/",
    Component: Layout,
    children: [
      { index: true, Component: Dashboard },
      { path: "schedule", Component: Schedule },
      { path: "feeder", Component: Feeder },
      { path: "logs", Component: Logs },
      { path: "settings", Component: Settings },
    ],
  },
]);
