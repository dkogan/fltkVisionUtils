#include "staticImageSource.hh"
#include <iostream>
using namespace std;

StaticImageSource::StaticImageSource(const char* file, FrameSource_UserColorChoice _userColorMode,
                                     CvRect _cropRect, double scale)
    // constructor optionally takes in cropping window and post-cropping scale factor
    : FrameSource(_userColorMode), image(NULL), timestamp_now_us(0)
{
    load(file, _cropRect, scale);
}

bool StaticImageSource::load(const char* file,
                             CvRect _cropRect, double scale)

{
    if(userColorMode == FRAMESOURCE_COLOR)
        image = cvLoadImage(file, CV_LOAD_IMAGE_COLOR);
    else if(userColorMode == FRAMESOURCE_GRAYSCALE)
        image = cvLoadImage(file, CV_LOAD_IMAGE_GRAYSCALE);
    else
    {
        cerr << "StaticImageSource::load(): unknown color mode" << endl;
        return false;
    }

    if(image == NULL)
        return false;

    width  = image->width;
    height = image->height;

    setupCroppingScaling(_cropRect, scale);

    isRunningNow.setTrue();

    return true;
}

StaticImageSource::~StaticImageSource()
{
    cleanupThreads();

    if(image != NULL)
    {
        cvReleaseImage(&image);
        image = NULL;
    }
}

StaticImageSource::operator bool()
{
    return image != NULL;
}

bool StaticImageSource::_getNextFrame  (IplImage* buffer, uint64_t* timestamp_us)
{
    if(!(*this))
        return false;

#warning I should do the cropping/scaling once at the start, instead of every time I give a frame
    applyCroppingScaling(image, buffer);
    makeTimestamp(timestamp_us);

    return true;
}

// for static images, _getNextFrame() is the same as _getLatestFrame()
bool StaticImageSource::_getLatestFrame(IplImage* buffer, uint64_t* timestamp_us)
{
    return _getNextFrame(buffer, timestamp_us);
}
