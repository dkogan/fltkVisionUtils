#include <time.h>
#include <iostream>
#include "frameSource.hh"
using namespace std;

FrameSource::FrameSource (FrameSource_UserColorChoice _userColorMode)
    : userColorMode(_userColorMode),
      cropRect( cvRect(-1, -1, -1, -1) ),
      preCropScaleBuffer(NULL),
      sourceThread_id(0)
{
    // we're not yet initialized and thus not able to serve data
    isRunningNow.reset();
}

void FrameSource::setupCroppingScaling(CvRect _cropRect, double scale)
{
    cropRect = _cropRect;

    if( (cropRect.width > 0 && cropRect.height > 0) ||
        scale != 1.0 )
    {
        // if we're cropping or scaling (or both), set up the temporary buffer image
        preCropScaleBuffer = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U,
                                           userColorMode == FRAMESOURCE_COLOR ? 3 : 1);
    }

    if(cropRect.width > 0 && cropRect.height > 0)
    {
        width  = lround(scale * (double)cropRect.width);
        height = lround(scale * (double)cropRect.height);
    }
    else
    {
        width  = lround(scale * (double)width);
        height = lround(scale * (double)height);
    }
}

void FrameSource::applyCroppingScaling(IplImage* src, IplImage* dst)
{
    if(cropRect.width > 0 && cropRect.height > 0)
        cvSetImageROI(src, cropRect);

#warning can swscale do this faster? I dont completely trust opencv to do this optimally
    cvResize(src, dst, CV_INTER_CUBIC);

    cvResetImageROI(src);
}

void FrameSource::cleanupThreads(void)
{
    if(sourceThread_id != 0)
    {
        pthread_cancel(sourceThread_id);
        pthread_join(sourceThread_id, NULL);
        sourceThread_id = 0;
    }
}

FrameSource::~FrameSource()
{
    // This should be called from the most derived class since the thread calls a virtual
    // function. Just in case we call it here also. This will do nothing if the derived class
    // already did this, but may crash if it didn't
    cleanupThreads();

    if(preCropScaleBuffer != NULL)
    {
        cvReleaseImage(&preCropScaleBuffer);
        preCropScaleBuffer = NULL;
    }
}

bool FrameSource::getNextFrame  (IplImage* image, uint64_t* timestamp_us)
{
    isRunningNow.waitForTrue();
    return _getNextFrame(image, timestamp_us);
}

bool FrameSource::getLatestFrame(IplImage* image, uint64_t* timestamp_us)
{
    isRunningNow.waitForTrue();
    return _getLatestFrame(image, timestamp_us);
}

// tell the source to stop sending data. Any queued, but not processed frames are discarded
bool FrameSource::stopStream(void)
{
    isRunningNow.reset();
    return _stopStream();
}

// re-activate the stream, rewinding to the beginning if asked (restart) and if possible. This
// is context-dependent. Video sources can rewind, but cameras cannot, for instance
bool FrameSource::resumeStream (void)
{
    if(_resumeStream())
    {
        isRunningNow.setTrue();
        return true;
    }
    return false;
}

bool FrameSource::restartStream(void)
{
    if(_restartStream())
    {
        isRunningNow.setTrue();
        return true;
    }
    return false;
}
// Instead of accessing the frame with blocking I/O, the frame source can spawn a thread to wait
// for the frames, and callback when a new frame is available. The thread can optionally wait
// some amount of time between successive frames to force-limit the framerate by giving a
// non-zero frameWait_us. In this case only the latest available frame will be returned with the
// rest thrown away. Otherwise, we try not to miss frames by returning the next available frame.
//
// The callback is passed the buffer where the data was stored. Note that this is the same
// buffer that was originally passed to startSourceThread. Note that this buffer is accessed
// asynchronously, so the caller can NOT assume it contains valid data outside of the callback.
//
// If there was an error reading the frame, I call-back ONCE with buffer==NULL, and exit the thread

static void* sourceThread_global(void *pArg)
{
    FrameSource* source = (FrameSource*)pArg;
    source->sourceThread();
    return NULL;
}

void FrameSource::startSourceThread(FrameSourceCallback_t* callback, uint64_t frameWait_us,
                                    IplImage* buffer)
{
    if(sourceThread_id != 0)
        return;

    sourceThread_callback     = callback;
    sourceThread_frameWait_us = frameWait_us;
    sourceThread_buffer       = buffer;

    if(pthread_create(&sourceThread_id, NULL, &sourceThread_global, this) != 0)
    {
        sourceThread_id = 0;
        cerr << "couldn't start source thread" << endl;
    }
}

void FrameSource::sourceThread(void)
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

            result = getLatestFrame(sourceThread_buffer, &timestamp_us);
        }
        else
        {
            // We are not limiting the framerate. Try to return ALL the available frames
            result = getNextFrame(sourceThread_buffer, &timestamp_us);
        }

        if(!result)
        {
            cerr << "thread couldn't get frame" << endl;
            (*sourceThread_callback)(NULL, timestamp_us);
            return;
        }

        (*sourceThread_callback)(sourceThread_buffer, timestamp_us);
    }
}
