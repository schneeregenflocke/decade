#include "locale_services.hpp"

#include <exception>
#include <iostream>
#include <locale>
#include <string>
#include <utility>

#include "../domain/date_format.hpp"

namespace application {

LocaleServices::LocaleServices(std::string locale_name)
    : locale_name_(std::move(locale_name)), date_formatter_(locale_name_) {
  Initialize();
}

LocaleDateFormatter& LocaleServices::date_formatter() {
  return date_formatter_;
}

const LocaleDateFormatter& LocaleServices::date_formatter() const {
  return date_formatter_;
}

const std::string& LocaleServices::locale_name() const { return locale_name_; }

void LocaleServices::Initialize() {
  try {
    const std::locale global_locale = locale_name_.empty()
                                          ? std::locale("")
                                          : std::locale(locale_name_.c_str());
    std::locale::global(global_locale);
  } catch (const std::exception& exception) {
    std::cerr << "failed to initialize global std locale: " << exception.what()
              << '\n';
    throw;
  }
}

}  // namespace application
