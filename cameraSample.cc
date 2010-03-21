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

#include "camera.hh"

#define SOURCE_PERIOD_US 1000000

static CvFltkWidget* widgetImage;
static FFmpegEncoder videoEncoder;

void gotNewFrame(unsigned char* buffer __attribute__((unused)), uint64_t timestamp_us __attribute__((unused)))
{
    // the buffer passed in here is the same buffer that I specified when starting the source
    // thread. In this case this is the widget's buffer
    if(!videoEncoder)
    {
        fprintf(stderr, "Couldn't encode frame!\n");
        return;
    }
    videoEncoder.writeFrameGrayscale( (unsigned char*)((IplImage*)(*widgetImage))->imageData);
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
    Camera* source = new Camera(FRAMESOURCE_GRAYSCALE);
    if(! *source)
    {
        fprintf(stderr, "couldn't open source\n");
        delete source;
        return 0;
    }
    cout << source->getDescription();

    videoEncoder.open("capture.avi", source->w(), source->h(), 15);
    if(!videoEncoder)
    {
        fprintf(stderr, "Couldn't initialize the video encoder\n");
        return 0;
    }

    Fl_Window window(source->w(), source->h());
    widgetImage = new CvFltkWidget(0, 0, source->w(), source->h(),
                                   WIDGET_GRAYSCALE);

    window.resizable(window);
    window.end();
    window.show();

    // I'm starting a new camera-reading thread and storing the frame directly into the widget
    // buffer
    source->startSourceThread(&gotNewFrame, SOURCE_PERIOD_US, (unsigned char*)((IplImage*)(*widgetImage))->imageData);

    while (Fl::wait())
    {
    }

    Fl::unlock();

    delete source;
    delete widgetImage;

    videoEncoder.close();

    return 0;
}
