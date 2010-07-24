#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <iostream>
#include <sstream>
#include "cameraSource.hh"
using namespace std;

// These describe the whole camera bus, not just a single camera. Thus we keep only one copy by
// declaring them static members
dc1394_t*            CameraSource::dc1394Context    = NULL;
dc1394camera_list_t* CameraSource::cameraList       = NULL;
unsigned int         CameraSource::numInitedCameras = 0;

// hardware camera settings:
// The resolutions that I know about. These are listed in order from least to most desireable
enum resolution_t { MODE_UNWANTED,
                    MODE_160x120,
                    MODE_320x240,
                    MODE_640x480,
                    MODE_800x600,
                    MODE_1024x768,
                    MODE_1280x960,
                    MODE_1600x1200};
// returns desireability of the resolution. Higher is more desireable
static resolution_t getResolutionWorth(dc1394video_mode_t mode)
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

// hardware camera settings:
// The colormodes that I know about. These are listed in order from least to most desireable
enum colormode_t { COLORMODE_UNWANTED,
                   COLORMODE_MONO16, // FrameSource always uses 8bits per channel, so mono16 does
                                     // nothing for me. Thus it's least desireable
                   COLORMODE_MONO8,
                   COLORMODE_YUV411,
                   COLORMODE_YUV422,
                   COLORMODE_YUV444,
                   COLORMODE_RGB8,

                   // if we wanted grayscale output, then it is more desireable still
                   COLORMODE_MONO8_REQUESTED
};
// returns desireability of the color mode. Higher is more desireable
static colormode_t getColormodeWorth(dc1394video_mode_t mode, bool wantColor)
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
        return wantColor ? COLORMODE_MONO8 : COLORMODE_MONO8_REQUESTED;

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

CameraSource::CameraSource(FrameSource_UserColorChoice _userColorMode,
                           bool resetbus, uint64_t guid,
                           CvRect _cropRect,
                           double scale)
    : FrameSource(_userColorMode), inited(false), camera(NULL), cameraFrame(NULL)
{
    if(!uninitedCamerasLeft())
    {
        cerr << "no more cameras left to init" << endl;
        return;
    }

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

    if(guid == 0)
    {
        // We're not looking for any particular camera, so open them one at a time until we find
        // one that works
        for(unsigned int i=0; i<cameraList->num; i++)
        {
            camera = dc1394_camera_new(dc1394Context, cameraList->ids[i].guid);
            if (camera)
                break;
        }
    }
    else
        camera = dc1394_camera_new(dc1394Context, guid);

    if (!camera)
    {
        dc1394_log_error("Failed to initialize a camera");
        return;
    }

    // Release the resources that could have been allocated by previous instances of a program using
    // the cameras. I don't know which channels and how much bandwidth was allocated, so release
    // EVERYTHING. This can generate bogus error messages, but these should be ignored
    static bool doneOnce = false;
    if(resetbus && !doneOnce)
    {
        doneOnce = true;
        cerr << "cleaning up ieee1394 stack. This may cause error messages..." << endl;
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
        colormode_t colormode = getColormodeWorth(video_modes.modes[i], userColorMode==FRAMESOURCE_COLOR);
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
        cerr << "No known resolutions/colormodes supported. Maybe this is a format7-only camera?" << endl;
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

    inited = true;
    numInitedCameras++;

    setupCroppingScaling(_cropRect, scale);

    isRunningNow.setTrue();

    cerr << "init done" << endl;
}

CameraSource::~CameraSource(void)
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

// _getNextFrame() blocks until a frame is available. true is returned on success.
bool CameraSource::_getNextFrame(IplImage* image, uint64_t* timestamp_us)
{
    beginPeek();

    dc1394error_t err;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &cameraFrame);

    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return false;
    }

    finishPeek(timestamp_us);
    return finishGet(image);
}

// _getLatestFrame() checks the frame buffer. If there are no frames in it, it blocks until a
// frame is available. If there are frames, the buffer is purged and the most recent frame is
// returned. true is returned on success.
bool CameraSource::_getLatestFrame(IplImage* image, uint64_t* timestamp_us)
{
    beginPeek();

    // first, poll the buffer. If no frames are available, use the plain peekNextFrame() call to
    // wait for one
    dc1394error_t err;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &cameraFrame);
    if( err != DC1394_SUCCESS )
    {
        dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                           dc1394_error_get_string(err),
                           __FUNCTION__, __FILE__, __LINE__);
        return false;
    }
    if(cameraFrame == NULL)
        return _getNextFrame(image, timestamp_us);


    // A frame was available. When the buffer fills up, newest incoming frames are thrown
    // away. Thus, I purge the buffer and get a fresh new frame
    if(!purgeBuffer())
        return NULL;

    // flushed the queue now, so grab the next frame
    return _getNextFrame(image, timestamp_us);
}

bool CameraSource::purgeBuffer(void)
{
    dc1394error_t err;
    do
    {
        // If I have a dequeued frame, give it back to the OS.
        if(cameraFrame != NULL)
        {
            err = dc1394_capture_enqueue(camera, cameraFrame);
            if( err != DC1394_SUCCESS )
            {
                dc1394_log_warning("%s: in %s (%s, line %d): Could not enqueue\n",
                                   dc1394_error_get_string(err),
                                   __FUNCTION__, __FILE__, __LINE__);
                return false;
            }
        }

        err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &cameraFrame);
        if( err != DC1394_SUCCESS )
        {
            dc1394_log_warning("%s: in %s (%s, line %d): Could not capture a frame\n",
                               dc1394_error_get_string(err),
                               __FUNCTION__, __FILE__, __LINE__);
            return false;
        }
    } while(cameraFrame != NULL);

    return true;
}

void CameraSource::beginPeek(void)
{
    if(cameraFrame != NULL)
    {
        cerr << "warning: peekNextFrame() before unpeekFrame()\n"
            "Calling unpeekFrame() for you, but you should do this yourself\n"
            "as soon as you're done with the data" << endl;
        unpeekFrame();
    }
}

unsigned char* CameraSource::finishPeek(uint64_t* timestamp_us)
{
    if(timestamp_us != NULL)
        *timestamp_us = cameraFrame->timestamp;

    return cameraFrame->image;
}

bool CameraSource::finishGet(IplImage* image)
{
    // These convert the data to my desired colorspace from the raw format of the camera. These
    // functions have a few drawbacks:
    //
    // 1. They will convert grayscale data into color data, if asked, but will NOT throw away data
    //    by converting color data into grayscale. This means that with this setup a color camera
    //    will not work with userColorMode == FRAMESOURCE_GRAYSCALE, unless it supports a grayscale
    //    mode in hardware. I tried to get around this by using libswscale to perform the conversion
    //    but it didn't support all the color modes I needed for IIDC cameras.
    // 2. These function assume a default stride in both the input and output data. This is likely
    //    OK for the input, since the cameras will not output anything weird, but the image we're
    //    writing into may have unusual padding. For now, I assert that this is not the case.
    //
    // To address these shortcomings I wanted to use the ffmpeg scaler (sws_scale) to perform these
    // conversions instead. That works great EXCEPT, a packed YUV411 mode is not currently supported
    // there, and this is a mode I explicitly need right now. I just removed the sws_scale
    // implementation of these conversions, so it can be accessed from the version control. I will
    // add that mode to sws_scale if the above shortcomings prove overly-problematic

    IplImage* buffer;
    if(preCropScaleBuffer == NULL) buffer = image;
    else                           buffer = preCropScaleBuffer;

    dc1394error_t err;
    if(userColorMode == FRAMESOURCE_COLOR)
    {
        // these assertions are explained in the long comment above
        assert(buffer->widthStep == buffer->width * 3);

        err = dc1394_convert_to_RGB8(cameraFrame->image,
                                     (unsigned char*)buffer->imageData,
                                     cameraFrame->size[0], cameraFrame->size[1],
                                     cameraFrame->yuv_byte_order,
                                     cameraFrame->color_coding,
                                     0 // supposedly useful for 16-bit formats only, so I don't care
                                     );
    }
    else
    {
        // these assertions are explained in the long comment above
        assert(buffer->widthStep == buffer->width);
        assert(cameraFrame->color_coding == DC1394_COLOR_CODING_MONO8 ||
               cameraFrame->color_coding == DC1394_COLOR_CODING_MONO16);

        err = dc1394_convert_to_MONO8(cameraFrame->image,
                                      (unsigned char*)buffer->imageData,
                                      cameraFrame->size[0], cameraFrame->size[1],
                                      cameraFrame->yuv_byte_order,
                                      cameraFrame->color_coding,
                                      0 // supposedly useful for 16-bit formats only, so I don't care
                                      );
    }

    if(preCropScaleBuffer != NULL)
    {
#warning this isnt very efficient. The cropping and scaling should be a part of the conversion functions above
        applyCroppingScaling(preCropScaleBuffer, image);
    }

    unpeekFrame();

    return true;
}

void CameraSource::unpeekFrame(void)
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
