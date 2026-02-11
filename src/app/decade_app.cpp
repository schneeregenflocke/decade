#include "decade_app.hpp"

#include <locale>
#include <memory>

#include "app_config.hpp"
#include "main_window.hpp"

bool DecadeApp::OnInit()
{
  locale_.Init();
  std::locale::global(std::locale(""));

  const app::MainWindowConfig window_config = app::DefaultMainWindowConfig();
  auto *main_window =
      std::make_unique<MainWindow>(nullptr, window_config.title, window_config.position,
                                   window_config.size, window_config.maximize_on_start)
          .release();
  SetTopWindow(main_window);
  return true;
}
