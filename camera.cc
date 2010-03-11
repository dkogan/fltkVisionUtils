#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <sstream>
#include "camera.hh"

// These describe the whole camera bus, not just a single camera. Thus we keep only one copy by
// declaring them static members
dc1394_t*            Camera::dc1394Context    = NULL;
dc1394camera_list_t* Camera::cameraList       = NULL;
unsigned int         Camera::numInitedCameras = 0;

// returns desireability of the resolution. Higher is more desireable
resolution_t Camera::getResolutionWorth(dc1394video_mode_t mode)
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

// returns desireability of the color mode. Higher is more desireable
colormode_t Camera::getColormodeWorth(dc1394video_mode_t mode)
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
        return userColorMode==FRAMESOURCE_COLOR ? COLORMODE_MONO8 : COLORMODE_MONO8_REQUESTED;

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

Camera::Camera(FrameSource_UserColorChoice _userColorMode)
    : FrameSource(_userColorMode), inited(false), camera(NULL), cameraFrame(NULL)
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

    err = dc1394_video_get_supported_modes(camera, &video_modes);
    DC1394_ERR(err, "Can't get video modes");

    resolution_t bestRes = MODE_UNWANTED;
    colormode_t bestColormode = COLORMODE_UNWANTED;
    int bestModeIdx = -1;
    for (unsigned int i=0; i<video_modes.num; i++)
    {
        resolution_t res = getResolutionWorth(video_modes.modes[i]);
        colormode_t colormode = getColormodeWorth(video_modes.modes[i]);
        if(res > bestRes)
        {
            if(colormode != COLORMODE_UNWANTED)
            {
                bestRes = res;
                bestColormode = colormode;
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
    cameraVideoMode = video_modes.modes[bestModeIdx];

    // get the highest framerate
    dc1394framerates_t framerates;
    dc1394framerate_t  bestFramerate = DC1394_FRAMERATE_MIN;
    err = dc1394_video_get_supported_framerates(camera, cameraVideoMode, &framerates);
    DC1394_ERR(err, "Could not get framerates");
    for(unsigned int i=0; i<framerates.num; i++)
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

    err = dc1394_video_set_mode(camera, cameraVideoMode);
    DC1394_ERR(err,"Could not set video mode");

    err = dc1394_video_set_framerate(camera, bestFramerate);
    DC1394_ERR(err,"Could not set framerate");

    // Using 5 frame buffers. This should work for many applications
    err = dc1394_capture_setup(camera, 5, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR(err,"Could not setup camera...");


    // have the camera start sending us data
    err = dc1394_video_set_transmission(camera, DC1394_ON);
    DC1394_ERR(err,"Could not start camera iso transmission");

    // get the dimensions of the frames. Not writing directly to width/height because I want to be
    // absolutely certain that the pointers I'm passing in are pointing to the correct data type
    uint32_t w, h;
    dc1394_get_image_size_from_video_mode(camera, cameraVideoMode, &w, &h);
    width  = w;
    height = h;

    inited = true;
    numInitedCameras++;


    // Now get the information about my camera and its setup into a string
    std::ostringstream descriptionStream;

    // I use the libdc1394 function to report the information about this camera. The library can
    // only output this to a FILE structure, so I jump through hoops a bit to get this into my C++
    // string instead
    char*  cameraInfoStr;
    size_t cameraInfoSize;
    FILE*  cameraInfo = open_memstream(&cameraInfoStr, &cameraInfoSize);
    err = dc1394_camera_print_info(camera, cameraInfo);
    fclose(cameraInfo);
    DC1394_ERR_CLN(err, free(cameraInfoStr), "Couldn't get the camera information");
    descriptionStream << cameraInfoStr << std::endl;
    free(cameraInfoStr);

    descriptionStream << "Resolution: " << width << ' ' << height << std::endl;

    float framerate;
    dc1394_framerate_as_float(bestFramerate, &framerate);
    descriptionStream << "Framerate: " << framerate << " frames per second" << std::endl;

    descriptionStream << "Color coding: ";
    dc1394_get_color_coding_from_video_mode(camera, cameraVideoMode, &cameraColorCoding);

#define COLOR_CODING_SWITCH_PRINT(c) case c: descriptionStream << #c; break
    switch(cameraColorCoding)
    {
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_MONO8 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_YUV411 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_YUV422 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_YUV444 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_RGB8 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_MONO16 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_RGB16 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_MONO16S );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_RGB16S );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_RAW8 );
        COLOR_CODING_SWITCH_PRINT( DC1394_COLOR_CODING_RAW16);

    default:
        descriptionStream << "Unknown!" << std::endl;
    }
    descriptionStream << std::endl;

    cameraDescription = descriptionStream.str();

    fprintf(stderr, "init done\n");
}

Camera::~Camera(void)
{
    cleanupThreads();

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

// peekNextFrame() blocks until a frame is available. A pointer to the internal buffer is returned
// (NULL on error). This buffer must be given back to the system by calling
// unpeekFrame(). unpeekFrame() need not be called if peekFrame() failed
unsigned char* Camera::_peekNextFrame(uint64_t* timestamp_us)
{
    beginPeek();

    dc1394error_t err;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &cameraFrame);

    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return NULL;
    }

    return finishPeek(timestamp_us);
}

// peekLatestFrame() checks the frame buffer. If there are no frames in it, it blocks until a
// frame is available. If there are frames, the buffer is purged and the most recent frame is
// returned. A pointer to the internal buffer is returned (NULL on error). This buffer must be given
// back to the system by calling unpeekFrame(). unpeekFrame() need not be called if peekFrame()
// failed
unsigned char* Camera::_peekLatestFrame(uint64_t* timestamp_us)
{
    beginPeek();

    // first, poll the buffer. If no frames are available, use the plain peekNextFrame() call to
    // wait for one
    dc1394error_t err;
    dc1394video_frame_t* polledFrame;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &polledFrame);
    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return NULL;
    }
    if(polledFrame == NULL)
        return _peekNextFrame(timestamp_us);


    // A frame was available, so I pull the frames off until I reach the end
    while(true)
    {
        // at this point we have one dequeued frame and it's in polledFrame. I try to dequeue one
        // more to see if it's the last

        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &cameraFrame);
        if( err != DC1394_SUCCESS )
        {
            dc1394_capture_enqueue(camera, polledFrame);
            dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                               dc1394_error_get_string(err),
                               __FUNCTION__, __FILE__, __LINE__);
            return NULL;
        }
        if(cameraFrame == NULL)
        {
            // no new frame, so polledFrame is the last
            cameraFrame = polledFrame;
            break;
        }

        // I just dequeued a frame, so polledFrame wasn't the last. I enqueue the polledFrame, and
        // point it to my new most-recent frame
        err = dc1394_capture_enqueue(camera, polledFrame);
        if( err != DC1394_SUCCESS )
        {
            dc1394_log_warning("%s: in %s (%s, line %d): Could not enqueue\n",
                               dc1394_error_get_string(err),
                               __FUNCTION__, __FILE__, __LINE__);
            return NULL;
        }
        polledFrame = cameraFrame;
    }

    return finishPeek(timestamp_us);
}

void Camera::beginPeek(void)
{
    if(cameraFrame != NULL)
    {
        fprintf(stderr, "warning: peekNextFrame() before unpeekFrame()\n"
                "Calling unpeekFrame() for you, but you should do this yourself\n"
                "as soon as you're done with the data\n");
        unpeekFrame();
    }
}

bool Camera::isOKtoPeek(void)
{
    if( (userColorMode == FRAMESOURCE_COLOR     && cameraColorCoding != DC1394_COLOR_CODING_RGB8) ||
        (userColorMode == FRAMESOURCE_GRAYSCALE && cameraColorCoding != DC1394_COLOR_CODING_MONO8) )
    {
        fprintf(stderr, "Camera::peek..() can only be used if the requested color mode exactly\n"
                "matches the color mode of the camera output. Change either of the modes, or use get() instead of peek()\n");
        return false;
    }
    return true;
}

unsigned char* Camera::peekNextFrame  (uint64_t* timestamp_us)
{
    if(!isOKtoPeek()) return NULL;
    return _peekNextFrame(timestamp_us);
}

unsigned char* Camera::peekLatestFrame(uint64_t* timestamp_us)
{
    if(!isOKtoPeek()) return NULL;
    return _peekLatestFrame(timestamp_us);
}

unsigned char* Camera::finishPeek(uint64_t* timestamp_us)
{
    if(timestamp_us != NULL)
        *timestamp_us = cameraFrame->timestamp;

    return cameraFrame->image;
}

bool Camera::getNextFrame(unsigned char* buffer, uint64_t* timestamp_us)
{
    if(_peekNextFrame(timestamp_us) == NULL)
        return false;

    return finishGet(buffer);
}

bool Camera::getLatestFrame(unsigned char* buffer, uint64_t* timestamp_us)
{
    if(_peekLatestFrame(timestamp_us) == NULL)
        return false;

    return finishGet(buffer);
}

bool Camera::finishGet(unsigned char* buffer)
{
    dc1394error_t err;
    if(userColorMode == FRAMESOURCE_COLOR)
        err = dc1394_convert_to_RGB8(cameraFrame->image,
                                     buffer,
                                     cameraFrame->size[0], cameraFrame->size[1],
                                     cameraFrame->yuv_byte_order,
                                     cameraFrame->color_coding,
                                     0 // supposedly useful for 16-bit formats only, so I don't care
                                     );
    else
        err = dc1394_convert_to_MONO8(cameraFrame->image,
                                      buffer,
                                      cameraFrame->size[0], cameraFrame->size[1],
                                      cameraFrame->yuv_byte_order,
                                      cameraFrame->color_coding,
                                      0 // supposedly useful for 16-bit formats only, so I don't care
                                      );
    unpeekFrame();

    if(err != DC1394_SUCCESS)
    {
        fprintf(stderr, "Error converting colorspaces\n");
        return false;
    }

    return true;
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
