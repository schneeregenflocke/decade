#ifndef DECADE_APP_HPP
#define DECADE_APP_HPP

#include <wx/app.h>
#include <wx/intl.h>

class DecadeApp : public wxApp {
 public:
  bool OnInit() override;

 private:
  wxLocale locale_;
};

#endif  // DECADE_APP_HPP
