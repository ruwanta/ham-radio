#pragma once

#include <wx/wx.h>
#include "wxWorldMapMercator.h"

class RadioFrame : public wxFrame
{
    wxWorldMapMercator* worldMapMercator;
    
public:
    RadioFrame();
 
private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
};

enum
{
    ID_Hello = 1
};
