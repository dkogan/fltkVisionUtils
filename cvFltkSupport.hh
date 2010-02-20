#ifndef __CV_FLTK_SUPPORT_HH__
#define __CV_FLTK_SUPPORT_HH__

#include <FL/Fl_RGB_Image.H>

#include "opencv/cv.h"

class CvFltkWidget : public Fl_Widget
{
    Fl_RGB_Image* flImage;      // for drawing
    IplImage*     cvImage;      // for processing

    void finishConstructing(void)
    {
        if(cvImage == NULL)
            return;

        flImage = new Fl_RGB_Image((unsigned char*)cvImage->imageData, cvImage->width, cvImage->height, 1);
        size(flImage->w(), flImage->h());
    }

public:
    CvFltkWidget(int x, int y, int w, int h)
        : Fl_Widget(x, y, w, h),
          flImage(NULL), cvImage(NULL)
    {
        cvImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
        finishConstructing();
    }

    ~CvFltkWidget()
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

    operator bool()
    {
        return cvImage != NULL;
    }

    operator IplImage*()
    {
        return cvImage;
    }

    // this is the FLTK draw-me-now callback
    void draw()
    {
        flImage->draw(0,0);
    }

    int w()
    {
        return flImage->w();
    }

    int h()
    {
        return flImage->h();
    }

    void updateImage(CvMat* mat)
    {
        cvCopy(mat, cvImage);
        flImage->uncache();
        redraw();

#warning this flush() shouldn't be needed. Without it, draw() isn't called when frames come from a camera
        Fl::flush();
    }

    void updateImage(IplImage* img)
    {
        // the opencv API treats IplImage* and CvMat* equally, but this API is written in C. I'm
        // using C++ so I must create this equivalence explicitly
        updateImage((CvMat*)img);
    }
};


#endif
