#include "bar.hpp"

#include <string>

#include "date_period.hpp"

Bar::Bar(const DatePeriod& date_interval) : date_interval_(date_interval) {}

void Bar::SetText(const std::string& text) { text_ = text; }

const std::string& Bar::GetText() const { return text_; }

const DatePeriod& Bar::Period() const { return date_interval_; }

int Bar::GetCategory() const { return category_; }

void Bar::SetCategory(int category) { category_ = category; }
