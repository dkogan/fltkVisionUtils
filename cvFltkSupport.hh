#ifndef __CV_FLTK_SUPPORT_HH__
#define __CV_FLTK_SUPPORT_HH__

#include <FL/Fl_RGB_Image.H>

#include "opencv/highgui.h"
#include "opencv/cv.h"

class cvFltkImage : public Fl_Widget
{
    Fl_RGB_Image* flImage;      // for drawing
    IplImage*     cvImage;      // for processing

public:
    // a dummy size to start out with. will correct once I load the image
    cvFltkImage(const char* path, int x, int y, int w = 10, int h = 10)
        : Fl_Widget(x, y, w, h),
          flImage(NULL), cvImage(NULL)
    {
        cvImage = cvLoadImage(path, CV_LOAD_IMAGE_GRAYSCALE);
        if(cvImage == NULL)
            return;

        flImage = new Fl_RGB_Image((unsigned char*)cvImage->imageData, cvImage->width, cvImage->height, 1);
        size(flImage->w(), flImage->h());
    }

    cvFltkImage(int x, int y, int w, int h)
        : Fl_Widget(x, y, w, h),
          flImage(NULL), cvImage(NULL)
    {
        cvImage = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
        if(cvImage == NULL)
            return;

        flImage = new Fl_RGB_Image((unsigned char*)cvImage->imageData, cvImage->width, cvImage->height, 1);
        size(flImage->w(), flImage->h());
    }

    ~cvFltkImage()
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

    void replace(CvMat* mat)
    {
        cvCopy(mat, cvImage);
        flImage->uncache();
        redraw();
    }

    void replace(IplImage* img)
    {
        // the opencv API treats IplImage* and CvMat* equally, but is written in C. Thus I must
        // create this equivalence explicitly
        replace((CvMat*)img);
    }
};


#endif
