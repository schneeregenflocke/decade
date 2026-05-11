#ifndef HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_TITLE_CONFIG_HPP
#define HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_TITLE_CONFIG_HPP

#include <array>
#include <string>
#include <utility>

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>

class TitleConfig {
public:
  TitleConfig() = default;

  [[nodiscard]] float FrameHeight() const { return frame_height; }
  void SetFrameHeight(float value) { frame_height = value; }

  [[nodiscard]] float FontSizeRatio() const { return font_size_ratio; }
  void SetFontSizeRatio(float value) { font_size_ratio = value; }

  [[nodiscard]] const std::string &TitleText() const { return title_text; }
  void SetTitleText(std::string value) { title_text = std::move(value); }

  [[nodiscard]] const std::array<float, 4> &TextColor() const { return text_color; }
  void SetTextColor(const std::array<float, 4> &value) { text_color = value; }

private:
  static constexpr float kDefaultFrameHeight = 10.0F;
  static constexpr float kDefaultFontSizeRatio = 1.0F;
  static constexpr float kDefaultTextColor = 0.0F;
  static constexpr float kDefaultTextAlpha = 1.0F;

  float frame_height{kDefaultFrameHeight};
  float font_size_ratio{kDefaultFontSizeRatio};
  std::string title_text{"title config constructor text"};
  std::array<float, 4> text_color{kDefaultTextColor, kDefaultTextColor, kDefaultTextColor,
                                  kDefaultTextAlpha};

  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &archive, const unsigned int version)
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(frame_height);
    archive &BOOST_SERIALIZATION_NVP(font_size_ratio);
    archive &BOOST_SERIALIZATION_NVP(title_text);
    archive &BOOST_SERIALIZATION_NVP(text_color);
  }
};

class TitleConfigStore {
public:
  void SendTitleConfig() { signal_title_config(title_config); }

  void ReceiveTitleConfig(const TitleConfig &incoming_title_config)
  {
    title_config = incoming_title_config;
    SendTitleConfig();
  }

  sigslot::signal<const TitleConfig &> &SignalTitleConfig() { return signal_title_config; }

private:
  TitleConfig title_config;
  sigslot::signal<const TitleConfig &> signal_title_config;

  friend class boost::serialization::access;
  template <class Archive> void save(Archive &archive, const unsigned int version) const
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(title_config);
  }
  template <class Archive> void load(Archive &archive, const unsigned int version)
  {
    (void)version;
    archive &BOOST_SERIALIZATION_NVP(title_config);
    signal_title_config(title_config);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
#endif // HOME_TITAN99_CODE_DECADE_SRC_PACKAGES_TITLE_CONFIG_HPP
