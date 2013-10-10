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
#include "IIDC_featuresWidget.hh"

#include <opencv2/imgproc/imgproc_c.h>

#define SOURCE_PERIOD_US 1000000

static CvFltkWidget* widgetImage;
static FFmpegEncoder videoEncoder;
static CvMat         edges;

bool gotNewFrame(IplImage* buffer __attribute__((unused)), uint64_t timestamp_us __attribute__((unused)))
{
    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer
    if(!videoEncoder)
    {
        cerr << "Couldn't encode frame!" << endl;
        return false;
    }
    videoEncoder.writeFrame(*widgetImage);
    if(!videoEncoder)
    {
        cerr << "Couldn't encode frame!" << endl;
        return false;
    }

    Fl::lock();
    cvSetImageCOI(*widgetImage, 1);
    cvCopy(*widgetImage, &edges);
    cvCanny(&edges, &edges, 20, 50);
    cvCopy(&edges, *widgetImage);
    cvSetImageCOI(*widgetImage, 0);
    widgetImage->redrawNewFrame();
    Fl::unlock();

    return true;
}

int main(int argc, char* argv[])
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. If there's an argument, assume it's an input video. Otherwise, try
    // reading a camera
    FrameSource* source;
    if(argc >= 2)
        source = new FFmpegDecoder(argv[1], FRAMESOURCE_COLOR, true,
                                   cvRect(0, 0, 320, 480), 1.5);
    else
    {
        source = new CameraSource_IIDC(FRAMESOURCE_COLOR);
        cout << ((CameraSource_IIDC*)source)->getDescription();
    }

    if(! *source)
    {
        cerr << "couldn't open source" << endl;
        delete source;
        return 0;
    }

    cvInitMatHeader(&edges, source->h(), source->w(), CV_8UC1);
    cvCreateData(&edges);

    videoEncoder.open("capture.avi", source->w(), source->h(), 15, FRAMESOURCE_COLOR);
    if(!videoEncoder)
    {
        cerr << "Couldn't initialize the video encoder" << endl;
        return 0;
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

    // I'm starting a new camera-reading thread and storing the frame directly into the widget
    // buffer
    source->startSourceThread(&gotNewFrame, SOURCE_PERIOD_US, *widgetImage);

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;

    videoEncoder.close();

    cvReleaseData(&edges);

    return 0;
}
