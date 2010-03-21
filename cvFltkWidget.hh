#ifndef __CV_FLTK_WIDGET_HH__
#define __CV_FLTK_WIDGET_HH__

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_RGB_Image.H>
#include <opencv/cv.h>

#include <string.h>
#include <stdio.h>

// this class is designed for simple visualization of data passed into the class from the
// outside. Examples of data sources are cameras, video files, still images, processed data, etc.

// Both color and grayscale displays are supported. Incoming data is assumed to be of the desired
// type
enum CvFltkWidget_ColorChoice  { WIDGET_COLOR, WIDGET_GRAYSCALE };

class CvFltkWidget : public Fl_Widget
{
protected:
    int frameW, frameH;

    CvFltkWidget_ColorChoice  colorMode;
    unsigned int bytesPerPixel;

    unsigned char* imageData;
    Fl_RGB_Image*  flImage;
    IplImage*      cvImage;

    void cleanup(void)
    {
        if(cvImage == NULL)
        {
            cvReleaseImageHeader(&cvImage);
            cvImage = NULL;
        }

        if(flImage != NULL)
        {
            delete flImage;
            flImage = NULL;
        }
        if(imageData != NULL)
        {
            delete[] imageData;
            imageData = NULL;
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
          frameW(w), frameH(h),
          colorMode(_colorMode),
          imageData(NULL), flImage(NULL), cvImage(NULL)
    {
        bytesPerPixel = (colorMode == WIDGET_COLOR) ? 3 : 1;

        imageData = new unsigned char[frameW * frameH * bytesPerPixel];
        if(imageData == NULL)
            return;

        flImage = new Fl_RGB_Image(imageData, frameW, frameH, bytesPerPixel);
        if(flImage == NULL)
        {
            cleanup();
            return;
        }

        cvImage = cvCreateImageHeader(cvSize(w,h), IPL_DEPTH_8U, bytesPerPixel);
        if(cvImage == NULL)
            return;

        cvSetData(cvImage, imageData, w*bytesPerPixel);
    }

    virtual ~CvFltkWidget()
    {
        cleanup();
    }

    operator IplImage*()
    {
        return cvImage;
    }

    // this can be used to render directly into the buffer
    unsigned char* getBuffer(void)
    {
        return imageData;
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
