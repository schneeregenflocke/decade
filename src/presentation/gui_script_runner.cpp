#include "gui_script_runner.hpp"

#include <QtTest/qtestkeyboard.h>
#include <QtTest/qtestmouse.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <expected>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "../common/debug_log.hpp"
#include "gui_script.hpp"

namespace {

using StepResult = std::expected<void, std::string>;

// What a person reads on a widget: without the mnemonic ampersand and without
// the ellipsis a menu entry carries.
QString Readable(QString text) {
  text.remove('&');
  if (text.endsWith("...")) {
    text.chop(3);
  }
  return text.trimmed();
}

bool Reads(const QString& text, const std::string& wanted) {
  return Readable(text).compare(QString::fromStdString(wanted),
                                Qt::CaseInsensitive) == 0;
}

// Every visible widget of type T across all open windows, dialogues included.
// A widget on a tab that is not shown is not visible, so it drops out.
template <typename T>
std::vector<T*> VisibleWidgets() {
  std::vector<T*> found;
  for (QWidget* widget : QApplication::allWidgets()) {
    if (auto* match = qobject_cast<T*>(widget);
        match != nullptr && match->isVisible()) {
      found.push_back(match);
    }
  }
  return found;
}

template <typename T, typename Predicate>
std::expected<T*, std::string> TheOneVisible(const std::string& what,
                                             Predicate matches) {
  std::vector<T*> hits;
  std::ranges::copy_if(
      VisibleWidgets<T>(), std::back_inserter(hits),
      [&matches](const T* widget) { return matches(*widget); });
  if (hits.empty()) {
    return std::unexpected("no visible " + what);
  }
  if (hits.size() > 1) {
    return std::unexpected("more than one visible " + what);
  }
  return hits.front();
}

std::expected<int, std::string> ColumnHeaded(const QTableWidget& table,
                                             const std::string& header) {
  for (int column = 0; column < table.columnCount(); ++column) {
    const QTableWidgetItem* item = table.horizontalHeaderItem(column);
    if (item != nullptr && Reads(item->text(), header)) {
      return column;
    }
  }
  return std::unexpected("no column headed " + header);
}

// The centre of a cell in viewport coordinates, scrolled into view first — a
// person scrolls before clicking a row below the fold.
std::expected<QPoint, std::string> CellCenter(QTableWidget& table, int row,
                                              int column) {
  if (row < 0 || row >= table.rowCount()) {
    return std::unexpected("no row " + std::to_string(row + 1));
  }
  const QModelIndex index = table.model()->index(row, column);
  table.scrollTo(index);
  return table.visualRect(index).center();
}

std::expected<QTableWidget*, std::string> VisibleTable() {
  return TheOneVisible<QTableWidget>("table",
                                     [](const QTableWidget&) { return true; });
}

StepResult SelectTab(const std::string& label) {
  for (const QTabWidget* tabs : VisibleWidgets<QTabWidget>()) {
    for (int index = 0; index < tabs->count(); ++index) {
      if (Reads(tabs->tabText(index), label)) {
        QTabBar* bar = tabs->tabBar();
        QTest::mouseClick(bar, Qt::LeftButton, Qt::KeyboardModifiers(),
                          bar->tabRect(index).center());
        return {};
      }
    }
  }
  return std::unexpected("no tab labelled " + label);
}

StepResult ClickButton(const std::string& text) {
  const auto button = TheOneVisible<QAbstractButton>(
      "button reading " + text, [&text](const QAbstractButton& candidate) {
        return candidate.isEnabled() && Reads(candidate.text(), text);
      });
  if (!button) {
    return std::unexpected(button.error());
  }
  QTest::mouseClick(*button, Qt::LeftButton);
  return {};
}

StepResult EditCell(const std::string& row_word, const std::string& header,
                    const std::string& text) {
  const auto row = ParseInteger(row_word);
  const auto table = VisibleTable();
  if (!row || !table) {
    return std::unexpected(!row ? row.error() : table.error());
  }
  const auto column = ColumnHeaded(**table, header);
  if (!column) {
    return std::unexpected(column.error());
  }
  const auto center = CellCenter(**table, *row - 1, *column);
  if (!center) {
    return std::unexpected(center.error());
  }
  // Click, then F2: the table opens its editor on EditKeyPressed. A synthetic
  // double click does not reach the view as an edit trigger.
  QTest::mouseClick((*table)->viewport(), Qt::LeftButton,
                    Qt::KeyboardModifiers(), *center);
  QTest::keyClick(*table, Qt::Key_F2);
  auto* editor = (*table)->viewport()->findChild<QLineEdit*>();
  if (editor == nullptr || !editor->isVisible()) {
    return std::unexpected("the cell did not open an editor");
  }
  QTest::keyClick(editor, Qt::Key_A, Qt::ControlModifier);
  QTest::keyClicks(editor, QString::fromStdString(text));
  QTest::keyClick(editor, Qt::Key_Return);
  // The delegate commits on Return through a queued call, so the editor sees
  // the key first; the cell holds the text only once that call ran.
  QCoreApplication::processEvents();
  const QTableWidgetItem* cell = (*table)->item(*row - 1, *column);
  if (cell == nullptr || cell->text() != QString::fromStdString(text)) {
    return std::unexpected(
        "the cell reads '" +
        (cell != nullptr ? cell->text().toStdString() : std::string{}) +
        "' afterwards, not '" + text + "'");
  }
  return {};
}

StepResult SelectRows(const std::vector<std::string>& words) {
  const auto rows = ParseRowList(words);
  const auto table = VisibleTable();
  if (!rows || !table) {
    return std::unexpected(!rows ? rows.error() : table.error());
  }
  bool first = true;
  for (const int row : *rows) {
    const auto center = CellCenter(**table, row - 1, 0);
    if (!center) {
      return std::unexpected(center.error());
    }
    QTest::mouseClick((*table)->viewport(), Qt::LeftButton,
                      first ? Qt::KeyboardModifiers() : Qt::ControlModifier,
                      *center);
    first = false;
  }
  std::vector<int> selected;
  for (const QModelIndex& index : (*table)->selectionModel()->selectedRows()) {
    selected.push_back(index.row() + 1);
  }
  std::vector<int> wanted = *rows;
  std::ranges::sort(selected);
  std::ranges::sort(wanted);
  if (selected != wanted) {
    return std::unexpected("the table did not select exactly those rows");
  }
  return {};
}

StepResult Choose(const std::string& label_text, const std::string& item) {
  const auto label = TheOneVisible<QLabel>(
      "label reading " + label_text, [&label_text](const QLabel& candidate) {
        return Reads(candidate.text(), label_text);
      });
  if (!label) {
    return std::unexpected(label.error());
  }
  auto* combo = qobject_cast<QComboBox*>((*label)->buddy());
  if (combo == nullptr) {
    return std::unexpected("no combo box beside " + label_text);
  }
  const int index = combo->findText(QString::fromStdString(item));
  if (index < 0) {
    return std::unexpected("no entry " + item + " in " + label_text);
  }
  QTest::mouseClick(combo, Qt::LeftButton);
  QAbstractItemView* list = combo->view();
  if (!list->isVisible()) {
    return std::unexpected("the combo box did not open");
  }
  // Keys, not a click: the popup swallows a release right after it opened, so
  // an opening press cannot pick an entry by accident.
  QTest::keyClick(list, Qt::Key_Home);
  for (int step = 0; step < index; ++step) {
    QTest::keyClick(list, Qt::Key_Down);
  }
  QTest::keyClick(list, Qt::Key_Return);
  if (list->isVisible() || combo->currentIndex() != index) {
    return std::unexpected("the combo box did not take " + item);
  }
  return {};
}

StepResult PickColor(const std::string& row_word, const std::string& hex) {
  const auto row = ParseInteger(row_word);
  const auto table = VisibleTable();
  if (!row || !table) {
    return std::unexpected(!row ? row.error() : table.error());
  }
  const auto column = ColumnHeaded(**table, "Color");
  if (!column) {
    return std::unexpected(column.error());
  }
  const auto center = CellCenter(**table, *row - 1, *column);
  if (!center) {
    return std::unexpected(center.error());
  }
  QTest::mouseClick((*table)->viewport(), Qt::LeftButton,
                    Qt::KeyboardModifiers(), *center);

  const auto dialog = TheOneVisible<QColorDialog>(
      "colour dialogue", [](const QColorDialog&) { return true; });
  if (!dialog) {
    return std::unexpected(dialog.error());
  }
  // The HTML field is the one line edit that holds a #rrggbb value.
  QLineEdit* html = nullptr;
  for (QLineEdit* edit : (*dialog)->findChildren<QLineEdit*>()) {
    if (edit->isVisible() && edit->text().startsWith('#')) {
      html = edit;
    }
  }
  const auto* buttons = (*dialog)->findChild<QDialogButtonBox*>();
  QPushButton* accept =
      buttons != nullptr ? buttons->button(QDialogButtonBox::Ok) : nullptr;
  if (html == nullptr || accept == nullptr) {
    return std::unexpected(
        "the colour dialogue has no HTML field or OK button");
  }
  QTest::keyClick(html, Qt::Key_A, Qt::ControlModifier);
  QTest::keyClicks(html, QString::fromStdString(hex));
  QTest::mouseClick(accept, Qt::LeftButton);
  if ((*dialog)->isVisible()) {
    return std::unexpected("the colour dialogue did not close");
  }
  return {};
}

// Opens a menu of the menu bar and clicks one of its entries. The entry may
// open a modal dialogue, in which case this returns once it closed again.
StepResult ChooseMenuEntry(QMainWindow& window, const std::string& menu_text,
                           const std::string& entry_text) {
  QMenuBar* bar = window.menuBar();
  for (QAction* menu_action : bar->actions()) {
    if (menu_action->menu() == nullptr ||
        !Reads(menu_action->text(), menu_text)) {
      continue;
    }
    QTest::mouseClick(bar, Qt::LeftButton, Qt::KeyboardModifiers(),
                      bar->actionGeometry(menu_action).center());
    QMenu* menu = menu_action->menu();
    for (QAction* entry : menu->actions()) {
      if (Reads(entry->text(), entry_text)) {
        QTest::mouseClick(menu, Qt::LeftButton, Qt::KeyboardModifiers(),
                          menu->actionGeometry(entry).center());
        return {};
      }
    }
    std::string message = "no entry ";
    message += entry_text;
    message += " in menu ";
    message += menu_text;
    return std::unexpected(message);
  }
  return std::unexpected("no menu " + menu_text);
}

StepResult FillFileDialog(const std::string& path) {
  const auto dialog = TheOneVisible<QFileDialog>(
      "file dialogue", [](const QFileDialog&) { return true; });
  if (!dialog) {
    return std::unexpected(dialog.error());
  }
  // Qt's own dialogue names its file name field; a native one has none, which
  // is why a script run switches native dialogues off.
  auto* name = (*dialog)->findChild<QLineEdit*>("fileNameEdit");
  if (name == nullptr) {
    return std::unexpected("the file dialogue has no file name field");
  }
  const QString absolute =
      QFileInfo(QString::fromStdString(path)).absoluteFilePath();
  QTest::keyClick(name, Qt::Key_A, Qt::ControlModifier);
  QTest::keyClicks(name, absolute);
  QTest::keyClick(name, Qt::Key_Return);
  return {};
}

}  // namespace

GuiScriptRunner::GuiScriptRunner(std::vector<GuiStep> steps,
                                 QMainWindow& window)
    : QObject(&window), steps_(std::move(steps)), window_(&window) {}

void GuiScriptRunner::Start() {
  QTimer::singleShot(kStepDelayMs, this, [this]() { RunNext(); });
}

void GuiScriptRunner::RunNext() {
  if (stopped_ || next_step_ >= steps_.size() || window_.isNull()) {
    return;
  }
  const GuiStep step = steps_[next_step_];
  ++next_step_;

  int delay_ms = kStepDelayMs;
  if (step.command == "wait") {
    const auto wait_ms = ParseInteger(step.arguments.front());
    if (!wait_ms || *wait_ms < 0) {
      Fail(step, "wait needs a non-negative number of milliseconds");
      return;
    }
    delay_ms = *wait_ms;
  }
  QTimer::singleShot(delay_ms, this, [this]() { RunNext(); });

  if (decade_debug::LogEnabled()) {
    std::cerr << "script: line " << step.line << ": " << step.command;
    for (const std::string& argument : step.arguments) {
      std::cerr << " \"" << argument << '"';
    }
    std::cerr << '\n';
  }
  if (const auto result = Execute(step); !result) {
    Fail(step, result.error());
  }
}

std::expected<void, std::string> GuiScriptRunner::Execute(const GuiStep& step) {
  const auto& arguments = step.arguments;
  if (step.command == "tab") {
    return SelectTab(arguments[0]);
  }
  if (step.command == "click") {
    return ClickButton(arguments[0]);
  }
  if (step.command == "edit-cell") {
    return EditCell(arguments[0], arguments[1], arguments[2]);
  }
  if (step.command == "select-rows") {
    return SelectRows(arguments);
  }
  if (step.command == "choose") {
    return Choose(arguments[0], arguments[1]);
  }
  if (step.command == "pick-color") {
    return PickColor(arguments[0], arguments[1]);
  }
  if (step.command == "menu") {
    return ChooseMenuEntry(*window_, arguments[0], arguments[1]);
  }
  if (step.command == "file-dialog") {
    return FillFileDialog(arguments[0]);
  }
  if (step.command == "quit") {
    window_->close();
  }
  return {};
}

void GuiScriptRunner::Fail(const GuiStep& step, const std::string& message) {
  stopped_ = true;
  std::cerr << "script: line " << step.line << ": " << message << '\n';
  QCoreApplication::exit(1);
}
