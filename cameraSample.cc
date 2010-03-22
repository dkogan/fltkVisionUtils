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

#include "cameraSource.hh"

#define SOURCE_PERIOD_US 1000000

static CvFltkWidget* widgetImage;
static FFmpegEncoder videoEncoder;

void gotNewFrame(IplImage* buffer __attribute__((unused)), uint64_t timestamp_us __attribute__((unused)))
{
    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer
    if(!videoEncoder)
    {
        fprintf(stderr, "Couldn't encode frame!\n");
        return;
    }
    videoEncoder.writeFrameGrayscale(*widgetImage);
    if(!videoEncoder)
    {
        fprintf(stderr, "Couldn't encode frame!\n");
        return;
    }

    Fl::lock();
    widgetImage->redrawNewFrame();
    Fl::unlock();
}


int main(void)
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. request color
    CameraSource* source = new CameraSource(FRAMESOURCE_COLOR);
    if(! *source)
    {
        fprintf(stderr, "couldn't open source\n");
        delete source;
        return 0;
    }
    cout << source->getDescription();

    videoEncoder.open("capture.avi", source->w(), source->h(), 15, FRAMESOURCE_COLOR);
    if(!videoEncoder)
    {
        fprintf(stderr, "Couldn't initialize the video encoder\n");
        return 0;
    }

    Fl_Window window(source->w(), source->h());
    widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                   WIDGET_COLOR);

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
    delete widgetImage;

    videoEncoder.close();

    return 0;
}
