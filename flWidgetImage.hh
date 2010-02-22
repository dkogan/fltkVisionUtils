#ifndef __FL_WIDGET_IMAGE_HH__
#define __FL_WIDGET_IMAGE_HH__

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_RGB_Image.H>
#include <string.h>
#include <stdio.h>

// this class is designed for simple visualization of data passed into the class from the
// outside. Examples of data sources are cameras, video files, still images, processed data, etc.

// Both color and grayscale displays are supported. Incoming data is assumed to be of the desired
// type
enum FlWidgetImage_ColorChoice  { WIDGET_COLOR, WIDGET_GRAYSCALE };

// Two drawing modes are supported:

// 1. Normal drawing using an Fl_Image object. This mode requires a bit of overhead for the initial
// drawing operation, but redraws are very fast. This is the mode to use most of the time
//
// 2. Direct drawing mode. This modes makes an assumption that new frames are constantly arriving,
// thus it draws DIRECTLY into the widget without creating an Fl_Image. This makes the drawing
// faster, but breaks redrawing. This mode assumes that we're constantly getting new data so
// intelligent redrawing is not needed. At some point in the distant future this should be changed
// to use hardware acceleration designed for this purpose, like Xv. This mode can be used to display
// data coming from a video camera, for example
enum FlWidgetImage_RedrawChoice { NORMAL, FAST_REDRAW = NORMAL,
                                  DIRECT, FAST_DRAW   = DIRECT};

class FlWidgetImage : public Fl_Widget
{
    int frameW, frameH;

    FlWidgetImage_ColorChoice  colorMode;
    FlWidgetImage_RedrawChoice redrawMode;
    unsigned int bytesPerPixel;

    // these are only used for FAST_REDRAW
    unsigned char* imageData;
    Fl_RGB_Image*  flImage;

    void cleanup(void)
    {
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

public:
    FlWidgetImage(int x, int y, int w, int h,
                  FlWidgetImage_ColorChoice  _colorMode,
                  FlWidgetImage_RedrawChoice _redrawMode)
        : Fl_Widget(x,y,w,h),
          frameW(w), frameH(h),
          colorMode(_colorMode), redrawMode(_redrawMode),
          imageData(NULL), flImage(NULL)
    {
        bytesPerPixel = (colorMode == WIDGET_COLOR) ? 3 : 1;

        if(redrawMode == NORMAL)
        {
            imageData = new unsigned char[frameW * frameH * bytesPerPixel];
            if(imageData == NULL)
                return;

            flImage = new Fl_RGB_Image(imageData, frameW, frameH, bytesPerPixel);
            if(flImage == NULL)
            {
                cleanup();
                return;
            }
        }
    }

    ~FlWidgetImage()
    {
        cleanup();
    }

    // this can be used to render directly into the buffer
    unsigned char* getBuffer(void)
    {
        if(redrawMode != NORMAL)
            fprintf(stderr, "FlWidgetImage::getBuffer() while redrawMode != NORMAL.\n"
                    "This mode doesn't use a buffer\n");

        return imageData;
    }

    void draw()
    {
        // this is the FLTK draw-me-now callback. Draw the image if we're not direct drawing
        if(redrawMode == NORMAL)
            flImage->draw(0,0);
    }

    // this should be called from the main FLTK thread or from any other thread after obtaining an
    // Fl::lock()
    void updateFrame(unsigned char* frame)
    {
        if(!frame)
            return;

        if(redrawMode == NORMAL)
        {
            memcpy(imageData, frame, frameW*frameH*bytesPerPixel);

            flImage->uncache();
            redraw();

            // If we're drawing from a different thread, FLTK needs to be woken up to actually do
            // the redraw
            Fl::awake();
        }
        else
        {
            if(colorMode == WIDGET_COLOR)
                fl_draw_image(frame, x(), y(), frameW, frameH, 3);
            else
                fl_draw_image_mono(frame, x(), y(), frameW, frameH);
        }
    }

    // Used to trigger a redraw if out drawing buffer was already updated. This functin makes sense
    // only in the buffered 'NORMAL' redraw mode
    void redrawNewFrame(void)
    {
        if(redrawMode == NORMAL)
        {
            flImage->uncache();
            redraw();

            // If we're drawing from a different thread, FLTK needs to be woken up to actually do
            // the redraw
            Fl::awake();
        }
        else
            fprintf(stderr, "redrawNewFrame() doesn't make sense in an unbuffered redraw mode\n");
    }
};

#endif
