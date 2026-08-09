#ifndef LOCALE_SERVICES_HPP
#define LOCALE_SERVICES_HPP

#include <exception>
#include <iostream>
#include <locale>
#include <string>
#include <utility>

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
  explicit LocaleServices(std::string locale_name = {})
      : locale_name_(std::move(locale_name)), date_formatter_(locale_name_) {
    Initialize();
  }

  LocaleServices(const LocaleServices&) = delete;
  LocaleServices& operator=(const LocaleServices&) = delete;
  LocaleServices(LocaleServices&&) = delete;
  LocaleServices& operator=(LocaleServices&&) = delete;
  ~LocaleServices() = default;

  [[nodiscard]] LocaleDateFormatter& date_formatter() {
    return date_formatter_;
  }

  [[nodiscard]] const LocaleDateFormatter& date_formatter() const {
    return date_formatter_;
  }

  [[nodiscard]] const std::string& locale_name() const { return locale_name_; }

 private:
  void Initialize() {
    try {
      const std::locale global_locale = locale_name_.empty()
                                            ? std::locale("")
                                            : std::locale(locale_name_.c_str());
      std::locale::global(global_locale);
    } catch (const std::exception& exception) {
      std::cerr << "failed to initialize global std locale: "
                << exception.what() << '\n';
      throw;
    }
  }

  std::string locale_name_;
  LocaleDateFormatter date_formatter_;
};

}  // namespace application

#endif  // LOCALE_SERVICES_HPP
