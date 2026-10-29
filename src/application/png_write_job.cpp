#include "png_write_job.hpp"

#include <QtCore/qnamespace.h>
#include <QtCore/qtmetamacros.h>

#include <QtCore/QMetaObject>
#include <cstddef>
#include <optional>
#include <stop_token>
#include <string>
#include <utility>

#include "graphics/render_to_png.hpp"

PngWriteJob::PngWriteJob(std::string file_path, RgbaImage image)
    // The thread owns the image, so its memory goes the moment the file is
    // written rather than with the job.
    : writer_([this, file_path = std::move(file_path),
               image = std::move(image)](const std::stop_token& stop) {
        Write(stop, file_path, image);
      }) {}

PngWriteJob::~PngWriteJob() {
  writer_.request_stop();
  writer_.join();
}

bool PngWriteJob::Running() const { return running_; }

void PngWriteJob::Write(const std::stop_token& stop,
                        const std::string& file_path, const RgbaImage& image) {
  constexpr std::size_t kHundred = 100;
  int reported = 0;
  const std::optional<std::string> error =
      WritePng(file_path, image, [&](std::size_t rows_written) {
        const auto percent =
            static_cast<int>(rows_written * kHundred / image.size.height);
        // One report per percent: a report per row would flood the event loop
        // with thousands of repaints.
        if (percent != reported) {
          reported = percent;
          QMetaObject::invokeMethod(
              this, [this, percent] { emit Progressed(percent); },
              Qt::QueuedConnection);
        }
        return !stop.stop_requested();
      });
  QMetaObject::invokeMethod(
      this,
      [this, error] {
        running_ = false;
        emit Finished(error);
      },
      Qt::QueuedConnection);
}
