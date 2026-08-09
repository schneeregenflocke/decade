#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

// The commands of the main menu. Qt addresses a menu entry through its QAction
// rather than through an identifier, which is why these stand here instead of
// the integer ids wx needed: the action is the entry, and the window connects
// its handler straight to it. QPointer, because the menu bar owns the actions
// and the window outlives neither.
struct MainMenuActions {
  QPointer<QAction> open_xml;
  QPointer<QAction> save_xml;
  QPointer<QAction> save_as_xml;
  QPointer<QAction> import_csv;
  QPointer<QAction> export_csv;
  QPointer<QAction> export_png;
  QPointer<QAction> quit;
  QPointer<QAction> license_info;
};

// Presentation: it owns the menu *layout* alone (File plus Help). What a
// command does is the window's decision; this class builds the menu bar, hangs
// it onto a window and publishes the actions the window connects to. The menu
// changes when the menu changes — independently of wiring and commands.
class MainMenu {
 public:
  explicit MainMenu(int export_png_dpi) : export_png_dpi_(export_png_dpi) {}

  [[nodiscard]] const MainMenuActions& Actions() const { return actions_; }

  // Builds the menu into the window's menu bar. It has to run before the window
  // connects — the actions come into being here.
  void AttachTo(QMainWindow& window) {
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

 private:
  [[nodiscard]] QString ExportPngLabel() const {
    return QString("&Export png (%1 dpi)...").arg(export_png_dpi_);
  }

  MainMenuActions actions_;
  int export_png_dpi_;
};

#endif  // MAIN_MENU_HPP
