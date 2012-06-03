#ifndef __STATIC_IMAGE_SOURCE_HH__
#define __STATIC_IMAGE_SOURCE_HH__

#include "frameSource.hh"
#include <opencv2/core/types_c.h>

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

    StaticImageSource(const char* file, FrameSource_UserColorChoice _userColorMode,
                      CvRect _cropRect = cvRect(-1, -1, -1, -1),
                      double scale = 1.0);

    bool load(const char* file,
              CvRect _cropRect = cvRect(-1, -1, -1, -1),
              double scale = 1.0);

    ~StaticImageSource();

    operator bool();

private:
    bool _getNextFrame  (IplImage* buffer, uint64_t* timestamp_us = NULL);
    bool _getLatestFrame(IplImage* buffer, uint64_t* timestamp_us = NULL);

    // static images don't have any hardware on/off switch, nor is there anything to rewind. Thus
    // these functions are all stubs
    bool _stopStream   (void) { return true; }
    bool _resumeStream (void) { return true; }
    bool _restartStream(void) { return true; }
};

#endif
