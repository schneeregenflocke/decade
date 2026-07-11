#ifndef LOCALE_SERVICES_HPP
#define LOCALE_SERVICES_HPP

#include <wx/intl.h>

#include <exception>
#include <iostream>
#include <locale>
#include <string>

#include "../domain/date_format.hpp"

namespace application {

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

  [[nodiscard]] wxLocale& wx_locale() { return locale_; }

  [[nodiscard]] const wxLocale& wx_locale() const { return locale_; }

  [[nodiscard]] const std::string& locale_name() const { return locale_name_; }

 private:
  void Initialize() {
    if (locale_name_.empty()) {
      if (!locale_.Init()) {
        throw std::runtime_error("failed to initialize wx locale");
      }
    } else {
      if (!locale_.Init(locale_name_)) {
        throw std::runtime_error("failed to initialize wx locale");
      }
    }

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
  wxLocale locale_;
  LocaleDateFormatter date_formatter_;
};

}  // namespace application

#endif  // LOCALE_SERVICES_HPP