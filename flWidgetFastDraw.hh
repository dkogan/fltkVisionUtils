#ifndef __FL_WIDGET_FAST_DRAW_HH__
#define __FL_WIDGET_FAST_DRAW_HH__

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <string.h>
#include <stdio.h>

// this class is designed for simple visualization of streaming data. This class makes an assumption
// that new frames are constantly arriving, thus it draws DIRECTLY into the widget without creating
// an Fl_Image. The makes the drawing faster, but breaks redrawing. We're assuming we're constantly
// getting new data so redrawing is not needed. At some point in the distant future this should be
// changed to use hardware acceleration designed for this purpose, like Xv
class FlWidgetFastDraw : public Fl_Widget
{
    int frameW, frameH;

public:
    FlWidgetFastDraw(int x, int y, int w, int h,
                     int _frameW, int _frameH)
        : Fl_Widget(x,y,w,h)
    {
        frameW = _frameW;
        frameH = _frameH;
    }

    // this is the FLTK draw-me-now callback
    void draw()
    {
        // nothing here. We draw directly into the widget when new data comes in and never bother
        // with redrawing
    }

    // this should be called from the main FLTK thread or from any other thread after obtaining an
    // Fl::lock()
    void updateFrame(unsigned char* frame)
    {
        if(frame)
        {
            // I now draw the frame. Normally the drawing will be done in the draw() callback, but
            // since I will update the image with the camera updates, this will happen often anyway
            // and I don't need X's fancy redrawing and caching logic
            fl_draw_image_mono(frame, x(), y(), frameW, frameH);
        }
    }
};

#endif
