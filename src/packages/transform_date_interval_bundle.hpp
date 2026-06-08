#ifndef TRANSFORM_DATE_INTERVAL_BUNDLE_HPP
#define TRANSFORM_DATE_INTERVAL_BUNDLE_HPP

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <sigslot/signal.hpp>
#include <vector>

#include "date_interval_bundle.hpp"
#include "detail/reentry_guard.hpp"

class TransformDateIntervalBundle {
 public:
  struct DateShift {
    int begin_days;
    int end_days;
  };

  TransformDateIntervalBundle()
      : date_shift_{.begin_days = 0, .end_days = 0} {};

  void ReceiveDateIntervalBundles(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    if (emitting_) {
      return;
    }
    const packages::detail::ScopedReentryFlag guard(emitting_);
    std::vector<DateIntervalBundle> transformed_bundles = date_interval_bundles;
    auto bundles_iterator = transformed_bundles.begin();

    for (const auto& date_interval_bundle : date_interval_bundles) {
      const auto& interval = date_interval_bundle.GetDateInterval();
      bundles_iterator->SetDateInterval(boost::gregorian::date_period(
          interval.begin() +
              boost::gregorian::date_duration(date_shift_.begin_days),
          interval.end() +
              boost::gregorian::date_duration(date_shift_.end_days)));
      ++bundles_iterator;
    }

    signal_transform_date_interval_bundles_(transformed_bundles);
  }

  void InputTransformedDateIntervals(
      const std::vector<DateIntervalBundle>& date_interval_bundles) {
    std::vector<DateIntervalBundle> untransformed_bundles =
        date_interval_bundles;
    auto bundles_iterator = untransformed_bundles.begin();

    for (const auto& date_interval_bundle : date_interval_bundles) {
      const auto& interval = date_interval_bundle.GetDateInterval();
      bundles_iterator->SetDateInterval(boost::gregorian::date_period(
          interval.begin() -
              boost::gregorian::date_duration(date_shift_.begin_days),
          interval.end() -
              boost::gregorian::date_duration(date_shift_.end_days)));
      ++bundles_iterator;
    }

    signal_date_interval_bundles_(untransformed_bundles);
  }

  void SetTransform(DateShift shift) { date_shift_ = shift; }

  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalDateIntervalBundles() {
    return signal_date_interval_bundles_;
  }
  sigslot::signal<const std::vector<DateIntervalBundle>&>&
  SignalTransformDateIntervalBundles() {
    return signal_transform_date_interval_bundles_;
  }

 private:
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_date_interval_bundles_;
  sigslot::signal<const std::vector<DateIntervalBundle>&>
      signal_transform_date_interval_bundles_;
  DateShift date_shift_;
  bool emitting_{false};
};
#endif  // TRANSFORM_DATE_INTERVAL_BUNDLE_HPP
