#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "app/services/csv_io.hpp"
#include "packages/date.hpp"
#include "packages/date_entry.hpp"
#include "packages/date_format.hpp"
#include "packages/date_period.hpp"

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
  const auto entries = app::io::ReadDateEntriesFromCsv(path, formatter);

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
  const auto entries = app::io::ReadDateEntriesFromCsv(path, formatter);

  // The reader keeps row count; the store filters null periods later.
  ASSERT_EQ(entries.size(), 2U);
  EXPECT_TRUE(entries[0].GetDateInterval().IsNull());
  EXPECT_FALSE(entries[1].GetDateInterval().IsNull());
}

TEST(CsvIoTest, MissingFileYieldsNoEntries) {
  auto formatter = MakeFormatter();
  EXPECT_TRUE(
      app::io::ReadDateEntriesFromCsv("/nonexistent/decade.csv", formatter)
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
  app::io::WriteDateEntriesToCsv(path, entries, formatter);
  const auto loaded = app::io::ReadDateEntriesFromCsv(path, formatter);

  ASSERT_EQ(loaded.size(), entries.size());
  for (size_t index = 0; index < entries.size(); ++index) {
    EXPECT_EQ(loaded[index].GetDateInterval(), entries[index].GetDateInterval())
        << "row " << index;
  }
}
