#include "RadioFrame.h"
// #include "wxWorldMapMercator.h"
 
RadioFrame::RadioFrame()
    : wxFrame(nullptr, wxID_ANY, "Hello World")
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
                     "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
 
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
 
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
 
    SetMenuBar( menuBar );
 
    CreateStatusBar();
    SetStatusText("Welcome to wxWidgets!");

    // wxPanel* myPanel = new wxPanel (this, wxID_ANY, wxDefaultPosition, wxSize(100, 100), wxTAB_TRAVERSAL, _T(""));
    // myPanel -> SetBackgroundColour(wxColour(100, 100, 200));
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    worldMapMercator = new wxWorldMapMercator(this, wxT("MercatorMap"), wxBITMAP_TYPE_JPEG);
    sizer->Add(worldMapMercator, 1, wxEXPAND);
    this->SetSizer(sizer);
 
    Bind(wxEVT_MENU, &RadioFrame::OnHello, this, ID_Hello);
    Bind(wxEVT_MENU, &RadioFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &RadioFrame::OnExit, this, wxID_EXIT);
}
 
void RadioFrame::OnExit(wxCommandEvent& event)
{
    Close(true);
}
 
void RadioFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("This is a wxWidgets Hello World example",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}
 
void RadioFrame::OnHello(wxCommandEvent& event)
{
    wxLogMessage("Hello world from wxWidgets!");
}