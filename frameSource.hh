#ifndef __FRAME_SOURCE_H__
#define __FRAME_SOURCE_H__

#include <stdint.h>

// user interface color choice. RGB8 or MONO8
enum FrameSource_UserColorChoice  { FRAMESOURCE_COLOR, FRAMESOURCE_GRAYSCALE };

// This is the base class for different frame grabbers. The constructor allows color or monochrome
// mode to be selected. For simplicity, color always means 8-bits-per-channel RGB and monochrome
// always means 8-bit grayscale
class FrameSource
{
protected:
    FrameSource_UserColorChoice userColorMode; // color (RGB8) or grayscale (MONO8)
    bool                        inited;
    unsigned int                width, height;

public:
    FrameSource (FrameSource_UserColorChoice _userColorMode = FRAMESOURCE_COLOR)
        : userColorMode(_userColorMode), inited(false) { }

    virtual ~FrameSource() {}

    operator bool() { return inited; }

    // peek...Frame() blocks until a frame is available. A pointer to the internal buffer is
    // returned (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed.
    // peekNextFrame() returns the next frame in the buffer.
    // peekLatestFrame() purges the buffer and returns the most recent frame available
    //
    // The peek...Frame() functions return RAW data. No color conversion is attempted. Use with
    // caution
    virtual unsigned char* peekNextFrame  (uint64_t* timestamp_us) = 0;
    virtual unsigned char* peekLatestFrame(uint64_t* timestamp_us) = 0;
    virtual void unpeekFrame(void) = 0;

    // these are like the peek() functions, but these convert the incoming data to the desired
    // colorspace (RGB8 or MONO8 depending on the userColorMode). Since these make a copy of the
    // data, calling unpeek() is not needed. false returned on error
    virtual bool getNextFrame  (uint64_t* timestamp_us, unsigned char* buffer) = 0;
    virtual bool getLatestFrame(uint64_t* timestamp_us, unsigned char* buffer) = 0;

    unsigned int w() { return width;  }
    unsigned int h() { return height; }
};

#endif
