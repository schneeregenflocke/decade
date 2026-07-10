#ifndef RUNTIME_INFO_HPP
#define RUNTIME_INFO_HPP

// Startup diagnostics (Infrastructure): prints compiler/OS/toolkit versions.
// Lives apart from the persistence services so csv_io/project_io stay free of
// presentation-toolkit includes where possible.

#include <wx/platinfo.h>
#include <wx/version.h>

#include <ostream>
#include <string>

namespace app::io {

inline void PrintRuntimeInfo(std::ostream& out) {
  out << std::string("__cplusplus ") + std::to_string(__cplusplus) << '\n';
  out << "OperatingSystemIdName "
      << wxPlatformInfo::Get().GetOperatingSystemIdName() << '\n';
  out << "ArchName " << wxPlatformInfo::Get().GetBitnessName() << '\n';
  out << "OSMajorVersion.OSMinorVersion.OSMicroVersion "
      << wxPlatformInfo::Get().GetOSMajorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMinorVersion() << '.'
      << wxPlatformInfo::Get().GetOSMicroVersion() << '\n';

  const auto wxwidgets_version = std::wstring(wxVERSION_STRING);
  out << "wxVERSION_STRING "
      << std::string(wxwidgets_version.cbegin(), wxwidgets_version.cend())
      << '\n';
}

}  // namespace app::io

#endif  // RUNTIME_INFO_HPP
