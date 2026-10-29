#include <gtest/gtest.h>

#include <QtGui/QAction>
#include <QtWidgets/QMainWindow>

#include "presentation/main_menu.hpp"

TEST(MainMenuTest, TheExportLabelStartsAtTheDefaultDpi) {
  QMainWindow window;
  MainMenu menu;
  menu.AttachTo(window);

  EXPECT_EQ(menu.Actions().export_png->text(), "&Export png (200 dpi)...");
}

TEST(MainMenuTest, TheExportLabelFollowsTheDpi) {
  QMainWindow window;
  MainMenu menu;
  menu.AttachTo(window);

  menu.ShowExportPngDpi(600);

  EXPECT_EQ(menu.Actions().export_png->text(), "&Export png (600 dpi)...");
}
