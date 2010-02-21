#ifndef __FRAME_SOURCE_H__
#define __FRAME_SOURCE_H__

#include <stdint.h>

// This is the base class for different frame grabbers. The constructor allows color or monochrome
// mode to be selected. For simplicity, color always means 8-bits-per-channel RGB and monochrome
// always means 8-bit grayscale
class FrameSource
{
protected:
    bool         isColor; // color (RGB8) or grayscale (MONO8)?
    bool         inited;
    unsigned int width, height;

public:
    FrameSource (bool _isColor)
        : isColor(_isColor), inited(false) { }

    virtual ~FrameSource() {}

    operator bool() { return inited; }

    // peekFrame() blocks until a frame is available. A pointer to the internal buffer is returned
    // (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed
    virtual unsigned char* peekFrame(uint64_t* timestamp_us) = 0;
    virtual void unpeekFrame(void)                           = 0;

    // copyFrame() is similar to peekFrame(), except the frame data is copied to the given
    // buffer. In this case, unpeek need not be called. NULL is returned on failure
    unsigned char* copyFrame(uint64_t* timestamp_us, unsigned char* frame);

    unsigned int w() { return width;  }
    unsigned int h() { return height; }
};

#endif
