#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "stdlib.h"
#include "camwire.h"

class Camera
{
    camwire_bus_handle* cameraHandle;
    bool inited;

    unsigned char* frame;

    int cameraIndex;

public:
    Camera(int _cameraIndex);
    ~Camera();

    operator bool() { return inited; }
    unsigned char* getFrame(struct timespec* timestamp = NULL);
    unsigned char* getFrameBuffer(void) { return frame; }
    int idx(void) { return cameraIndex; }
};

#endif
