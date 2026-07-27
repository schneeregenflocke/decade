#include <gtest/gtest.h>

#include "domain/state_topic.hpp"
#include "domain/title_config.hpp"
#include "domain/title_config_store.hpp"

TEST(TitleConfigTest, GettersReturnSetValues) {
  TitleConfig config;
  config.SetFrameHeight(42.0F);
  config.SetFontSizePoints(36.0F);
  config.SetTitleText("hello");
  config.SetTextColor({0.1F, 0.2F, 0.3F, 0.4F});

  EXPECT_FLOAT_EQ(config.FrameHeight(), 42.0F);
  EXPECT_FLOAT_EQ(config.FontSizePoints(), 36.0F);
  EXPECT_FLOAT_EQ(config.FontSizeMillimetres(), 36.0F * 25.4F / 72.0F);
  EXPECT_EQ(config.TitleText(), "hello");
  EXPECT_FLOAT_EQ(config.TextColor()[0], 0.1F);
  EXPECT_FLOAT_EQ(config.TextColor()[3], 0.4F);
}

TEST(TitleConfigStoreTest, ReceiveStoresAndEmits) {
  domain::StateTopic<TitleConfig> topic;
  TitleConfigStore store(topic);
  TitleConfig incoming;
  incoming.SetTitleText("incoming");
  incoming.SetFrameHeight(20.0F);

  int emissions = 0;
  std::string observed_text;
  topic.connect([&](const TitleConfig& cfg) {
    ++emissions;
    observed_text = cfg.TitleText();
  });

  store.ReceiveTitleConfig(incoming);

  EXPECT_EQ(emissions, 1);
  EXPECT_EQ(observed_text, "incoming");
}

TEST(TitleConfigStoreTest, ReentryGuardBlocksRecursiveReceive) {
  domain::StateTopic<TitleConfig> topic;
  TitleConfigStore store(topic);
  int emissions = 0;
  topic.connect([&](const TitleConfig&) {
    ++emissions;
    if (emissions == 1) {
      TitleConfig recursive;
      recursive.SetTitleText("recursive");
      store.ReceiveTitleConfig(recursive);
    }
  });

  TitleConfig first;
  first.SetTitleText("first");
  store.ReceiveTitleConfig(first);

  EXPECT_EQ(emissions, 1);
}
