#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "application/event_bus.hpp"
#include "application/project_document.hpp"
#include "domain/date_format.hpp"

namespace {

// Sammelt, was das Dokument über seinen Dateipfad veröffentlicht.
struct PathRecorder {
  std::vector<std::string> published;

  explicit PathRecorder(EventBus& bus) {
    bus.project_file_path().connect(
        [this](const std::string& path) { published.push_back(path); });
  }
};

std::string TempXmlPath(const std::string& name) {
  return testing::TempDir() + name;
}

}  // namespace

// Der Dateipfad hat keinen Store: nur Laden und Speichern setzen ihn, und die
// Anzeige erfährt ihn allein über das Topic.
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

// Ein gescheitertes Laden darf weder den Pfad noch die Anzeige verstellen.
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
