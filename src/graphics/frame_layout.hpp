#ifndef FRAME_LAYOUT_HPP
#define FRAME_LAYOUT_HPP

#include <iterator>
#include <numeric>
#include <vector>

#include "rect.hpp"

class ProportionFrameLayout {
 public:
  void SetupRowFrames(const rectf& frame, const size_t number_rows) {
    row_frames_.resize(number_rows);

    const auto row_height = frame.height() / static_cast<float>(number_rows);

    for (size_t index = 0; index < row_frames_.size(); ++index) {
      const auto float_index = static_cast<float>(index);
      const auto current_bottom = frame.b() + (float_index * row_height);
      row_frames_.at(index) = rectf(frame.l(), frame.r(), current_bottom,
                                    current_bottom + row_height);
    }
  }

  // The proportions alternate gap / subrow / … / gap, so a valid set has an
  // odd length 2k+1 yielding k subrows. The guard keeps a degenerate (empty)
  // set from underflowing the unsigned subtraction.
  void SetupSubFrames(const std::vector<float>& proportions) {
    number_sub_frames_per_row_ =
        proportions.empty() ? 0 : (proportions.size() - 1) / 2;

    sub_frames_.resize(row_frames_.size() * number_sub_frames_per_row_);

    for (size_t index = 0; index < row_frames_.size(); ++index) {
      const auto sections =
          Section(proportions, row_frames_.at(index).height());
      std::vector<float> cumulative_sections(sections.size());

      for (size_t subindex = 0; subindex < cumulative_sections.size();
           ++subindex) {
        cumulative_sections.at(subindex) = std::accumulate(
            sections.cbegin(),
            std::next(sections.cbegin(), static_cast<std::ptrdiff_t>(subindex)),
            0.0F);
      }

      for (size_t subindex = 0; subindex < number_sub_frames_per_row_;
           ++subindex) {
        const auto frame_index =
            (index * number_sub_frames_per_row_) + subindex;
        sub_frames_.at(frame_index).setL(row_frames_.at(index).l());
        sub_frames_.at(frame_index).setR(row_frames_.at(index).r());
        sub_frames_.at(frame_index)
            .setB(row_frames_.at(index).b() +
                  cumulative_sections.at((subindex * 2) + 1));
        sub_frames_.at(frame_index)
            .setT(row_frames_.at(index).b() +
                  cumulative_sections.at((subindex * 2) + 2));
      }
    }
  }

  [[nodiscard]] rectf GetSubFrame(const size_t row, const size_t sub) const {
    return sub_frames_.at((number_sub_frames_per_row_ * row) + sub);
  }

 private:
  [[nodiscard]] static std::vector<float> Section(
      const std::vector<float>& proportions, float value) {
    const auto sum =
        std::accumulate(proportions.cbegin(), proportions.cend(), 0.0F);

    std::vector<float> sections;
    sections.reserve(proportions.size());

    for (const auto& proportion : proportions) {
      sections.push_back((proportion / sum) * value);
    }

    return sections;
  }

  std::vector<rectf> row_frames_;
  std::vector<rectf> sub_frames_;
  size_t number_sub_frames_per_row_{0};
};
#endif  // FRAME_LAYOUT_HPP
