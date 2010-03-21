#ifndef __STATIC_IMAGE_SOURCE_HH__
#define __STATIC_IMAGE_SOURCE_HH__

#include "frameSource.hh"
#include <opencv/highgui.h>

class StaticImageSource : public FrameSource
{
    IplImage*            image;
    uint64_t             timestamp_now_us;

    void makeTimestamp(uint64_t* timestamp_us)
    {
        if(timestamp_us == NULL)
            return;

        timestamp_now_us++;
        *timestamp_us = timestamp_now_us;
    }

public:
    StaticImageSource(FrameSource_UserColorChoice _userColorMode)
        : FrameSource(_userColorMode), image(NULL), timestamp_now_us(0)
    {
    }

    StaticImageSource(const char* file, FrameSource_UserColorChoice _userColorMode)
        : FrameSource(_userColorMode), image(NULL), timestamp_now_us(0)
    {
        load(file);
    }

    bool load(const char* file)
    {
        if(userColorMode == FRAMESOURCE_COLOR)
            image = cvLoadImage(file, CV_LOAD_IMAGE_COLOR);
        else if(userColorMode == FRAMESOURCE_GRAYSCALE)
            image = cvLoadImage(file, CV_LOAD_IMAGE_GRAYSCALE);
        else
        {
            fprintf(stderr, "StaticImageSource::load(): unknown color mode\n");
            return false;
        }

        if(image == NULL)
            return false;

        width  = image->width;
        height = image->height;
        return true;
    }

    ~StaticImageSource()
    {
        cleanupThreads();

        if(image != NULL)
        {
            cvReleaseImage(&image);
            image = NULL;
        }
    }

    operator bool() { return image != NULL; }

    bool getNextFrame  (IplImage* buffer, uint64_t* timestamp_us = NULL)
    {
        cvCopy(image, buffer);
        makeTimestamp(timestamp_us);
        return true;
    }

    bool getLatestFrame(IplImage* buffer, uint64_t* timestamp_us = NULL)
    {
        cvCopy(image, buffer);
        makeTimestamp(timestamp_us);
        return true;
    }

};

#endif
