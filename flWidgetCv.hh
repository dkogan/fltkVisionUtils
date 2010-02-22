#ifndef __FL_WIDGET_CV_HH__
#define __FL_WIDGET_CV_HH__

#include "flWidgetImage.hh"
#include "opencv/cv.h"

// this class is designed for simple visualization of an openCV image in an FLTK widget. The
// IplImage object contains the image data. The Fl_RGB_Image object does not contain the image data,
// but rather references the IplImage
class FlWidgetCv : public FlWidgetImage
{
    IplImage*     cvImage;

    void cleanup(void)
    {
        if(cvImage == NULL)
        {
            cvReleaseImageHeader(&cvImage);
            cvImage = NULL;
        }

        FlWidgetImage::cleanup();
    }

public:
    FlWidgetCv(int x, int y, int w, int h,
               FlWidgetImage_ColorChoice  _colorMode)
        : FlWidgetImage(x,y,w,h, _colorMode, FAST_REDRAW),
          cvImage(NULL)
    {
        cvImage = cvCreateImageHeader(cvSize(w,h), IPL_DEPTH_8U, bytesPerPixel);
        if(cvImage == NULL)
            return;

        cvSetData(cvImage, imageData, w*bytesPerPixel);
    }

    ~FlWidgetCv()
    {
        cleanup();
    }

    operator IplImage*()
    {
        return cvImage;
    }

    void updateFrameCv(IplImage* image)
    {
        cvCopy(image, cvImage);
        redrawNewFrame();
    }

    void updateFrameCv(CvMat* image)
    {
        cvCopy(image, cvImage);
        redrawNewFrame();
    }

    void updateFrameCv(CvArr* image)
    {
        cvCopy(image, cvImage);
        redrawNewFrame();
    }

};

#endif
