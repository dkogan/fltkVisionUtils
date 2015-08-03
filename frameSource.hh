#ifndef __FRAME_SOURCE_H__
#define __FRAME_SOURCE_H__

#include <stdint.h>
#include "threadUtils.hh"
#include <opencv2/core/types_c.h>

// user interface color choice. RGB8 or MONO8
enum FrameSource_UserColorChoice  { FRAMESOURCE_COLOR, FRAMESOURCE_GRAYSCALE };

typedef bool (FrameSourceCallback_t)(IplImage* buffer, uint64_t timestamp_us);

// This is the base class for different frame grabbers. The constructor allows color or monochrome
// mode to be selected. For simplicity, color always means 8-bits-per-channel RGB and monochrome
// always means 8-bit grayscale
class FrameSource
{
protected:
    FrameSource_UserColorChoice userColorMode; // color (RGB8) or grayscale (MONO8)

    // the dimensions of the returned data. Post cropping and scaling
    unsigned int width, height;

    // I'm cropping to this rect. If cropRect.width < 0, I don't crop at all
    CvRect    cropRect;
    // The raw frame goes here, then this gets cropped and scaled. Frame sources can bypass this
    // image to gain efficiency.
    IplImage* preCropScaleBuffer;

    pthread_t              sourceThread_id;
    uint64_t               sourceThread_frameWait_us;

    FrameSourceCallback_t* sourceThread_callback;
    IplImage*              sourceThread_buffer;

    // I use a condition to control the "running" state of the frame source. Every get..Frame() call
    // checks this conditon and waits for it to trigger, if necessary
    MTcondition isRunningNow;

private:
    // These are the internal APIs called only by the external function definitions below.
    // To impleement new frame sources, these MUST be overridden
    virtual bool _restartStream(void) = 0;
    virtual bool _resumeStream (void) = 0;
    virtual bool _getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL) = 0;
    virtual bool _getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL) = 0;
    virtual bool _stopStream(void) = 0;

public:
    FrameSource (FrameSource_UserColorChoice _userColorMode = FRAMESOURCE_COLOR);

protected:
    void setupCroppingScaling(CvRect _cropRect, double scale);
    void applyCroppingScaling(IplImage* src, IplImage* dst);

public:
    virtual void cleanupThreads(void);
    virtual ~FrameSource();

    // I want the derived classes to override this. It indicates whether the class is initialized
    // and ready to use
    virtual operator bool() = 0;

    // blocking frame accessors
    //
    // getNextFrame() returns the next frame in the buffer.
    // getLatestFrame() purges the buffer and returns the most recent frame available
    // For non-realtime data sources, such as video files, getNextFrame() and getLatestFrame() are
    // the same.
    bool getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL);
    bool getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL);

    bool stopStream(void);
    bool resumeStream (void);
    bool restartStream(void);

    // Instead of accessing the frame with blocking I/O, the frame source can spawn a thread to wait
    // for the frames, and callback when a new frame is available. The thread can optionally wait
    // some amount of time between successive frames to force-limit the framerate by giving a
    // non-zero frameWait_us. In this case only the latest available frame will be returned with the
    // rest thrown away. Otherwise, we try not to miss frames by returning the next available frame.
    //
    // The callback is passed the buffer where the data was stored. Note that this is the same
    // buffer that was originally passed to startSourceThread. Note that this buffer is accessed
    // asynchronously, so the caller can NOT assume it contains valid data outside of the callback

    void startSourceThread(FrameSourceCallback_t* callback, uint64_t frameWait_us,
                           IplImage* buffer);
    void sourceThread(void);

    // Another way to drive the application is to keep everything synchronous
    // using a select() or poll() in the main loop to look at ALL the file
    // descriptors that may need attention. Here I return such a file
    // descriptor. If unavailable, returns -1
    virtual int getFD(void);

    unsigned int w() { return width;  }
    unsigned int h() { return height; }
};

#endif
