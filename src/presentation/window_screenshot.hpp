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

// Ein Bild, das über den Widget-Abzug gelegt wird, samt Platz im Fenster.
// Nötig für die GL-Fläche: ein wxDC sieht sie nicht, ihr Inhalt muss separat
// geliefert und einmontiert werden.
struct Overlay {
  wxImage image;
  wxPoint origin;
  wxSize size;
};

// Nimmt den Widget-Baum eines Fensters über einen wxClientDC ab, montiert das
// Overlay darauf und schreibt das Ganze als PNG. Meldet Erfolg.
//
// Der Widget-Abzug gelingt nur unter X11/Xvfb; unter Wayland liefert der Blit
// Schwarz. Siehe betrieb.md, Kopflose Läufe.
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
