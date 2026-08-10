#include "main_menu.hpp"

#include <QtCore/QString>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

MainMenu::MainMenu(int export_png_dpi) : export_png_dpi_(export_png_dpi) {}

const MainMenuActions& MainMenu::Actions() const { return actions_; }

void MainMenu::AttachTo(QMainWindow& window) {
  QMenuBar* menu_bar = window.menuBar();

  QMenu* file_menu = menu_bar->addMenu("&File");
  actions_.open_xml = file_menu->addAction("&Open...");
  file_menu->addSeparator();
  actions_.save_xml = file_menu->addAction("&Save");
  actions_.save_xml->setShortcut(QKeySequence::Save);
  actions_.save_as_xml = file_menu->addAction("Save &As...");
  file_menu->addSeparator();
  actions_.import_csv = file_menu->addAction("&Import csv...");
  actions_.export_csv = file_menu->addAction("&Export csv...");
  file_menu->addSeparator();
  actions_.export_png = file_menu->addAction(ExportPngLabel());
  file_menu->addSeparator();
  actions_.quit = file_menu->addAction("E&xit");
  actions_.quit->setShortcut(QKeySequence::Quit);

  QMenu* help_menu = menu_bar->addMenu("&Help");
  actions_.license_info = help_menu->addAction("&Open Source Licenses");
}

QString MainMenu::ExportPngLabel() const {
  return QString("&Export png (%1 dpi)...").arg(export_png_dpi_);
}
