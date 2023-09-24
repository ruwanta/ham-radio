#pragma once
/**
 * Definition for the worldmap in Mercator projection
*/
#include <wx/wx.h>

class wxWorldMapMercator : public wxPanel
{
    wxImage image;
    wxBitmap resized;
    int w, h;
    
public:
    wxWorldMapMercator(wxFrame* parent, wxString file, wxBitmapType format);
    
    void paintEvent(wxPaintEvent & evt);
    void paintNow();
    void OnSize(wxSizeEvent& event);
    void render(wxDC& dc);
    
    // some useful events
    /*
     void mouseMoved(wxMouseEvent& event);
     void mouseDown(wxMouseEvent& event);
     void mouseWheelMoved(wxMouseEvent& event);
     void mouseReleased(wxMouseEvent& event);
     void rightClick(wxMouseEvent& event);
     void mouseLeftWindow(wxMouseEvent& event);
     void keyPressed(wxKeyEvent& event);
     void keyReleased(wxKeyEvent& event);
     */
    
    DECLARE_EVENT_TABLE()
};