#include <gtest/gtest.h>

#include "domain/png_export_config.hpp"

TEST(PngExportConfigTest, StartsAtTheDefaults) {
  const PngExportConfig config;

  EXPECT_EQ(config.Dpi(), 300);
  EXPECT_EQ(config.MsaaSamples(), 16);
  EXPECT_TRUE(config.FilePath().empty());
}

TEST(PngExportConfigTest, DpiStaysWithinItsRange) {
  PngExportConfig config;

  config.SetDpi(0);
  EXPECT_EQ(config.Dpi(), 1);
  config.SetDpi(5000);
  EXPECT_EQ(config.Dpi(), 2400);
}

TEST(PngExportConfigTest, MsaaSamplesStayWithinTheirRange) {
  PngExportConfig config;

  config.SetMsaaSamples(0);
  EXPECT_EQ(config.MsaaSamples(), 1);
  config.SetMsaaSamples(32);
  EXPECT_EQ(config.MsaaSamples(), 16);
}

TEST(PngExportConfigTest, DefaultPathSitsBesideTheProjectFile) {
  EXPECT_EQ(DefaultPngFilePath("/p/plan.xml"), "/p/plan.png");
  EXPECT_EQ(DefaultPngFilePath(""), "");
}

TEST(PngExportConfigTest, AnEmptyFieldResolvesToTheDefaultPath) {
  const PngExportConfig config;

  EXPECT_EQ(config.ResolvedFilePath("/p/plan.xml"), "/p/plan.png");
  EXPECT_EQ(config.ResolvedFilePath(""), "");
}

TEST(PngExportConfigTest, ARelativePathResolvesAgainstTheProjectFile) {
  PngExportConfig config;
  config.SetFilePath("out/a.png");

  EXPECT_EQ(config.ResolvedFilePath("/p/plan.xml"), "/p/out/a.png");
}

TEST(PngExportConfigTest, AnAbsolutePathStaysAsWritten) {
  PngExportConfig config;
  config.SetFilePath("/abs/a.png");

  EXPECT_EQ(config.ResolvedFilePath("/p/plan.xml"), "/abs/a.png");
}

// Without a project file a relative path has no anchor but the process's
// working directory, which the user never sees: ask instead.
TEST(PngExportConfigTest, ARelativePathWithoutProjectAsks) {
  PngExportConfig config;
  config.SetFilePath("a.png");

  EXPECT_EQ(config.ResolvedFilePath(""), "");
}

// The tab offers powers of two alone; a hand-edited file must land on one.
TEST(PngExportConfigTest, MsaaSamplesRoundDownToAPowerOfTwo) {
  PngExportConfig config;

  config.SetMsaaSamples(6);
  EXPECT_EQ(config.MsaaSamples(), 4);
  config.SetMsaaSamples(3);
  EXPECT_EQ(config.MsaaSamples(), 2);
}
