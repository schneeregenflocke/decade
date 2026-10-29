#ifndef PNG_EXPORT_CONFIG_HPP
#define PNG_EXPORT_CONFIG_HPP

#include <string>

// How the page gets exported as a PNG: resolution, multisampling and target.
// The file path stays as the user wrote it, empty or relative included; only
// ResolvedFilePath turns it into a place on disk.
class PngExportConfig {
 public:
  static constexpr int kMinDpi = 1;
  static constexpr int kMaxDpi = 2400;
  static constexpr int kDefaultDpi = 200;
  static constexpr int kMaxMsaaSamples = 16;
  static constexpr int kDefaultMsaaSamples = 16;

  [[nodiscard]] int Dpi() const;
  [[nodiscard]] int MsaaSamples() const;
  [[nodiscard]] const std::string& FilePath() const;

  void SetDpi(int dpi);
  void SetMsaaSamples(int msaa_samples);
  void SetFilePath(std::string file_path);

  // A relative path counts from the project file's directory. Empty when
  // neither the path nor the project file names a place: then ask.
  [[nodiscard]] std::string ResolvedFilePath(
      const std::string& project_file_path) const;

 private:
  int dpi_{kDefaultDpi};
  int msaa_samples_{kDefaultMsaaSamples};
  std::string file_path_;
};

// `<project stem>.png` beside the project file; empty without one.
[[nodiscard]] std::string DefaultPngFilePath(
    const std::string& project_file_path);

#endif  // PNG_EXPORT_CONFIG_HPP
