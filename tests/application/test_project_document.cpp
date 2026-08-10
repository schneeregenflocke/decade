#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <fstream>
#include <string>
#include <vector>

#include "application/event_bus.hpp"
#include "application/project_document.hpp"
#include "domain/date_format.hpp"

namespace {

// Collects what the document publishes about its file path.
struct PathRecorder {
  std::vector<std::string> published;

  explicit PathRecorder(EventBus& bus) {
    QObject::connect(
        &bus.project_file_path(), &domain::FilePathTopic::Published,
        [this](const std::string& path) { published.push_back(path); });
  }
};

std::string TempXmlPath(const std::string& name) {
  return testing::TempDir() + name;
}

// Records the burst bracket together with the store publishes it encloses, in
// the order they arrive. That order is the whole point: a consumer may only
// see the six stores inside one open bracket.
struct BurstRecorder {
  std::vector<std::string> events;

  explicit BurstRecorder(EventBus& bus) {
    QObject::connect(
        &bus.state_burst(), &domain::StateBurstTopic::Published,
        [this](bool open) { events.emplace_back(open ? "open" : "close"); });
    QObject::connect(&bus.date_groups(), &domain::DateGroupsTopic::Published,
                     [this](const std::vector<DateGroup>&) {
                       events.emplace_back("store");
                     });
    QObject::connect(&bus.date_entries(), &domain::DateEntriesTopic::Published,
                     [this](const std::vector<DateEntry>&) {
                       events.emplace_back("store");
                     });
    QObject::connect(
        &bus.page_setup(), &domain::PageSetupTopic::Published,
        [this](const PageSetupConfig&) { events.emplace_back("store"); });
    QObject::connect(
        &bus.title_config(), &domain::TitleConfigTopic::Published,
        [this](const TitleConfig&) { events.emplace_back("store"); });
    QObject::connect(
        &bus.shape_config_set(), &domain::ShapeConfigSetTopic::Published,
        [this](const ShapeConfigSet&) { events.emplace_back("store"); });
    QObject::connect(
        &bus.calendar_config(), &domain::CalendarConfigTopic::Published,
        [this](const CalendarConfig&) { events.emplace_back("store"); });
  }

  [[nodiscard]] bool EveryStoreInsideOneBracket() const {
    int depth = 0;
    int stores = 0;
    for (const std::string& event : events) {
      if (event == "open") {
        ++depth;
      } else if (event == "close") {
        --depth;
      } else if (depth != 1) {
        return false;
      } else {
        ++stores;
      }
      if (depth < 0) {
        return false;
      }
    }
    return depth == 0 && stores > 0;
  }
};

}  // namespace

// The file path has no store: loading and saving alone set it, and the display
// learns it over the topic alone.
TEST(ProjectDocumentTest, SavePublishesFilePath) {
  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);
  const PathRecorder recorder(bus);

  const std::string path = TempXmlPath("decade_document_save.xml");
  ASSERT_FALSE(document.SaveXml(path).has_value());

  EXPECT_EQ(recorder.published, std::vector<std::string>{path});
  EXPECT_EQ(document.FilePath(), path);
}

TEST(ProjectDocumentTest, LoadPublishesFilePath) {
  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);

  const std::string path = TempXmlPath("decade_document_load.xml");
  ASSERT_FALSE(document.SaveXml(path).has_value());

  const PathRecorder recorder(bus);
  ASSERT_FALSE(document.LoadXml(path).has_value());

  EXPECT_EQ(recorder.published, std::vector<std::string>{path});
}

// A failed load must misplace neither the path nor the display.
TEST(ProjectDocumentTest, FailedLoadKeepsFilePath) {
  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);

  const std::string path = TempXmlPath("decade_document_kept.xml");
  ASSERT_FALSE(document.SaveXml(path).has_value());

  const PathRecorder recorder(bus);
  ASSERT_TRUE(document.LoadXml(TempXmlPath("decade_missing.xml")).has_value());

  EXPECT_TRUE(recorder.published.empty());
  EXPECT_EQ(document.FilePath(), path);
}

TEST(ProjectDocumentTest, FreshDocumentHasNoFilePath) {
  EventBus bus;
  LocaleDateFormatter formatter;
  const application::ProjectDocument document(bus, formatter);

  EXPECT_FALSE(document.HasFilePath());
}

// Loading fills six stores one after another and every one of them publishes.
// Whoever rebuilds on that must be able to do it once, so the publishes have to
// arrive inside one bracket (#36).
TEST(ProjectDocumentTest, LoadBracketsEveryStorePublishInOneBurst) {
  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);

  const std::string path = TempXmlPath("decade_document_burst.xml");
  ASSERT_FALSE(document.SaveXml(path).has_value());

  const BurstRecorder recorder(bus);
  ASSERT_FALSE(document.LoadXml(path).has_value());

  EXPECT_TRUE(recorder.EveryStoreInsideOneBracket()) << recorder.events.size();
}

// The same for the CSV import, which publishes the entries and, over the
// transform, the derived bars. The file gets written here rather than taken
// from examples/, so the test does not hang on the working directory.
TEST(ProjectDocumentTest, CsvImportBracketsItsPublishes) {
  const std::string csv_path = TempXmlPath("decade_document_burst.csv");
  {
    std::ofstream csv(csv_path);
    ASSERT_TRUE(csv.is_open());
    csv << "23.09.1998,24.11.1998\n23.12.1998,16.01.1999\n";
  }

  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);

  const BurstRecorder recorder(bus);
  document.ImportCsv(csv_path);

  EXPECT_TRUE(recorder.EveryStoreInsideOneBracket()) << recorder.events.size();
}

// A failed load closes its bracket too — otherwise a consumer would hold its
// rebuild for the rest of the session.
TEST(ProjectDocumentTest, AFailedLoadStillClosesTheBurst) {
  EventBus bus;
  LocaleDateFormatter formatter;
  application::ProjectDocument document(bus, formatter);

  std::vector<bool> brackets;
  QObject::connect(&bus.state_burst(), &domain::StateBurstTopic::Published,
                   [&](bool open) { brackets.push_back(open); });
  ASSERT_TRUE(
      document.LoadXml(TempXmlPath("decade_no_such_file.xml")).has_value());

  EXPECT_EQ(brackets, (std::vector<bool>{true, false}));
}
