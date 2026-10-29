#include <gtest/gtest.h>

#include <QtCore/QObject>

#include "domain/png_export_config.hpp"
#include "domain/png_export_config_store.hpp"
#include "domain/state_topics.hpp"

namespace {

PngExportConfig MakeConfig(int dpi, int msaa_samples) {
  PngExportConfig config;
  config.SetDpi(dpi);
  config.SetMsaaSamples(msaa_samples);
  config.SetFilePath("out/a.png");
  return config;
}

}  // namespace

TEST(PngExportConfigStoreTest, ReceiveStoresConfigAndEmitsSignal) {
  domain::PngExportConfigTopic topic;
  PngExportConfigStore store(topic);
  int emissions = 0;
  PngExportConfig captured;
  QObject::connect(&topic, &domain::PngExportConfigTopic::Published,
                   [&](const PngExportConfig& config) {
                     ++emissions;
                     captured = config;
                   });

  store.ReceivePngExportConfig(MakeConfig(300, 4));

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(captured.Dpi(), 300);
  EXPECT_EQ(captured.MsaaSamples(), 4);
  EXPECT_EQ(store.Get().FilePath(), "out/a.png");
}

TEST(PngExportConfigStoreTest, SendPngExportConfigEmitsCurrentConfig) {
  domain::PngExportConfigTopic topic;
  PngExportConfigStore store(topic);
  store.ReceivePngExportConfig(MakeConfig(150, 8));
  int emissions = 0;
  QObject::connect(&topic, &domain::PngExportConfigTopic::Published,
                   [&](const PngExportConfig&) { ++emissions; });

  store.SendPngExportConfig();
  store.SendPngExportConfig();
  EXPECT_EQ(emissions, 2);
}

TEST(PngExportConfigStoreTest, ReentryGuardBlocksRecursiveReceive) {
  domain::PngExportConfigTopic topic;
  PngExportConfigStore store(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::PngExportConfigTopic::Published,
                   [&](const PngExportConfig&) {
                     ++emissions;
                     if (emissions == 1) {
                       store.ReceivePngExportConfig(MakeConfig(72, 1));
                     }
                   });

  store.ReceivePngExportConfig(MakeConfig(300, 4));

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(store.Get().Dpi(), 300);
}
