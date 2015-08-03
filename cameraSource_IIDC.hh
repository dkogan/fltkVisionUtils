#pragma once

#include <string>
#include <dc1394/dc1394.h>
#include "frameSource.hh"

class CameraSource_IIDC : public FrameSource
{
    bool                 inited;

    dc1394camera_t*      camera;
    dc1394video_frame_t* cameraFrame;
    dc1394video_mode_t   cameraVideoMode;
    dc1394color_coding_t cameraColorCoding;

    std::string          cameraDescription;

    // These describe the whole camera bus, not just a single camera. Thus we keep only one copy by
    // declaring them static
    static dc1394_t*            dc1394Context;
    static dc1394camera_list_t* cameraList;
    static unsigned int         numInitedCameras;

    unsigned char* finishPeek(uint64_t* timestamp_us);
    bool finishGet(IplImage* image);

    // These private versions of the peek() functions contain 99% of the functionality. The public
    // functions perform some checks to make sure it is valid to use these at all.
    void beginPeek(void);

    void unpeekFrame(void);

    bool purgeBuffer(void);

public:
    // The firewire stack may still have resources allocated from previous usage. I have no way to
    // tell zombie resources from legitimate ones. If resetbus==true, I clear out all the existing
    // firewire resources. This will make room for this app, but if there's another program reading
    // the bus at the same time (even if it's reading a different camera) that program will stop
    // working.
    CameraSource_IIDC(FrameSource_UserColorChoice _userColorMode,
                      bool resetbus = false,
                      uint64_t guid = 0, // guid == 0 -> find the next available camera
                      CvRect _cropRect = cvRect(-1, -1, -1, -1),
                      double scale = 1.0);

    ~CameraSource_IIDC();

    operator bool            () { return inited; }
    operator dc1394camera_t* () { return camera; }

    const std::string& getDescription(void) { return cameraDescription; }

    static bool uninitedCamerasLeft(void)
    {
        // if we don't yet have a camera list, say there are cameras left to try to open them
        return cameraList == NULL ||
            numInitedCameras < cameraList->num;
    }

    // These functions implement the FrameSource virtuals, and are the main differentiators between
    // the various frame sources, along with the constructor and destructor
private:
    bool _getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL);
    bool _getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL);

    bool _stopStream   (void)
    {
        if(DC1394_SUCCESS == dc1394_video_set_transmission(camera, DC1394_OFF))
        {
            purgeBuffer();
            return true;
        }
        return false;
    }
    bool _resumeStream (void)
    {
        purgeBuffer();
        return DC1394_SUCCESS == dc1394_video_set_transmission(camera, DC1394_ON);
    }

    // There's no concept of rewinding in a real camera, so restart == resume
    bool _restartStream(void)
    {
        return _resumeStream();
    }
};
