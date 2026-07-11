#include <gtest/gtest.h>

#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "domain/calendar_config_store.hpp"
#include "domain/date.hpp"
#include "domain/date_entry.hpp"
#include "domain/date_entry_store.hpp"
#include "domain/date_group.hpp"
#include "domain/date_group_store.hpp"
#include "domain/date_period.hpp"
#include "domain/page_setup_store.hpp"
#include "domain/shape_configuration_store.hpp"
#include "domain/title_config_store.hpp"
#include "infrastructure/persistence/project_io.hpp"

namespace {

// Bündelt die sechs Stores, die Load/Save verlangen, damit die Tests lesbar
// bleiben.
struct ProjectStores {
  DateGroupStore date_groups;
  DateEntryStore date_entries;
  PageSetupStore page_setup;
  TitleConfigStore title_config;
  ShapeConfigurationStore shape_configuration;
  CalendarConfigStore calendar_configuration;
};

std::optional<std::string> Load(const std::string& path, ProjectStores& s) {
  return persistence::LoadProjectXml(path, s.date_groups, s.date_entries,
                                     s.page_setup, s.title_config,
                                     s.shape_configuration,
                                     s.calendar_configuration);
}

std::optional<std::string> Save(const std::string& path,
                                const ProjectStores& s) {
  return persistence::SaveProjectXml(path, s.date_groups, s.date_entries,
                                     s.page_setup, s.title_config,
                                     s.shape_configuration,
                                     s.calendar_configuration);
}

void SeedProject(ProjectStores& s) {
  std::vector<DateGroup> groups;
  groups.emplace_back("Seeded");
  s.date_groups.ReceiveDateGroups(groups);
  s.date_entries.ReceiveDateGroups(groups);

  DateEntry entry;
  entry.SetDateInterval(
      DatePeriod(Date::FromYmd(2030, 1, 1), Date::FromYmd(2030, 1, 10)));
  std::vector<DateEntry> entries;
  entries.push_back(entry);
  s.date_entries.ReceiveDateEntries(entries);
}

std::string TempXmlPath(const std::string& name) {
  return testing::TempDir() + name;
}

void WriteFile(const std::string& path, const std::string& content) {
  std::ofstream stream(path, std::ios_base::trunc);
  stream << content;
}

}  // namespace

// Regression: Eine kaputte Projektdatei (oder das alte, bewusst nicht mehr
// lesbare Format) darf weder eine Exception in den Aufrufer werfen noch die
// Stores halb überschreiben — der Fehler wird gemeldet, der Zustand bleibt.
TEST(ProjectIoTest, CorruptFileReportsErrorAndLeavesStoresUntouched) {
  ProjectStores stores;
  SeedProject(stores);
  const std::string path = TempXmlPath("decade_corrupt.xml");
  WriteFile(path, "this is not a boost serialization archive");

  const auto error = Load(path, stores);

  ASSERT_TRUE(error.has_value());
  ASSERT_EQ(stores.date_entries.GetDateEntries().size(), 1U);
  EXPECT_EQ(stores.date_groups.GetDateGroups().size(), 1U);
  EXPECT_EQ(stores.date_groups.GetDateGroups()[0].GetName(), "Seeded");
}

TEST(ProjectIoTest, MissingFileReportsError) {
  ProjectStores stores;
  const auto error = Load(TempXmlPath("decade_does_not_exist.xml"), stores);
  EXPECT_TRUE(error.has_value());
}

TEST(ProjectIoTest, UnwritablePathReportsError) {
  ProjectStores stores;
  SeedProject(stores);
  const auto error = Save("/nonexistent-dir/decade_out.xml", stores);
  EXPECT_TRUE(error.has_value());
}

TEST(ProjectIoTest, RoundTripSucceedsWithoutError) {
  ProjectStores source;
  SeedProject(source);
  const std::string path = TempXmlPath("decade_roundtrip.xml");

  ASSERT_FALSE(Save(path, source).has_value());

  ProjectStores target;
  ASSERT_FALSE(Load(path, target).has_value());
  ASSERT_EQ(target.date_entries.GetDateEntries().size(), 1U);
  EXPECT_EQ(target.date_entries.GetDateEntries()[0].GetDateInterval().Begin(),
            Date::FromYmd(2030, 1, 1));
}
