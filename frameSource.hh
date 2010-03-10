#ifndef __FRAME_SOURCE_H__
#define __FRAME_SOURCE_H__

#include <stdint.h>
#include <time.h>
#include <opencv/cv.h>

// user interface color choice. RGB8 or MONO8
enum FrameSource_UserColorChoice  { FRAMESOURCE_COLOR, FRAMESOURCE_GRAYSCALE };
typedef void (FrameSourceCallback_t)  (unsigned char* buffer, uint64_t timestamp_us);
typedef void (FrameSourceCallbackCv_t)(CvMat*         buffer, uint64_t timestamp_us);

static void* sourceThread_global  (void *pArg);
static void* sourceThreadCv_global(void *pArg);

// This is the base class for different frame grabbers. The constructor allows color or monochrome
// mode to be selected. For simplicity, color always means 8-bits-per-channel RGB and monochrome
// always means 8-bit grayscale
class FrameSource
{
protected:
    FrameSource_UserColorChoice userColorMode; // color (RGB8) or grayscale (MONO8)
    unsigned int                width, height;

    pthread_t              sourceThread_id;
    uint64_t               sourceThread_frameWait_us;

    // The thread will use the openCV objects or the raw objects, not both
    FrameSourceCallback_t*   sourceThread_callback;
    unsigned char*           sourceThread_buffer;
    FrameSourceCallbackCv_t* sourceThread_callbackCv;
    CvMat*                   sourceThread_bufferCv;

    unsigned char* getRawBuffer(IplImage* image)
    {
        unsigned char* buffer;
        cvGetRawData(image, &buffer);
        return buffer;
    }
    unsigned char* getRawBuffer(CvMat* image)
    {
        unsigned char* buffer;
        cvGetRawData(image, &buffer);
        return buffer;
    }


public:
    FrameSource (FrameSource_UserColorChoice _userColorMode = FRAMESOURCE_COLOR)
        : userColorMode(_userColorMode), sourceThread_id(0) { }

    virtual void cleanupThreads(void)
    {
        if(sourceThread_id != 0)
        {
            pthread_cancel(sourceThread_id);
            pthread_join(sourceThread_id, NULL);
            sourceThread_id = 0;
        }
    }

    virtual ~FrameSource()
    {
        // This should be called from the most derived class since the thread calls a virtual
        // function. Just in case we call it here also. This will do nothing if the derived class
        // already did this, but may crash if it didn't
        cleanupThreads();
    }

    // I want the derived classes to override this
    virtual operator bool() = 0;

    // peek...Frame() blocks until a frame is available. A pointer to the internal buffer is
    // returned (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed.
    // peekNextFrame() returns the next frame in the buffer.
    // peekLatestFrame() purges the buffer and returns the most recent frame available
    // For non-realtime data sources, such as video files, peekNextFrame() and peekLatestFrame() are
    // the same
    //
    // The peek...Frame() functions return RAW data. No color conversion is attempted. Use with
    // caution
    virtual unsigned char* peekNextFrame  (uint64_t* timestamp_us = NULL) = 0;
    virtual unsigned char* peekLatestFrame(uint64_t* timestamp_us = NULL) = 0;
    virtual void unpeekFrame(void) = 0;

    // these are like the peek() functions, but these convert the incoming data to the desired
    // colorspace (RGB8 or MONO8 depending on the userColorMode). Since these make a copy of the
    // data, calling unpeek() is not needed. false returned on error
    virtual bool getNextFrame  (unsigned char* buffer, uint64_t* timestamp_us = NULL) = 0;
    virtual bool getLatestFrame(unsigned char* buffer, uint64_t* timestamp_us = NULL) = 0;

    virtual bool getNextFrameCv  (IplImage* image, uint64_t* timestamp_us = NULL)
    {
        return getNextFrame(getRawBuffer(image), timestamp_us);
    }
    virtual bool getLatestFrameCv(IplImage* image, uint64_t* timestamp_us = NULL)
    {
        return getLatestFrame(getRawBuffer(image), timestamp_us);
    }
    virtual bool getNextFrameCv  (CvMat* image, uint64_t* timestamp_us = NULL)
    {
        return getNextFrame(getRawBuffer(image), timestamp_us);
    }
    virtual bool getLatestFrameCv(CvMat* image, uint64_t* timestamp_us = NULL)
    {
        return getLatestFrame(getRawBuffer(image), timestamp_us);
    }


    // Instead of accessing the frame with blocking I/O, the frame source can spawn a thread to wait
    // for the frames, and callback when a new frame is available. The thread can optionally wait
    // some amount of time between successive frames to force-limit the framerate by giving a
    // non-zero frameWait_us. In this case only the latest available frame will be returned with the
    // rest thrown away. Otherwise, we try not to miss frames by returning the next available frame.
    //
    // The callback is passed the buffer where the data was stored. Note that this is the same
    // buffer that was originally passed to startSourceThread. Note that this buffer is accessed
    // asynchronously, so the caller can NOT assume it contains valid data outside of the callback

    void startSourceThreadCv(FrameSourceCallbackCv_t* callback, uint64_t frameWait_us,
                             CvMat* buffer)
    {
        if(sourceThread_id != 0)
            return;

        sourceThread_callbackCv   = callback;
        sourceThread_frameWait_us = frameWait_us;
        sourceThread_bufferCv     = buffer;

        if(pthread_create(&sourceThread_id, NULL, &sourceThreadCv_global, this) != 0)
        {
            sourceThread_id = 0;
            fprintf(stderr, "couldn't start source thread\n");
        }
    }

    void startSourceThread(FrameSourceCallback_t* callback, uint64_t frameWait_us,
                           unsigned char* buffer)
    {
        if(sourceThread_id != 0)
            return;

        sourceThread_callback     = callback;
        sourceThread_frameWait_us = frameWait_us;
        sourceThread_buffer       = buffer;

        if(pthread_create(&sourceThread_id, NULL, &sourceThread_global, this) != 0)
        {
            sourceThread_id = 0;
            fprintf(stderr, "couldn't start source thread\n");
        }
    }

    void sourceThread(bool isCv)
    {
        while(1)
        {
            uint64_t timestamp_us;
            bool result;
            if(sourceThread_frameWait_us != 0)
            {
                // We are limiting the framerate. Sleep for a bit, then return the newest frame in
                // the buffer, throwing away the rest
                struct timespec delay;
                delay.tv_sec  = sourceThread_frameWait_us / 1000000;
                delay.tv_nsec = (sourceThread_frameWait_us - delay.tv_sec*1000000) * 1000;
                nanosleep(&delay, NULL);

                if(isCv)
                    result = getLatestFrameCv(sourceThread_bufferCv, &timestamp_us);
                else
                    result = getLatestFrame  (sourceThread_buffer, &timestamp_us);
            }
            else
            {
                // We are not limiting the framerate. Try to return ALL the available frames
                if(isCv)
                    result = getNextFrameCv(sourceThread_bufferCv, &timestamp_us);
                else
                    result = getNextFrame  (sourceThread_buffer, &timestamp_us);
            }

            if(!result)
            {
                fprintf(stderr, "thread couldn't get frame\n");
                return;
            }

            if(isCv)
                (*sourceThread_callbackCv)(sourceThread_bufferCv, timestamp_us);
            else
                (*sourceThread_callback)  (sourceThread_buffer, timestamp_us);
        }
    }

    unsigned int w() { return width;  }
    unsigned int h() { return height; }
};

static void* sourceThread_global(void *pArg)
{
    FrameSource* source = (FrameSource*)pArg;
    source->sourceThread(false);
    return NULL;
}

static void* sourceThreadCv_global(void *pArg)
{
    FrameSource* source = (FrameSource*)pArg;
    source->sourceThread(true);
    return NULL;
}

#endif
