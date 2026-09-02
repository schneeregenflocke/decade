// The test binary carries its own main instead of linking gtest_main: the
// widget tests need a QApplication, and it has to stand before the first
// widget. The offscreen platform keeps the run headless — CI has no display,
// and on a developer machine a visible window would steal the focus. It stays
// overridable, so a run under a real display can watch what the simulated
// events do.

#include <QtCore/qtenvironmentvariables.h>
#include <gtest/gtest.h>

#include <QtWidgets/QApplication>

int main(int argc, char** argv) {
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
  }
  const QApplication app(argc, argv);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
