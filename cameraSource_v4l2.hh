#ifndef __CAMERA_SOURCE_HH__
#define __CAMERA_SOURCE_HH__

#include <string>
#include "frameSource.hh"

class CameraSource_V4L2 : public FrameSource
{
    struct
    {
        void*  start;
        size_t length;
    }* buffers;

    int      camera_fd;
    unsigned numBuffers;

public:
    CameraSource_V4L2(FrameSource_UserColorChoice _userColorMode,
                      char* device,
                      CvRect _cropRect = cvRect(-1, -1, -1, -1),
                      double scale = 1.0);

    ~CameraSource_V4L2();

    operator bool() { return camera_fd > 0; }

private:
    void uninit(void);


    // These functions implement the FrameSource virtuals, and are the main differentiators between
    // the various frame sources, along with the constructor and destructor
private:
    bool _getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL);
    bool _getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL);

    bool _stopStream   (void);
    bool _resumeStream (void);

    // There's no concept of rewinding in a real camera, so restart == resume
    bool _restartStream(void)
    {
        return _resumeStream();
    }
};

#endif
