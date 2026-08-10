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
#include "domain/state_topics.hpp"
#include "domain/title_config_store.hpp"
#include "infrastructure/persistence/project_io.hpp"

namespace {

// Bundles the six stores Load and Save demand together with their topics —
// every store publishes on an injected channel. The topics are declared first,
// so they outlive the stores.
struct ProjectStores {
  domain::DateGroupsTopic date_groups_topic;
  domain::DateEntriesTopic date_entries_topic;
  domain::PageSetupTopic page_setup_topic;
  domain::TitleConfigTopic title_config_topic;
  domain::ShapeConfigSetTopic shape_configuration_topic;
  domain::CalendarConfigTopic calendar_configuration_topic;

  DateGroupStore date_groups{date_groups_topic};
  DateEntryStore date_entries{date_entries_topic};
  PageSetupStore page_setup{page_setup_topic};
  TitleConfigStore title_config{title_config_topic};
  ShapeConfigurationStore shape_configuration{shape_configuration_topic};
  CalendarConfigStore calendar_configuration{calendar_configuration_topic};
};

std::optional<std::string> Load(const std::string& path, ProjectStores& s) {
  return persistence::LoadProjectXml(
      path, s.date_groups, s.date_entries, s.page_setup, s.title_config,
      s.shape_configuration, s.calendar_configuration);
}

std::optional<std::string> Save(const std::string& path,
                                const ProjectStores& s) {
  return persistence::SaveProjectXml(
      path, s.date_groups, s.date_entries, s.page_setup, s.title_config,
      s.shape_configuration, s.calendar_configuration);
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

// Regression: a broken project file (or the old format deliberately no longer
// readable) must neither throw an exception into the caller nor half-overwrite
// the stores — the error gets reported, the state stays.
TEST(ProjectIoTest, CorruptFileReportsErrorAndLeavesStoresUntouched) {
  ProjectStores stores;
  SeedProject(stores);
  const std::string path = TempXmlPath("decade_corrupt.xml");
  WriteFile(path, "this is not a boost serialization archive");

  const auto error = Load(path, stores);

  ASSERT_TRUE(error.has_value());
  ASSERT_EQ(stores.date_entries.Get().Items().size(), 1U);
  EXPECT_EQ(stores.date_groups.Get().Items().size(), 1U);
  EXPECT_EQ(stores.date_groups.Get().Items()[0].GetName(), "Seeded");
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
  ASSERT_EQ(target.date_entries.Get().Items().size(), 1U);
  EXPECT_EQ(target.date_entries.Get().Items()[0].GetDateInterval().Begin(),
            Date::FromYmd(2030, 1, 1));
}
