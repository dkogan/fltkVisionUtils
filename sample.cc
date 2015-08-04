#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "cvFltkWidget.hh"
#include "ffmpegInterface.hh"

#include "cameraSource_IIDC.hh"
#include "cameraSource_v4l2.hh"
#include "IIDC_featuresWidget.hh"

#include <opencv2/imgproc/imgproc_c.h>




static const bool do_encode_video  = false;
static const int  source_period_us = -1;; // <0 means use synchronous poll-based I/O





static CvFltkWidget* widgetImage;
static FFmpegEncoder videoEncoder;
static CvMat         edges;
static FrameSource*  source;

static void newFrameArrived(bool dolock)
{
    if(do_encode_video)
    {
        if( !videoEncoder ||
            !videoEncoder.writeFrame(*widgetImage) ||
            !videoEncoder )
        {
            cerr << "Couldn't encode frame!" << endl;
            return;
        }
    }


    if(dolock)
        Fl::lock();

    cvSetImageCOI(*widgetImage, 1);
    cvCopy(*widgetImage, &edges);
    cvCanny(&edges, &edges, 20, 50);
    cvCopy(&edges, *widgetImage);
    cvSetImageCOI(*widgetImage, 0);
    widgetImage->redrawNewFrame();

    if(dolock)
        Fl::unlock();
}

// asynchronous timer-based callback. This is the next frame available after a
// preset delay
static bool gotNewFrame(IplImage* buffer      __attribute__((unused)),
                        uint64_t timestamp_us __attribute__((unused)))
{
    // the buffer passed in here is the same buffer that I specified when
    // starting the source thread. In this case this is the widget's buffer
    newFrameArrived(true);
    return true;
}

// synchronous callback; happens when there's a frame available to read
static void syncronous_calback(FL_SOCKET fd __attribute__((unused)),
                               void *data   __attribute__((unused)))
{
    source->getNextFrame(*widgetImage);
    newFrameArrived(false); // synchronous callback, so no unlocking needed
}

int main(int argc, char* argv[])
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. If there's an argument, assume it's an input video. Otherwise, try
    // reading a camera
    if(argc >= 2)
        source = new FFmpegDecoder(argv[1], FRAMESOURCE_COLOR, true,
                                   cvRect(0, 0, 320, 480), 1.5);
    else
    {
        struct v4l2_settings settings[] =
            {
                // manual exposure
                {V4L2_CID_EXPOSURE_AUTO_PRIORITY, 1},
                {V4L2_CID_EXPOSURE_AUTO,          1},   // 0-3
                {V4L2_CID_EXPOSURE_ABSOLUTE,      300}, // 3-2047
                {-1, -1} };

        source = new CameraSource_V4L2(FRAMESOURCE_COLOR, "/dev/video0",
                                       640, 480, settings);
    }

    if(! *source)
    {
        cerr << "couldn't open source" << endl;
        delete source;
        return 0;
    }

    cvInitMatHeader(&edges, source->h(), source->w(), CV_8UC1);
    cvCreateData(&edges);

    if(do_encode_video)
    {
        videoEncoder.open("capture.avi", source->w(), source->h(), 15, FRAMESOURCE_COLOR);
        if(!videoEncoder)
        {
            cerr << "Couldn't initialize the video encoder" << endl;
            return 0;
        }
    }

    Fl_Window window(source->w(), source->h() + 400);
    widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                   WIDGET_COLOR);

    if(dynamic_cast<CameraSource_IIDC*>(source) != NULL)
    {
        IIDC_featuresWidget* features __attribute__((unused)) =
            new IIDC_featuresWidget(*(CameraSource_IIDC*)source,
                                    0, source->h(),
                                    800, 100, NULL, true);
    }




    window.resizable(window);
    window.end();
    window.show();

    if( source_period_us < 0 )
    {
        int camera_fd = source->getFD();
        if( camera_fd >= 0 )
            Fl::add_fd(camera_fd, FL_READ, &syncronous_calback);
        else
        {
            fprintf(stderr, "poll-based i/o requested, but this frame source does not support it\n");
            return 1;
        }
    }
    else
        // I'm starting a new camera-reading thread and storing the frame directly into the widget
        // buffer
        source->startSourceThread(&gotNewFrame, source_period_us, *widgetImage);

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;

    if(do_encode_video)
        videoEncoder.close();

    cvReleaseData(&edges);

    return 0;
}
