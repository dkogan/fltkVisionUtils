#ifndef __CAMERA_SOURCE_HH__
#define __CAMERA_SOURCE_HH__

#include <asm/types.h>
#include <linux/videodev2.h>

#include <string>
#include "frameSource.hh"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}

class CameraSource_V4L2 : public FrameSource
{
    int             camera_fd;
    v4l2_pix_format pixfmt;
    unsigned char*  buffer;
    SwsContext*     scaleContext;

    AVCodecContext* codecContext;
    AVFrame*        ffmpegFrame;

public:
    CameraSource_V4L2(FrameSource_UserColorChoice _userColorMode,
                      const char* device,
                      CvRect _cropRect = cvRect(-1, -1, -1, -1),
                      double scale = 1.0);

    ~CameraSource_V4L2();

    operator bool() { return camera_fd > 0; }

private:
    void uninit(void);

    bool setupSwsContext(enum PixelFormat swscalePixfmt);
    bool findDecoder(void);

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
