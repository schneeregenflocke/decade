#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <QtCore/QPointer>

class QAction;
class QMainWindow;
class QString;

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
  explicit MainMenu(int export_png_dpi);

  [[nodiscard]] const MainMenuActions& Actions() const;

  // Builds the menu into the window's menu bar. It has to run before the window
  // connects — the actions come into being here.
  void AttachTo(QMainWindow& window);

 private:
  [[nodiscard]] QString ExportPngLabel() const;

  MainMenuActions actions_;
  int export_png_dpi_;
};

#endif  // MAIN_MENU_HPP
