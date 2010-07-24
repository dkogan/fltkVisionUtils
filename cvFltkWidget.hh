#ifndef __CV_FLTK_WIDGET_HH__
#define __CV_FLTK_WIDGET_HH__

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_RGB_Image.H>

#include "cvlib.hh"

#include <string.h>

// this class is designed for simple visualization of data passed into the class from the
// outside. Examples of data sources are cameras, video files, still images, processed data, etc.

// Both color and grayscale displays are supported. Incoming data is assumed to be of the desired
// type
enum CvFltkWidget_ColorChoice  { WIDGET_COLOR, WIDGET_GRAYSCALE };

class CvFltkWidget : public Fl_Widget
{
    CvFltkWidget_ColorChoice  colorMode;

    Fl_RGB_Image*  flImage;
    IplImage*      cvImage;

    void cleanup(void)
    {
        if(flImage != NULL)
        {
            delete flImage;
            flImage = NULL;
        }

        if(cvImage == NULL)
        {
            cvReleaseImage(&cvImage);
            cvImage = NULL;
        }
    }

    // called by FLTK to alert this widget about an event. An upper level callback can be triggered
    // here
    virtual int handle(int event)
    {
        switch(event)
        {
        case FL_DRAG:
            if(!Fl::event_inside(this))
                break;
            // fall through if we're inside

        case FL_PUSH:
            do_callback();
            return 1;
        }

        return Fl_Widget::handle(event);
    }

public:
    CvFltkWidget(int x, int y, int w, int h,
                 CvFltkWidget_ColorChoice  _colorMode)
        : Fl_Widget(x,y,w,h),
          colorMode(_colorMode),
          flImage(NULL), cvImage(NULL)
    {
        int numChannels = (colorMode == WIDGET_COLOR) ? 3 : 1;

        cvImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, numChannels);
        if(cvImage == NULL)
            return;

        flImage = new Fl_RGB_Image((unsigned char*)cvImage->imageData,
                                   w, h, numChannels, cvImage->widthStep);
        if(flImage == NULL)
        {
            cleanup();
            return;
        }
    }

    virtual ~CvFltkWidget()
    {
        cleanup();
    }

    operator IplImage*()
    {
        return cvImage;
    }

    void draw()
    {
        // this is the FLTK draw-me-now callback
        flImage->draw(x(), y());
    }

    // Used to trigger a redraw if out drawing buffer was already updated.
    void redrawNewFrame(void)
    {
        flImage->uncache();
        redraw();

        // If we're drawing from a different thread, FLTK needs to be woken up to actually do
        // the redraw
        Fl::awake();
    }
};

#endif
