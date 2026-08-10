#include "decade_app.hpp"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QtLogging>
#include <QtCore/QtMessageHandler>
#include <QtGui/QSurfaceFormat>
#include <QtWidgets/QApplication>
#include <exception>
#include <iostream>
#include <memory>

#include "../common/debug_log.hpp"
#include "../presentation/gl_canvas.hpp"
#include "app_composition.hpp"
#include "locale_services.hpp"
#include "runtime_info.hpp"
#include "runtime_options.hpp"

namespace application {

namespace {

// Holds the two channels of operations.md: a run without --debug-log says
// nothing on stdout and puts on stderr only what the user has to act on. Qt
// speaks up through qDebug and qInfo about matters that are diagnosis, not
// news, so those stay silent unless the switch is on. A warning is never
// swallowed — it is the category that means "act".
void MessageHandler(QtMsgType type, const QMessageLogContext& /*unused*/,
                    const QString& message) {
  if ((type == QtDebugMsg || type == QtInfoMsg) &&
      !decade_debug::LogEnabled()) {
    return;
  }
  std::cerr << message.toStdString() << '\n';
}

}  // namespace

int RunDecadeApp(int& argument_count, char** arguments) {
  QSurfaceFormat::setDefaultFormat(GLCanvas::SurfaceFormat());

  const QApplication app(argument_count, arguments);
  QCoreApplication::setApplicationName("decade");

  QCommandLineParser parser;
  parser.setApplicationDescription(
      "A calendar and timeline for periods across several years.");
  parser.addHelpOption();
  AddRuntimeOptions(parser);
  parser.process(app);

  const RuntimeOptions runtime_options = RuntimeOptionsFromParser(parser);
  decade_debug::SetLogEnabled(runtime_options.debug_log);
  qInstallMessageHandler(MessageHandler);

  std::unique_ptr<LocaleServices> locale_services;
  try {
    locale_services = std::make_unique<LocaleServices>();
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }

  if (decade_debug::LogEnabled()) {
    PrintRuntimeInfo(std::cout);
  }

  const AppComposition composition(locale_services->date_formatter(),
                                   runtime_options);
  return QApplication::exec();
}
}  // namespace application
