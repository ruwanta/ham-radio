
// Start of wxWidgets "Hello World" Program
#include <wx/wx.h>
#include "RadioFrame.h"
 
class BeaconMonitorApp : public wxApp
{
public:
    bool OnInit() override;
};
 
wxIMPLEMENT_APP(BeaconMonitorApp);
 

 
bool BeaconMonitorApp::OnInit()
{
    RadioFrame *frame = new RadioFrame();
    frame->Show(true);
    return true;
}
 
