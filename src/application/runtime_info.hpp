#ifndef RUNTIME_INFO_HPP
#define RUNTIME_INFO_HPP

// Startup diagnostics (Infrastructure): prints compiler/OS/toolkit versions.
// Lives apart from the persistence services so csv_io/project_io stay free of
// presentation-toolkit includes where possible.

#include <iosfwd>

namespace application {

void PrintRuntimeInfo(std::ostream& out);

}  // namespace application

#endif  // RUNTIME_INFO_HPP
