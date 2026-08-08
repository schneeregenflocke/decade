#ifndef RUNTIME_INFO_HPP
#define RUNTIME_INFO_HPP

// Startup diagnostics (Infrastructure): prints compiler/OS/toolkit versions.
// Lives apart from the persistence services so csv_io/project_io stay free of
// presentation-toolkit includes where possible.

#include <QtCore/QSysInfo>
#include <QtCore/QtVersion>
#include <ostream>
#include <string>

namespace application {

inline void PrintRuntimeInfo(std::ostream& out) {
  out << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
  out << "ProductType " << QSysInfo::productType().toStdString() << '\n';
  out << "ProductVersion " << QSysInfo::productVersion().toStdString() << '\n';
  out << "KernelType " << QSysInfo::kernelType().toStdString() << ' '
      << QSysInfo::kernelVersion().toStdString() << '\n';
  out << "BuildAbi " << QSysInfo::buildAbi().toStdString() << '\n';
  out << "QT_VERSION_STR " << QT_VERSION_STR << '\n';
}

}  // namespace application

#endif  // RUNTIME_INFO_HPP
