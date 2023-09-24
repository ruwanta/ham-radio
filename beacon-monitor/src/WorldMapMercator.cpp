/**
 * Wx Panel having the mercator projection of the world map
*/

#include "wxWorldMapMercator.h"
#include <wx/sizer.h>



BEGIN_EVENT_TABLE(wxWorldMapMercator, wxPanel)
// some useful events
/*
 EVT_MOTION(wxWorldMapMercator::mouseMoved)
 EVT_LEFT_DOWN(wxWorldMapMercator::mouseDown)
 EVT_LEFT_UP(wxWorldMapMercator::mouseReleased)
 EVT_RIGHT_DOWN(wxWorldMapMercator::rightClick)
 EVT_LEAVE_WINDOW(wxWorldMapMercator::mouseLeftWindow)
 EVT_KEY_DOWN(wxWorldMapMercator::keyPressed)
 EVT_KEY_UP(wxWorldMapMercator::keyReleased)
 EVT_MOUSEWHEEL(wxWorldMapMercator::mouseWheelMoved)
 */

// catch paint events
// EVT_PAINT(wxWorldMapMercator::paintEvent)
//Size event
EVT_SIZE(wxWorldMapMercator::OnSize)
END_EVENT_TABLE()


// some useful events
/*
 void wxWorldMapMercator::mouseMoved(wxMouseEvent& event) {}
 void wxWorldMapMercator::mouseDown(wxMouseEvent& event) {}
 void wxWorldMapMercator::mouseWheelMoved(wxMouseEvent& event) {}
 void wxWorldMapMercator::mouseReleased(wxMouseEvent& event) {}
 void wxWorldMapMercator::rightClick(wxMouseEvent& event) {}
 void wxWorldMapMercator::mouseLeftWindow(wxMouseEvent& event) {}
 void wxWorldMapMercator::keyPressed(wxKeyEvent& event) {}
 void wxWorldMapMercator::keyReleased(wxKeyEvent& event) {}
 */

wxWorldMapMercator::wxWorldMapMercator(wxFrame* parent, wxString file, wxBitmapType format) :
wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    image.LoadFile(file, format);
    w = -1;
    h = -1;
}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

void wxWorldMapMercator::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxWorldMapMercator::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxWorldMapMercator::render(wxDC&  dc)
{
    int neww, newh;
    dc.GetSize( &neww, &newh );
    
    if( neww != w || newh != h )
    {
        resized = wxBitmap( image.Scale( neww, newh /*, wxIMAGE_QUALITY_HIGH*/ ) );
        w = neww;
        h = newh;
        dc.DrawBitmap( resized, 0, 0, false );
    }else{
        dc.DrawBitmap( resized, 0, 0, false );
    }
}

/*
 * Here we call refresh to tell the panel to draw itself again.
 * So when the user resizes the image panel the image should be resized too.
 */
void wxWorldMapMercator::OnSize(wxSizeEvent& event){
    Refresh();
    //skip the event.
    event.Skip();
}