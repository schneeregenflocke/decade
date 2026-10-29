#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/Qt>
#include <QtTest/QTest>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <vector>

#include "domain/page_setup_config.hpp"
#include "domain/png_export_config.hpp"
#include "presentation/png_export_panel.hpp"

namespace {

PngExportConfig ConfigAt(int dpi) {
  PngExportConfig config;
  config.SetDpi(dpi);
  return config;
}

PageSetupConfig A4Landscape() {
  PageSetupConfig page;
  page.SetSize({297.0F, 210.0F});
  return page;
}

QString LabelText(const PngExportPanel& panel, const char* name) {
  const auto* label = panel.findChild<QLabel*>(name);
  return label == nullptr ? QString() : label->text();
}

// Collects every edit the panel reports, so a test can assert on the count.
struct EditLog {
  explicit EditLog(PngExportPanel& panel) {
    QObject::connect(
        &panel, &PngExportPanel::PngExportConfigEdited,
        [this](const PngExportConfig& config) { edits.push_back(config); });
  }
  std::vector<PngExportConfig> edits;
};

}  // namespace

TEST(PngExportPanelTest, ShowsPixelsMillimetresAndEffectiveDpi) {
  PngExportPanel panel(nullptr);
  panel.ReceivePageSetup(A4Landscape());
  panel.ReceivePngExportConfig(ConfigAt(300));

  EXPECT_EQ(LabelText(panel, "pixel_size"), "3508 × 2480 px");
  EXPECT_EQ(LabelText(panel, "millimetre_size"), "297.0 × 210.0 mm");
  EXPECT_EQ(LabelText(panel, "effective_dpi"), "300.011 × 299.962 dpi");
}

TEST(PngExportPanelTest, AnImpossibleSizeShowsADash) {
  PngExportPanel panel(nullptr);
  panel.ReceivePngExportConfig(ConfigAt(300));
  panel.ReceivePageSetup(PageSetupConfig{});

  EXPECT_EQ(LabelText(panel, "pixel_size"), "–");
  EXPECT_EQ(LabelText(panel, "effective_dpi"), "–");
}

TEST(PngExportPanelTest, TypingADpiAndEnterReportsIt) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  panel.show();
  auto* dpi = panel.findChild<QComboBox*>("dpi");
  ASSERT_NE(dpi, nullptr);

  dpi->lineEdit()->selectAll();
  QTest::keyClicks(dpi->lineEdit(), "150");
  QTest::keyClick(dpi->lineEdit(), Qt::Key_Return);

  ASSERT_EQ(log.edits.size(), 1U);
  EXPECT_EQ(log.edits[0].Dpi(), 150);
}

// A preset must not complete a typed prefix: "30" stays 30, not 300.
TEST(PngExportPanelTest, TypingAPrefixOfAPresetKeepsTheTypedDpi) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  panel.show();
  auto* dpi = panel.findChild<QComboBox*>("dpi");
  ASSERT_NE(dpi, nullptr);
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&panel));
  dpi->setFocus();

  dpi->lineEdit()->selectAll();
  QTest::keyClicks(dpi->lineEdit(), "30");
  QTest::keyClick(dpi->lineEdit(), Qt::Key_Return);

  ASSERT_EQ(log.edits.size(), 1U);
  EXPECT_EQ(log.edits[0].Dpi(), 30);
}

TEST(PngExportPanelTest, AnIncompleteDpiReportsNothingAndStaysWhileTyping) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  panel.show();
  auto* dpi = panel.findChild<QComboBox*>("dpi");
  ASSERT_NE(dpi, nullptr);

  dpi->lineEdit()->selectAll();
  QTest::keyClick(dpi->lineEdit(), Qt::Key_Backspace);
  QTest::keyClicks(dpi->lineEdit(), "0");

  EXPECT_TRUE(log.edits.empty());
  EXPECT_EQ(dpi->lineEdit()->text(), "0");
}

// Leaving the box with an invalid value would show one dpi and export another.
TEST(PngExportPanelTest, LeavingAnInvalidDpiRestoresTheConfiguredOne) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  panel.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&panel));
  auto* dpi = panel.findChild<QComboBox*>("dpi");
  auto* path = panel.findChild<QLineEdit*>("file_path");
  ASSERT_NE(dpi, nullptr);
  ASSERT_NE(path, nullptr);
  dpi->setFocus();

  dpi->lineEdit()->selectAll();
  QTest::keyClick(dpi->lineEdit(), Qt::Key_Backspace);
  QTest::keyClicks(dpi->lineEdit(), "0");
  path->setFocus();

  EXPECT_TRUE(log.edits.empty());
  EXPECT_EQ(dpi->lineEdit()->text(), "300");
}

TEST(PngExportPanelTest, ChoosingMsaaSamplesReportsThem) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  auto* samples = panel.findChild<QComboBox*>("msaa_samples");
  ASSERT_NE(samples, nullptr);

  samples->setCurrentIndex(samples->findText("4"));

  ASSERT_EQ(log.edits.size(), 1U);
  EXPECT_EQ(log.edits[0].MsaaSamples(), 4);
}

TEST(PngExportPanelTest, ReceivingAConfigShowsItWithoutReporting) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  PngExportConfig config = ConfigAt(600);
  config.SetMsaaSamples(8);
  config.SetFilePath("out/a.png");

  panel.ReceivePngExportConfig(config);

  EXPECT_TRUE(log.edits.empty());
  EXPECT_EQ(panel.findChild<QComboBox*>("dpi")->currentText(), "600");
  EXPECT_EQ(panel.findChild<QComboBox*>("msaa_samples")->currentText(), "8");
  EXPECT_EQ(panel.findChild<QLineEdit*>("file_path")->text(), "out/a.png");
}

TEST(PngExportPanelTest, EditingThePathReportsIt) {
  PngExportPanel panel(nullptr);
  EditLog log(panel);
  panel.show();
  auto* path = panel.findChild<QLineEdit*>("file_path");
  ASSERT_NE(path, nullptr);

  QTest::keyClicks(path, "b.png");
  QTest::keyClick(path, Qt::Key_Return);

  ASSERT_FALSE(log.edits.empty());
  EXPECT_EQ(log.edits.back().FilePath(), "b.png");
}

TEST(PngExportPanelTest, TheProjectFileSetsThePlaceholder) {
  PngExportPanel panel(nullptr);

  panel.ReceiveProjectFilePath("/home/x/plan.xml");

  EXPECT_EQ(panel.findChild<QLineEdit*>("file_path")->placeholderText(),
            "/home/x/plan.png");
}

TEST(PngExportPanelTest, TheExportButtonRequestsAnExport) {
  PngExportPanel panel(nullptr);
  int requests = 0;
  QObject::connect(&panel, &PngExportPanel::ExportRequested,
                   [&requests] { ++requests; });
  panel.show();
  auto* button = panel.findChild<QPushButton*>("export");
  ASSERT_NE(button, nullptr);

  QTest::mouseClick(button, Qt::LeftButton);

  EXPECT_EQ(requests, 1);
}

TEST(PngExportPanelTest, ASavedExportShowsPathAndTime) {
  PngExportPanel panel(nullptr);

  panel.ShowSaved("/tmp/a.png", QTime(14, 32, 5));

  EXPECT_EQ(LabelText(panel, "saved"), "Saved: /tmp/a.png, 14:32:05");
}

TEST(PngExportPanelTest, TheBrowseButtonRequestsAFileChoice) {
  PngExportPanel panel(nullptr);
  int requests = 0;
  QObject::connect(&panel, &PngExportPanel::BrowseRequested,
                   [&requests] { ++requests; });
  panel.show();
  auto* button = panel.findChild<QPushButton*>("browse");
  ASSERT_NE(button, nullptr);

  QTest::mouseClick(button, Qt::LeftButton);

  EXPECT_EQ(requests, 1);
}

TEST(PngExportPanelTest, AnotherProjectClearsTheSavedLine) {
  PngExportPanel panel(nullptr);
  panel.ReceiveProjectFilePath("/home/x/plan.xml");
  panel.ShowSaved("/home/x/plan.png", QTime(14, 32, 5));

  panel.ReceiveProjectFilePath("/home/x/other.xml");

  EXPECT_EQ(LabelText(panel, "saved"), "");
}

// Saving the project publishes its path again; the export still stands.
TEST(PngExportPanelTest, TheSameProjectKeepsTheSavedLine) {
  PngExportPanel panel(nullptr);
  panel.ReceiveProjectFilePath("/home/x/plan.xml");
  panel.ShowSaved("/home/x/plan.png", QTime(14, 32, 5));

  panel.ReceiveProjectFilePath("/home/x/plan.xml");

  EXPECT_EQ(LabelText(panel, "saved"), "Saved: /home/x/plan.png, 14:32:05");
}

TEST(PngExportPanelTest, AFailedExportClearsTheSavedLine) {
  PngExportPanel panel(nullptr);
  panel.ShowSaved("/home/x/plan.png", QTime(14, 32, 5));

  panel.ClearSaved();

  EXPECT_EQ(LabelText(panel, "saved"), "");
}
