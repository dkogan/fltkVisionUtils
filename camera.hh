#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "dc1394/dc1394.h"
#include "frameSource.hh"

class Camera : public FrameSource
{
    unsigned                    cameraIndex;
    dc1394camera_t*             camera;
    dc1394video_frame_t*        cameraFrame;

    static dc1394_t*            dc1394Context;
    static dc1394camera_list_t* cameraList;
    static int                  numInitedCameras;

    void flushFrameBuffer(void);

public:
    Camera(unsigned _cameraIndex);
    ~Camera();

    // peekFrame() blocks until a frame is available. A pointer to the internal buffer is returned
    // (NULL on error). This buffer must be given back to the system by calling
    // unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed
    unsigned char* peekFrame(uint64_t* timestamp_us);
    void unpeekFrame(void);

    int idx(void) { return cameraIndex; }
};

#endif
