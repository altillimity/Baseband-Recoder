#include "panadapter.h"

#include <thread>

BEGIN_EVENT_TABLE(PanadapterWidget, wxPanel)
EVT_PAINT(PanadapterWidget::paintEvent)
EVT_ERASE_BACKGROUND(PanadapterWidget::OnEraseBackGround)
END_EVENT_TABLE()

PanadapterWidget::PanadapterWidget(wxFrame *parent) : wxPanel(parent)
{
    constellation_buffer = new float[1024];
    std::fill(&constellation_buffer[0], &constellation_buffer[1024], 0);
}

void PanadapterWidget::render(wxDC &dc)
{
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawRectangle(0, 0, GetSize().GetWidth(), GetSize().GetHeight());

    dc.SetPen(*wxGREY_PEN);

    for (int i = 0; i < GetSize().GetHeight(); i += 15)
    {
        dc.DrawLine(wxPoint(0, i), wxPoint(GetSize().GetWidth(), i));
    }

    dc.SetBrush(*wxGREEN_BRUSH);
    dc.SetPen(*wxGREEN_PEN);

    float scale_ratio = (float)GetSize().GetWidth() / 1024.0f;
    int height = GetSize().GetHeight();
    float scale_height = height / 80.0f;

    for (int i = 0; i < 1024 - 1; i++)
    {
        dc.DrawLine(wxPoint(i * scale_ratio, abs(height - std::min((int)(constellation_buffer[i] * scale_height), height))),
                    wxPoint(i * scale_ratio + 1 * scale_ratio, abs(height - std::min((int)(constellation_buffer[i + 1] * scale_height), height))));
    }
}

void PanadapterWidget::paintEvent(wxPaintEvent &evt)
{
    wxPaintDC dc(this);
    render(dc);
}

void PanadapterWidget::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}

void PanadapterWidget::renderLoop(wxIdleEvent &evt)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Refresh();
    evt.RequestMore();
}
