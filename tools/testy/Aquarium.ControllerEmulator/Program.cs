using Aquarium.ControllerEmulator.Forms;

namespace Aquarium.ControllerEmulator;

internal static class Program
{
  [STAThread]
  private static void Main()
  {
    ApplicationConfiguration.Initialize();
    Application.Run(new MainForm());
  }
}
