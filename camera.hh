#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "stdlib.h"
#include "camwire.h"

class Camera
{
    camwire_bus_handle* cameraHandle;
    bool inited;

    unsigned char* frame;

public:
    Camera(int cameraIndex);
    ~Camera();

    operator bool() { return inited; }
    unsigned char* getFrame(struct timespec* timestamp = NULL);
    unsigned char* getFrameBuffer(void) { return frame; }
};

#endif
