#include <stdio.h>
#include <limits.h>
#include "camera.hh"

#define BYTES_PER_PIXEL 1
// These describe the whole camera bus, not just a single camera. Thus we keep only one copy by
// declaring them static members
static dc1394_t*            Camera::dc1394Context    = NULL;
static dc1394camera_list_t* Camera::cameraList       = NULL;
static int                  Camera::numInitedCameras = 0;

// The resolutions that I know about. These are listed in order from least to most desireable
enum { MODE_UNWANTED,
       MODE_160x120,
       MODE_320x240,
       MODE_640x480,
       MODE_800x600,
       MODE_1024x768,
       MODE_1280x960,
       MODE_1600x1200} resolution_t;

static resolution_t getResolution(dc1394video_mode_t mode)
{
    // I only look at the modes that were defined in libdc1394 as of version 2.1.2-1 of the
    // libdc1394-22 debian package (~ 2/2010). I explicitly ignore format 7 since I don't want to
    // bother supporting it until I need it

    switch(mode)
    {
    case DC1394_VIDEO_MODE_160x120_YUV444:
        return MODE_160x120;

    case DC1394_VIDEO_MODE_320x240_YUV422:
        return MODE_320x240;

    case DC1394_VIDEO_MODE_640x480_MONO8:
    case DC1394_VIDEO_MODE_640x480_MONO16:
    case DC1394_VIDEO_MODE_640x480_YUV411:
    case DC1394_VIDEO_MODE_640x480_YUV422:
    case DC1394_VIDEO_MODE_640x480_RGB8:
        return MODE_640x480;

    case DC1394_VIDEO_MODE_800x600_MONO8:
    case DC1394_VIDEO_MODE_800x600_MONO16:
    case DC1394_VIDEO_MODE_800x600_YUV422:
    case DC1394_VIDEO_MODE_800x600_RGB8:
        return MODE_800x600;

    case DC1394_VIDEO_MODE_1024x768_YUV422:
    case DC1394_VIDEO_MODE_1024x768_RGB8:
    case DC1394_VIDEO_MODE_1024x768_MONO8:
    case DC1394_VIDEO_MODE_1024x768_MONO16:
        return MODE_1024x768;

    case DC1394_VIDEO_MODE_1280x960_YUV422:
    case DC1394_VIDEO_MODE_1280x960_RGB8:
    case DC1394_VIDEO_MODE_1280x960_MONO8:
    case DC1394_VIDEO_MODE_1280x960_MONO16:
        return MODE_1280x960;

    case DC1394_VIDEO_MODE_1600x1200_YUV422:
    case DC1394_VIDEO_MODE_1600x1200_RGB8:
    case DC1394_VIDEO_MODE_1600x1200_MONO8:
    case DC1394_VIDEO_MODE_1600x1200_MONO16:
        return MODE_1600x1200;

    default:
        return MODE_UNWANTED;
    }
}

// The colormodes that I know about. These are listed in order from least to most desireable
enum { COLORMODE_UNWANTED,
       COLORMODE_MONO8,
       COLORMODE_MONO16,
       COLORMODE_YUV411,
       COLORMODE_YUV422,
       COLORMODE_YUV444,
       COLORMODE_RGB8} colormode_t;

static colormode_t getColormode(dc1394video_mode_t mode)
{
    // I only look at the modes that were defined in libdc1394 as of version 2.1.2-1 of the
    // libdc1394-22 debian package (~ 2/2010). I explicitly ignore format 7 since I don't want to
    // bother supporting it until I need it

    switch(mode)
    {
    case DC1394_VIDEO_MODE_640x480_MONO8:
    case DC1394_VIDEO_MODE_800x600_MONO8:
    case DC1394_VIDEO_MODE_1024x768_MONO8:
    case DC1394_VIDEO_MODE_1280x960_MONO8:
    case DC1394_VIDEO_MODE_1600x1200_MONO8:
        return COLORMODE_MONO8;

    case DC1394_VIDEO_MODE_640x480_MONO16:
    case DC1394_VIDEO_MODE_800x600_MONO16:
    case DC1394_VIDEO_MODE_1024x768_MONO16:
    case DC1394_VIDEO_MODE_1280x960_MONO16:
    case DC1394_VIDEO_MODE_1600x1200_MONO16:
        return COLORMODE_MONO16;

    case DC1394_VIDEO_MODE_640x480_YUV411:
        return COLORMODE_YUV411;

    case DC1394_VIDEO_MODE_320x240_YUV422:
    case DC1394_VIDEO_MODE_640x480_YUV422:
    case DC1394_VIDEO_MODE_800x600_YUV422:
    case DC1394_VIDEO_MODE_1024x768_YUV422:
    case DC1394_VIDEO_MODE_1280x960_YUV422:
    case DC1394_VIDEO_MODE_1600x1200_YUV422:
        return COLORMODE_YUV422;

    case DC1394_VIDEO_MODE_160x120_YUV444:
        return COLORMODE_YUV444;

    case DC1394_VIDEO_MODE_640x480_RGB8:
    case DC1394_VIDEO_MODE_800x600_RGB8:
    case DC1394_VIDEO_MODE_1024x768_RGB8:
    case DC1394_VIDEO_MODE_1280x960_RGB8:
    case DC1394_VIDEO_MODE_1600x1200_RGB8:
        return COLORMODE_RGB8;

    default:
        return COLORMODE_UNWANTED;
    }
}

Camera::Camera()
    : camera(NULL), cameraFrame(NULL)
{
    if(!uninitedCamerasLeft())
    {
        fprintf(stderr, "no more cameras left to init\n");
        return;
    }

    cameraIndex = numInitedCameras;

    dc1394error_t err;

    if(dc1394Context == NULL)
    {
        dc1394Context = dc1394_new();
        if (!dc1394Context)
            return;

        err = dc1394_camera_enumerate (dc1394Context, &cameraList);
        DC1394_ERR(err,"Failed to enumerate cameras");

        if(cameraList->num == 0)
        {
            dc1394_log_error("No cameras found");
            return;
        }
    }

    camera = dc1394_camera_new(dc1394Context, cameraList->ids[cameraIndex].guid);
    if (!camera)
    {
        dc1394_log_error("Failed to initialize camera with guid %ld", cameraList->ids[cameraIndex].guid);
        return;
    }

    // I use the libdc1394 function to report the information about this camera. The library can
    // only output this to a FILE structure, so I jump through hoops a bit to get this into my C++
    // string instead
    char*  cameraInfoStr;
    size_t cameraInfoSize;
    FILE*  cameraInfo = open_memstream(&cameraInfoStr, &cameraInfoSize);
    err = dc1394_camera_print_info(camera, cameraInfo);
    fclose(cameraInfo);
    DC1394_ERR_CLN(err, free(cameraInfoStr), "Couldn't get the camera information");
    cameraDescription = cameraInfoStr;
    cameraDescription += "\n";
    free(cameraInfoStr);

    // Release the resources that could have been allocated by previous instances of a program using
    // the cameras. I don't know which channels and how much bandwidth was allocated, so release
    // EVERYTHING. This can generate bogus error messages, but these should be ignored
    static bool doneOnce = false;
    if(!doneOnce)
    {
        doneOnce = true;
        fprintf(stderr, "cleaning up ieee1394 stack. This may cause error messages...\n");
        dc1394_iso_release_bandwidth(camera, INT_MAX);
        for (int channel = 0; channel < 64; channel++)
            dc1394_iso_release_channel(camera, channel);
    }

    // I am now configuring the camera. Right now this is hardcoded to pick the highest available
    // spatial resolution then the best color resolution then the highest framerate

    // Resolution. For simplicity I currently avoid mode 7 (scalable video) since it requires
    // more configuration. I will add it later when/if I need it
    dc1394video_modes_t  video_modes;
    dc1394video_mode_t   video_mode;

    err = dc1394_video_get_supported_modes(camera, &video_modes);
    DC1394_ERR(err, "Can't get video modes");

    resolution_t bestRes = MODE_UNWANTED;
    colormode_t bestColormode = COLORMODE_UNWANTED;
    int bestModeIdx = -1;
    for (int i=0; i<video_modes.num; i++)
    {
        resolution_t res = getResolution(video_modes.modes[i]);
        colormode_t colormode = getColormode(video_modes.modes[i]);
        if(res > bestRes)
        {
            if(colormode != COLORMODE_UNWANTED)
            {
                bestRes = res;
                bestColormode = colormode_unwanted;
                bestModeIdx = i;
            }
        }
        else if(res == bestRes)
            if(colormode > bestColormode)
            {
                bestColormode = colormode;
                bestModeIdx = i;
            }
    }
    if(bestModeIdx == -1)
    {
        fprintf(stderr, "No known resolutions/colormodes supported. Maybe this is a format7-only camera?\n");
        return;
    }
    video_mode = video_modes.modes[bestModeIdx];

    // get the highest framerate
    dc1394framerates_t framerates;
    dc1394framerate_t  bestFramerate = DC1394_FRAMERATE_MIN;
    err = dc1394_video_get_supported_framerates(camera, video_mode, &framerates);
    DC1394_ERR(err, "Could not get framerates");
    for(int i=0; i<framerates.num; i++)
    {
        // the framerates defined by the dc1394framerate_t enum are sorted from worst to best, so I
        // just loop through and pick the highest one
        if(framerates.framerates[i] > bestFramerate)
            bestFramerate = framerates.framerates[i];
    }

    // setup capture. Use the 1394-B mode if possible
    if( dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B) == DC1394_SUCCESS)
    {
        err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_800);
        DC1394_ERR(err,"Could not set iso speed");
    }
    else
    {
        err = dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_LEGACY);
        DC1394_ERR(err,"Could not set operation mode");

        err = dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
        DC1394_ERR(err,"Could not set iso speed");
    }

    err = dc1394_video_set_mode(camera, video_mode);
    DC1394_ERR(err,"Could not set video mode");

    err = dc1394_video_set_framerate(camera, bestFramerate);
    DC1394_ERR(err,"Could not set framerate");

    // I always want only the latest frame available. I.e. I don't really care if I missed some
    // frames. So I use a single DMA buffer
    err = dc1394_capture_setup(camera, 1, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR(err,"Could not setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera");


    // have the camera start sending us data
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR(err,"Could not start camera iso transmission");

    dc1394_get_image_size_from_video_mode(camera, video_mode, &width, &height);
    bitsPerPixel = BYTES_PER_PIXEL*8;

    inited = true;
    numInitedCameras++;

    fprintf(stderr, "init done\n");
}

Camera::~Camera(void)
{
    if (camera != NULL)
    {
        dc1394_video_set_transmission(camera, DC1394_OFF);
        dc1394_capture_stop(camera);
        dc1394_camera_free(camera);
        camera = NULL;
    }

    if(inited)
        numInitedCameras--;

    if(dc1394Context != NULL && numInitedCameras <= 0)
    {
        dc1394_free(dc1394Context);
        dc1394Context = NULL;
        dc1394_camera_free_list (cameraList);
    }
}

// Fetches all of the frames in the buffer and throws them away. If we do this we guarantee that the
// next frame read will be the most recent
void Camera::flushFrameBuffer(void)
{
    dc1394error_t err;
    while(true)
    {
        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &cameraFrame);

        if( err != DC1394_SUCCESS )
        {
            dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                               dc1394_error_get_string(err),
                               __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        if(cameraFrame == NULL)
            break;

        err = dc1394_capture_enqueue(camera, cameraFrame);
        if( err != DC1394_SUCCESS )
        {
            dc1394_log_warning("%s: in %s (%s, line %d): Could not enqueue\n",
                               dc1394_error_get_string(err),
                               __FUNCTION__, __FILE__, __LINE__);
            return;
        }
        cameraFrame = NULL;
    }
}

// peekFrame() blocks until a frame is available. A pointer to the internal buffer is returned
// (NULL on error). This buffer must be given back to the system by calling
// unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed
unsigned char* Camera::peekFrame(uint64_t* timestamp_us)
{
    static uint64_t timestamp0 = 0;

    if(cameraFrame != NULL)
    {
        fprintf(stderr, "error: peekFrame() before unpeekFrame()\n");
        return NULL;
    }

    dc1394error_t err;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &cameraFrame);

    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return NULL;
    }

    if(timestamp0 == 0)
        timestamp0 = cameraFrame->timestamp;

    if(timestamp_us != NULL)
        *timestamp_us = cameraFrame->timestamp - timestamp0;

    return cameraFrame->image;
}

void Camera::unpeekFrame(void)
{
    if(cameraFrame == NULL)
        return;

    dc1394error_t err;
    err = dc1394_capture_enqueue(camera, cameraFrame);
    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not enqueue\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return;
    }

    cameraFrame = NULL;
}
