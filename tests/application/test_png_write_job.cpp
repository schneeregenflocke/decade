#include <gtest/gtest.h>

#include <QtCore/QObject>
#include <QtTest/QTest>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "application/png_write_job.hpp"
#include "graphics/render_to_png.hpp"

namespace {

constexpr int kWaitMs = 10000;

std::string TempPngPath(const std::string& name) {
  return testing::TempDir() + name;
}

RgbaImage BlankImage(std::size_t width, std::size_t height) {
  return RgbaImage{
      .size = {.width = width, .height = height},
      .pixels = std::vector<unsigned char>(width * height * 4, 0xFF),
  };
}

// Records what a job reports, so a test can wait for the end and look back.
struct JobLog {
  explicit JobLog(PngWriteJob& job) {
    QObject::connect(&job, &PngWriteJob::Progressed,
                     [this](int percent) { progress.push_back(percent); });
    QObject::connect(&job, &PngWriteJob::Finished,
                     [this](const std::optional<std::string>& error) {
                       finished = true;
                       result = error;
                     });
  }
  std::vector<int> progress;
  bool finished = false;
  std::optional<std::string> result;
};

}  // namespace

TEST(PngWriteJobTest, WritesTheFileAndCountsUpToHundred) {
  const std::string file = TempPngPath("png_write_job_written.png");
  PngWriteJob job(file, BlankImage(4, 200));
  JobLog log(job);

  ASSERT_TRUE(QTest::qWaitFor([&log] { return log.finished; }, kWaitMs));

  EXPECT_EQ(log.result, std::nullopt);
  EXPECT_FALSE(job.Running());
  ASSERT_FALSE(log.progress.empty());
  EXPECT_TRUE(std::ranges::is_sorted(log.progress));
  EXPECT_EQ(log.progress.back(), 100);
  EXPECT_TRUE(std::filesystem::exists(file));
  std::filesystem::remove(file);
}

TEST(PngWriteJobTest, RunsUntilItFinishes) {
  const std::string file = TempPngPath("png_write_job_running.png");
  PngWriteJob job(file, BlankImage(4, 4));
  JobLog log(job);

  EXPECT_TRUE(job.Running());
  ASSERT_TRUE(QTest::qWaitFor([&log] { return log.finished; }, kWaitMs));
  std::filesystem::remove(file);
}

TEST(PngWriteJobTest, ReportsAFileItCannotWrite) {
  PngWriteJob job(TempPngPath("missing-directory/a.png"), BlankImage(4, 4));
  JobLog log(job);

  ASSERT_TRUE(QTest::qWaitFor([&log] { return log.finished; }, kWaitMs));

  EXPECT_NE(log.result, std::nullopt);
}

// Closing the window mid-export stops the writer rather than waiting for it,
// and leaves no half image behind.
TEST(PngWriteJobTest, DestroyingARunningJobLeavesNoFile) {
  const std::string file = TempPngPath("png_write_job_stopped.png");
  {
    const PngWriteJob job(file, BlankImage(4000, 4000));
  }

  EXPECT_FALSE(std::filesystem::exists(file));
}
