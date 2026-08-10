#ifndef LOCALE_SERVICES_HPP
#define LOCALE_SERVICES_HPP

#include <string>

#include "../domain/date_format.hpp"

namespace application {

// The application-wide locale: the C++ global locale plus the ICU-backed date
// formatter every panel and the CSV I/O share.
//
// There is no toolkit locale object beside them any more. wxLocale had to be
// held alive because wxString conversions read it; Qt formats through QLocale,
// which asks the system itself and needs nothing kept.
class LocaleServices {
 public:
  explicit LocaleServices(std::string locale_name = {});

  LocaleServices(const LocaleServices&) = delete;
  LocaleServices& operator=(const LocaleServices&) = delete;
  LocaleServices(LocaleServices&&) = delete;
  LocaleServices& operator=(LocaleServices&&) = delete;
  ~LocaleServices() = default;

  [[nodiscard]] LocaleDateFormatter& date_formatter();

  [[nodiscard]] const LocaleDateFormatter& date_formatter() const;

  [[nodiscard]] const std::string& locale_name() const;

 private:
  void Initialize();

  std::string locale_name_;
  LocaleDateFormatter date_formatter_;
};

}  // namespace application

#endif  // LOCALE_SERVICES_HPP
