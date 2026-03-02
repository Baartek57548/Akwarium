import { RouterProvider } from "react-router";
import { router } from "./routes";
import { DeviceProvider } from "./deviceContext";

export default function App() {
  return (
    <DeviceProvider>
      <RouterProvider router={router} />
    </DeviceProvider>
  );
}
