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

        isRunningNow.setTrue();

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

private:
    bool _getNextFrame  (IplImage* buffer, uint64_t* timestamp_us = NULL)
    {
        if(!(*this))
            return false;

        cvCopy(image, buffer);
        makeTimestamp(timestamp_us);
        return true;
    }

    // for static images, _getNextFrame() is the same as _getLatestFrame()
    bool _getLatestFrame(IplImage* buffer, uint64_t* timestamp_us = NULL)
    {
        return _getNextFrame(buffer, timestamp_us);
    }

    // static images don't have any hardware on/off switch, nor is there anything to rewind. Thus
    // these functions are all stubs
    bool _stopStream   (void) { return true; }
    bool _resumeStream (void) { return true; }
    bool _restartStream(void) { return true; }
};

#endif
