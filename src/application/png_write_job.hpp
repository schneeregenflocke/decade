#ifndef PNG_WRITE_JOB_HPP
#define PNG_WRITE_JOB_HPP

#include <QtCore/QObject>
#include <optional>
#include <string>
#include <thread>

#include "graphics/render_to_png.hpp"

// Compresses a rendered page into a PNG on a thread of its own, so the window
// stays responsive while a large export takes its time. libpng deflates one
// stream row after row, which leaves nothing to spread over more threads.
//
// The writer starts with the job. Its reports reach the job's thread through
// the event loop, so connecting right after construction misses none.
class PngWriteJob : public QObject {
  Q_OBJECT

 public:
  PngWriteJob(std::string file_path, RgbaImage image);

  // Stops the writer and waits for it; the unfinished file goes away.
  ~PngWriteJob() override;

  PngWriteJob(const PngWriteJob&) = delete;
  PngWriteJob& operator=(const PngWriteJob&) = delete;
  PngWriteJob(PngWriteJob&&) = delete;
  PngWriteJob& operator=(PngWriteJob&&) = delete;

  // True until Finished has gone out.
  [[nodiscard]] bool Running() const;

 signals:
  void Progressed(int percent);

  // The error is empty when the file got written.
  void Finished(const std::optional<std::string>& error);

 private:
  void Write(const std::stop_token& stop, const std::string& file_path,
             const RgbaImage& image);

  bool running_{true};
  // Last, so it joins before anything it reaches goes.
  std::jthread writer_;
};

#endif  // PNG_WRITE_JOB_HPP
