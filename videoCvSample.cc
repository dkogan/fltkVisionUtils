#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "flWidgetCv.hh"

#include "ffmpegInterface.hh"
#include "pthread.h"

#define SOURCE_PERIOD_S  0
#define SOURCE_PERIOD_NS 100000000

static FlWidgetCv* widgetImage;

static bool sourceThread_doTerminate = false;
void* sourceThread(void *pArg)
{
    FrameSource* source = (FrameSource*)pArg;

    while(!sourceThread_doTerminate)
    {
        struct timespec delay;
        delay.tv_sec =  SOURCE_PERIOD_S;
        delay.tv_nsec = SOURCE_PERIOD_NS;
        nanosleep(&delay, NULL);

        uint64_t timestamp_us;

        if( !source->getNextFrame(&timestamp_us, widgetImage->getBuffer()) )
        {
            cerr << "couldn't get frame\n";
            return NULL;
        }

        cvCanny(*widgetImage, *widgetImage, 20, 50);

        Fl::lock();
        if(sourceThread_doTerminate) return NULL;

        widgetImage->redrawNewFrame();
        Fl::unlock();
    }
    return NULL;
}


int main(int argc, char* argv[])
{
    if(argc <= 1)
    {
        fprintf(stderr, "need video file on the cmdline\n");
        return 0;
    }

    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. request color
    FrameSource* source = new FFmpegDecoder(argv[1], FRAMESOURCE_GRAYSCALE);
    if(! *source)
    {
        fprintf(stderr, "couldn't open source\n");
        delete source;
        return 0;
    }

    Fl_Window window(source->w(), source->h());
    widgetImage = new FlWidgetCv(0, 0, source->w(), source->h(),
                                 WIDGET_GRAYSCALE);

    window.resizable(window);
    window.end();
    window.show();


    pthread_t sourceThread_id;
    if(pthread_create(&sourceThread_id, NULL, &sourceThread, source) != 0)
    {
        cerr << "couldn't start source thread" << endl;
        return 0;
    }

    while (Fl::wait())
    {
    }

    Fl::unlock();
    sourceThread_doTerminate = true;

    pthread_join(sourceThread_id, NULL);

    delete source;
    delete widgetImage;

    return 0;
}
