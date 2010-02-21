#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <string>

#include "dc1394/dc1394.h"
#include "frameSource.hh"

class Camera : public FrameSource
{
    unsigned             cameraIndex;
    dc1394camera_t*      camera;
    dc1394video_frame_t* cameraFrame;

    std::string          cameraDescription;

    uint64_t             frame0Timestamp;

    // These describe the whole camera bus, not just a single camera. Thus we keep only one copy by
    // declaring them static
    static dc1394_t*            dc1394Context;
    static dc1394camera_list_t* cameraList;
    static unsigned int         numInitedCameras;

public:
    Camera(bool _isColor);
    ~Camera();

    // peek...Frame() blocks until a frame is available. A pointer to the internal buffer is
    // returned (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed.
    // peekNextFrame() returns the next frame in the buffer.
    // peekMostRecentFrame() purges the buffer and returns the most recent frame availabl
    unsigned char* peekNextFrame      (uint64_t* timestamp_us);
    unsigned char* peekMostRecentFrame(uint64_t* timestamp_us);
    void unpeekFrame(void);

    int getCameraIndex(void)                { return cameraIndex;       }
    const std::string& getDescription(void) { return cameraDescription; }

    static bool uninitedCamerasLeft(void)   { return numInitedCameras < cameraList->num; }
};

#endif
