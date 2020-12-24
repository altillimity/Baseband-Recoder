#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class PanadapterWidget : public wxPanel
{

public:
    PanadapterWidget(wxFrame *parent);

    void paintEvent(wxPaintEvent &evt);
    void paintNow();
    void render(wxDC &dc);
    void renderLoop(wxIdleEvent &evt);
    void OnEraseBackGround(wxEraseEvent &event){};

    int update_freq;

    DECLARE_EVENT_TABLE()

public:
    float *constellation_buffer;
};