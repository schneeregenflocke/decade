#include "png_export_config.hpp"

#include <algorithm>
#include <bit>
#include <filesystem>
#include <string>
#include <utility>

int PngExportConfig::Dpi() const { return dpi_; }

int PngExportConfig::MsaaSamples() const { return msaa_samples_; }

const std::string& PngExportConfig::FilePath() const { return file_path_; }

void PngExportConfig::SetDpi(int dpi) {
  dpi_ = std::clamp(dpi, kMinDpi, kMaxDpi);
}

void PngExportConfig::SetMsaaSamples(int msaa_samples) {
  msaa_samples_ = static_cast<int>(std::bit_floor(
      static_cast<unsigned>(std::clamp(msaa_samples, 1, kMaxMsaaSamples))));
}

void PngExportConfig::SetFilePath(std::string file_path) {
  file_path_ = std::move(file_path);
}

std::string PngExportConfig::ResolvedFilePath(
    const std::string& project_file_path) const {
  if (file_path_.empty()) {
    return DefaultPngFilePath(project_file_path);
  }
  const std::filesystem::path written(file_path_);
  if (written.is_absolute() || project_file_path.empty()) {
    return file_path_;
  }
  return (std::filesystem::path(project_file_path).parent_path() / written)
      .string();
}

std::string DefaultPngFilePath(const std::string& project_file_path) {
  if (project_file_path.empty()) {
    return {};
  }
  return std::filesystem::path(project_file_path)
      .replace_extension(".png")
      .string();
}
