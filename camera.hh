#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "stdlib.h"
#include "dc1394/dc1394.h"

class Camera
{
    bool inited;

    unsigned char* frame;

    unsigned int                width, height;
    unsigned                    cameraIndex;
    dc1394camera_t*             camera;
    static dc1394_t*            dc1394Context;
    static dc1394camera_list_t* cameraList;
    static int                  numInitedCameras;

public:
    Camera(unsigned _cameraIndex);
    ~Camera();

    operator bool() { return inited; }
    unsigned char* getFrame(uint64_t* timestamp_us);
    unsigned char* getFrameBuffer(void) { return frame; }
    int idx(void) { return cameraIndex; }
};

#endif
