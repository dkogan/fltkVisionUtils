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

    // peek...Frame() blocks until a frame is available. A pointer to the internal buffer is
    // returned (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed.
    // peekNextFrame() returns the next frame in the buffer.
    // peekLatestFrame() purges the buffer and returns the most recent frame available
    //
    // The peek...Frame() functions return RAW data. No color conversion is attempted. Use with
    // caution
    unsigned char* peekNextFrame  (uint64_t* timestamp_us = NULL)
    {
        makeTimestamp(timestamp_us);
        return (unsigned char*)image->imageData;
    }

    unsigned char* peekLatestFrame(uint64_t* timestamp_us = NULL)
    {
        makeTimestamp(timestamp_us);
        return (unsigned char*)image->imageData;
    }

    void unpeekFrame(void)
    {
    }

    // these are like the peek() functions, but these convert the incoming data to the desired
    // colorspace (RGB8 or MONO8 depending on the userColorMode). Since these make a copy of the
    // data, calling unpeek() is not needed. false returned on error
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
