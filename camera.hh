#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "stdlib.h"
#include "dc1394/dc1394.h"

class Camera
{
    bool inited;

    unsigned int                width, height;
    unsigned                    cameraIndex;
    dc1394camera_t*             camera;
    dc1394video_frame_t*        cameraFrame;

    static dc1394_t*            dc1394Context;
    static dc1394camera_list_t* cameraList;
    static int                  numInitedCameras;

public:
    Camera(unsigned _cameraIndex);
    ~Camera();

    operator bool() { return inited; }

    // returns a pointer to the raw frame data. This data should not be modified and releaseFrame()
    // should be called when we're done
    unsigned char* getFrame(uint64_t* timestamp_us);
    void releaseFrame(void);

    int idx(void) { return cameraIndex; }
};

#endif
