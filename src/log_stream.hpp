#ifndef LOG_STREAM_HPP
#define LOG_STREAM_HPP

#include <ostream>
#include <wx/textctrl.h>

class LogStream : public std::ostream {
public:
  void set(wxTextCtrl *text_ctrl) { set_rdbuf(text_ctrl); }
};

extern LogStream logs;

#endif // LOG_STREAM_HPP