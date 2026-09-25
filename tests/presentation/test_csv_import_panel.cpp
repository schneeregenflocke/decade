#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QCheckBox>
#include <vector>

#include "domain/csv_import_options.hpp"
#include "presentation/csv_import_panel.hpp"

TEST(CsvImportPanelTest, ClickingTheTitleOptionReportsTheNewOptions) {
  CsvImportPanel panel(nullptr);
  std::vector<bool> reported;
  QObject::connect(&panel, &CsvImportPanel::CsvImportOptionsEdited,
                   [&reported](const CsvImportOptions& options) {
                     reported.push_back(options.TitleFromFileName());
                   });
  panel.show();
  auto* title_option = panel.findChild<QCheckBox*>();
  ASSERT_NE(title_option, nullptr);
  ASSERT_TRUE(title_option->isChecked());

  QTest::mouseClick(title_option, Qt::LeftButton);

  EXPECT_EQ(reported, std::vector<bool>{false});
}
