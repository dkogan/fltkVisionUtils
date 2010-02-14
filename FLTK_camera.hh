#ifndef __FLTK_CAMERA_HH__
#define __FLTK_CAMERA_HH__

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <string.h>
#include <stdio.h>

class CameraWidget : public Fl_Widget
{
    int cameraW, cameraH;

public:
    CameraWidget(int x, int y, int w, int h,
                 int _cameraW, int _cameraH)
        : Fl_Widget(x,y,w,h)
    {
        cameraW = _cameraW;
        cameraH = _cameraH;
    }

    // this is the FLTK draw-me-now callback
    void draw()
    {
    }

    void updateFrame(unsigned char* frame)
    {
        if(frame)
        {
            // I want to draw the next frame. Set the damage flag to tell the widget that it needs
            // to be redrawn, and actually draw the frame. Normally the drawing will be done in the
            // draw() callback, but since I will update the image with the camera updates, this will
            // happen often anyway and I don't need X's fancy redrawing and caching logic
            damage(0);
            fl_draw_image_mono(frame, x(), y(), cameraW, cameraH);
        }
    }
};

#endif
