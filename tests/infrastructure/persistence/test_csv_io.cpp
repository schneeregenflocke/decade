#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "infrastructure/persistence/csv_io.hpp"
#include "domain/date.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_format.hpp"
#include "domain/date_period.hpp"

namespace {

// Fixed locale so the tests are machine-independent (dd.MM.yyyy).
LocaleDateFormatter MakeFormatter() { return LocaleDateFormatter("de_CH"); }

std::string TempCsvPath(const std::string& name) {
  return testing::TempDir() + name;
}

void WriteFile(const std::string& path, const std::string& content) {
  std::ofstream stream(path, std::ios_base::trunc);
  stream << content;
}

}  // namespace

TEST(CsvIoTest, ReadsInclusiveDatesAsHalfOpenPeriods) {
  const std::string path = TempCsvPath("decade_read.csv");
  WriteFile(path,
            "23.09.1998,24.11.1998\n"
            "01.01.2022,01.01.2022\n");

  auto formatter = MakeFormatter();
  const auto entries = persistence::ReadDateEntriesFromCsv(path, formatter);

  ASSERT_EQ(entries.size(), 2U);
  EXPECT_EQ(entries[0].GetDateInterval().Begin(), Date::FromYmd(1998, 9, 23));
  // The CSV to-date is inclusive; internally the end is exclusive.
  EXPECT_EQ(entries[0].GetDateInterval().End(), Date::FromYmd(1998, 11, 25));
  // 23.09. through 24.11. inclusive = 63 days.
  EXPECT_EQ(entries[0].GetDateInterval().LengthDays(), 63);
  // A same-day row is a single-day period, not a null period.
  EXPECT_EQ(entries[1].GetDateInterval().LengthDays(), 1);
}

TEST(CsvIoTest, UnparseableRowsYieldNullPeriods) {
  const std::string path = TempCsvPath("decade_invalid.csv");
  WriteFile(path,
            "not a date,24.11.1998\n"
            "23.09.1998,24.11.1998\n");

  auto formatter = MakeFormatter();
  const auto entries = persistence::ReadDateEntriesFromCsv(path, formatter);

  // The reader keeps row count; the store filters null periods later.
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_TRUE(entries[0].GetDateInterval().IsNull());
  EXPECT_FALSE(entries[1].GetDateInterval().IsNull());
}

TEST(CsvIoTest, BlankLinesAreSkipped) {
  const std::string path = TempCsvPath("decade_blank_lines.csv");
  // A leading blank line and blank lines between rows are not entries.
  WriteFile(path,
            "\n"
            "01.01.1992,03.05.1993\n"
            "\n"
            "03.05.1994,05.05.1994\n"
            "\n"
            "\n"
            "01.03.1996,05.08.1997\n");

  auto formatter = MakeFormatter();
  const auto entries = persistence::ReadDateEntriesFromCsv(path, formatter);

  ASSERT_EQ(entries.size(), 3U);
  for (const auto& entry : entries) {
    EXPECT_FALSE(entry.GetDateInterval().IsNull());
  }
}

TEST(CsvIoTest, SingleColumnRowsYieldSingleDayPeriods) {
  const std::string path = TempCsvPath("decade_single_column.csv");
  WriteFile(path,
            "02.05.2001\n"
            "01.03.1996,05.08.1997\n"
            "text\n");

  auto formatter = MakeFormatter();
  const auto entries = persistence::ReadDateEntriesFromCsv(path, formatter);

  ASSERT_EQ(entries.size(), 3U);
  // A missing to-date means "same as from-date": a single-day period.
  EXPECT_EQ(entries[0].GetDateInterval().Begin(), Date::FromYmd(2001, 5, 2));
  EXPECT_EQ(entries[0].GetDateInterval().LengthDays(), 1);
  EXPECT_FALSE(entries[1].GetDateInterval().IsNull());
  // Without a parseable from-date the row is a null period.
  EXPECT_TRUE(entries[2].GetDateInterval().IsNull());
}

TEST(CsvIoTest, MissingFileYieldsNoEntries) {
  auto formatter = MakeFormatter();
  EXPECT_TRUE(
      persistence::ReadDateEntriesFromCsv("/nonexistent/decade.csv", formatter)
          .empty());
}

TEST(CsvIoTest, WriteReadRoundTripPreservesPeriods) {
  const std::string path = TempCsvPath("decade_roundtrip.csv");

  std::vector<DateEntry> entries(2);
  entries[0].SetDateInterval(
      DatePeriod(Date::FromYmd(2030, 6, 10), Date::FromYmd(2030, 6, 21)));
  entries[1].SetDateInterval(
      DatePeriod(Date::FromYmd(2022, 1, 1), Date::FromYmd(2022, 1, 2)));

  auto formatter = MakeFormatter();
  persistence::WriteDateEntriesToCsv(path, entries, formatter);
  const auto loaded = persistence::ReadDateEntriesFromCsv(path, formatter);

  ASSERT_EQ(loaded.size(), entries.size());
  for (size_t index = 0; index < entries.size(); ++index) {
    EXPECT_EQ(loaded[index].GetDateInterval(), entries[index].GetDateInterval())
        << "row " << index;
  }
}
