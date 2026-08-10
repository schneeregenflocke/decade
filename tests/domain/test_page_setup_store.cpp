#include <gtest/gtest.h>

#include <QtCore/QObject>

#include "domain/page_setup_config.hpp"
#include "domain/page_setup_store.hpp"
#include "domain/state_topics.hpp"

namespace {

PageSetupConfig MakeConfig(float width, float height, int orientation) {
  PageSetupConfig config;
  config.SetSize({width, height});
  config.SetMargins({1.0F, 2.0F, 3.0F, 4.0F});
  config.SetOrientation(orientation);
  return config;
}

}  // namespace

TEST(PageSetupStoreTest, ReceiveStoresConfigAndEmitsSignal) {
  domain::PageSetupTopic topic;
  PageSetupStore store(topic);
  int emissions = 0;
  PageSetupConfig captured{};
  QObject::connect(&topic, &domain::PageSetupTopic::Published,
                   [&](const PageSetupConfig& config) {
                     ++emissions;
                     captured = config;
                   });

  const auto config = MakeConfig(210.0F, 297.0F, 0);
  store.ReceivePageSetup(config);

  EXPECT_EQ(emissions, 1);
  EXPECT_FLOAT_EQ(captured.Size()[0], 210.0F);
  EXPECT_FLOAT_EQ(captured.Size()[1], 297.0F);
  EXPECT_EQ(captured.Orientation(), 0);
  EXPECT_FLOAT_EQ(store.Get().Margins()[2], 3.0F);
}

TEST(PageSetupStoreTest, SendPageSetupEmitsCurrentConfig) {
  domain::PageSetupTopic topic;
  PageSetupStore store(topic);
  store.ReceivePageSetup(MakeConfig(100.0F, 100.0F, 1));
  int emissions = 0;
  QObject::connect(&topic, &domain::PageSetupTopic::Published,
                   [&](const PageSetupConfig&) { ++emissions; });

  store.SendPageSetup();
  store.SendPageSetup();
  EXPECT_EQ(emissions, 2);
}

TEST(PageSetupStoreTest, ReentryGuardBlocksRecursiveReceive) {
  domain::PageSetupTopic topic;
  PageSetupStore store(topic);
  int emissions = 0;
  QObject::connect(&topic, &domain::PageSetupTopic::Published,
                   [&](const PageSetupConfig&) {
                     ++emissions;
                     if (emissions == 1) {
                       store.ReceivePageSetup(MakeConfig(0.0F, 0.0F, 9));
                     }
                   });

  store.ReceivePageSetup(MakeConfig(210.0F, 297.0F, 0));

  EXPECT_EQ(emissions, 1);
  EXPECT_FLOAT_EQ(store.Get().Size()[0], 210.0F);
  EXPECT_EQ(store.Get().Orientation(), 0);
}
