#ifndef PNG_EXPORT_CONFIG_STORE_HPP
#define PNG_EXPORT_CONFIG_STORE_HPP

#include "png_export_config.hpp"
#include "state_topics.hpp"

// Owns a PngExportConfig value and publishes it on the injected topic.
class PngExportConfigStore {
 public:
  explicit PngExportConfigStore(domain::PngExportConfigTopic& topic);
  ~PngExportConfigStore() = default;
  PngExportConfigStore(const PngExportConfigStore&) = delete;
  PngExportConfigStore& operator=(const PngExportConfigStore&) = delete;
  PngExportConfigStore(PngExportConfigStore&&) = delete;
  PngExportConfigStore& operator=(PngExportConfigStore&&) = delete;

  void ReceivePngExportConfig(const PngExportConfig& incoming_config);

  void SendPngExportConfig();

  [[nodiscard]] const PngExportConfig& Get() const;

 private:
  PngExportConfig png_export_config_;
  domain::PngExportConfigTopic& topic_;
  bool emitting_{false};
};
#endif  // PNG_EXPORT_CONFIG_STORE_HPP
