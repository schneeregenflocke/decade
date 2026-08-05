#ifndef WINDOW_SCREENSHOT_HPP
#define WINDOW_SCREENSHOT_HPP

#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/window.h>

#include <string>

#include "wx_owned.hpp"

namespace window_screenshot {

// An image laid over the widget capture, together with its place in the window.
// Needed for the GL surface: a wxDC does not see it, so its content must be
// delivered separately and mounted in.
struct Overlay {
  wxImage image;
  wxPoint origin;
  wxSize size;
};

// Captures the widget tree of a window through a wxClientDC, mounts the overlay
// onto it and writes the whole as a PNG. It reports success.
//
// The widget capture succeeds under X11 and Xvfb alone; under Wayland the blit
// delivers black. See operations.md, Headless runs.
[[nodiscard]] inline bool SaveWindowPng(wxWindow& window,
                                        const Overlay& overlay,
                                        const std::string& file_path) {
  const wxSize size = window.GetClientSize();
  if (size.GetWidth() <= 0 || size.GetHeight() <= 0) {
    return false;
  }

  wxBitmap bitmap(size.GetWidth(), size.GetHeight());
  {
    wxClientDC client_dc(&window);
    wxMemoryDC memory_dc(bitmap);
    memory_dc.Blit(0, 0, size.GetWidth(), size.GetHeight(), &client_dc, 0, 0);
  }

  if (overlay.image.IsOk()) {
    const wxImage fitted =
        (overlay.image.GetWidth() == overlay.size.GetWidth() &&
         overlay.image.GetHeight() == overlay.size.GetHeight())
            ? overlay.image
            : overlay.image.Scale(overlay.size.GetWidth(),
                                  overlay.size.GetHeight());
    wxMemoryDC memory_dc(bitmap);
    memory_dc.DrawBitmap(wxBitmap(fitted), overlay.origin, false);
  }

  if (wxImage::FindHandler(wxBITMAP_TYPE_PNG) == nullptr) {
    wxImage::AddHandler(MakeOwned<wxPNGHandler>());
  }
  return bitmap.ConvertToImage().SaveFile(file_path, wxBITMAP_TYPE_PNG);
}

}  // namespace window_screenshot

#endif  // WINDOW_SCREENSHOT_HPP
