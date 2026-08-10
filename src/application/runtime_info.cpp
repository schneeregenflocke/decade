#include "runtime_info.hpp"

#include <QtCore/QString>
#include <QtCore/QSysInfo>
#include <QtCore/QtVersion>
#include <ostream>
#include <string>

namespace application {

void PrintRuntimeInfo(std::ostream& out) {
  out << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
  out << "ProductType " << QSysInfo::productType().toStdString() << '\n';
  out << "ProductVersion " << QSysInfo::productVersion().toStdString() << '\n';
  out << "KernelType " << QSysInfo::kernelType().toStdString() << ' '
      << QSysInfo::kernelVersion().toStdString() << '\n';
  out << "BuildAbi " << QSysInfo::buildAbi().toStdString() << '\n';
  out << "QT_VERSION_STR " << QT_VERSION_STR << '\n';
}
}  // namespace application
