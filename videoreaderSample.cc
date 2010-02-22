#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "flWidgetImage.hh"

#include "ffmpegInterface.hh"
#include "pthread.h"

#define SOURCE_PERIOD_NS 1000000000

static FlWidgetImage* widgetImage;

static bool sourceThread_doTerminate = false;
void* sourceThread(void *pArg)
{
    FrameSource* source = (FrameSource*)pArg;

    while(!sourceThread_doTerminate)
    {
        struct timespec delay;
        delay.tv_sec = 0;
        delay.tv_nsec = SOURCE_PERIOD_NS;
        nanosleep(&delay, NULL);

        uint64_t timestamp_us;
        unsigned char* frame = source->peekLatestFrame(&timestamp_us);

        if(frame == NULL)
        {
            cerr << "couldn't get frame\n";
            source->unpeekFrame();
            return NULL;
        }

        Fl::lock();
        if(sourceThread_doTerminate) return NULL;

        widgetImage->updateFrame( frame );
        Fl::unlock();
        source->unpeekFrame();
    }
    return NULL;
}


int main(int argc, char* argv[])
{
    if(argc == 0)
    {
        fprintf(stderr, "need video file on the cmdline\n");
        return 0;
    }

    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first source. request color
    FrameSource* source = new FFmpegDecoder(argv[1]);
    if(! *source)
    {
        fprintf(stderr, "couldn't open source\n");
        delete source;
        return 0;
    }

    Fl_Window window(source->w(), source->h());
    widgetImage = new FlWidgetImage(0, 0, source->w(), source->h(),
                                  WIDGET_COLOR, FAST_DRAW);

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
