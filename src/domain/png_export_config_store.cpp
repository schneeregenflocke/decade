#include "png_export_config_store.hpp"

#include "detail/reentry_guard.hpp"
#include "png_export_config.hpp"
#include "state_topics.hpp"

PngExportConfigStore::PngExportConfigStore(domain::PngExportConfigTopic& topic)
    : topic_(topic) {}

void PngExportConfigStore::ReceivePngExportConfig(
    const PngExportConfig& incoming_config) {
  if (emitting_) {
    return;
  }
  const domain::detail::ScopedReentryFlag guard(emitting_);
  png_export_config_ = incoming_config;
  topic_.Publish(png_export_config_);
}

void PngExportConfigStore::SendPngExportConfig() {
  topic_.Publish(png_export_config_);
}

const PngExportConfig& PngExportConfigStore::Get() const {
  return png_export_config_;
}
