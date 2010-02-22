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

#include "camera.hh"
#include "pthread.h"

#define CAMERA_PERIOD_NS 1000000000

static FlWidgetImage* camWidget;

static bool cameraThread_doTerminate = false;
void* cameraThread(void *pArg)
{
    Camera* cam = (Camera*)pArg;

    while(!cameraThread_doTerminate)
    {
        struct timespec delay;
        delay.tv_sec = 0;
        delay.tv_nsec = CAMERA_PERIOD_NS;
        nanosleep(&delay, NULL);

        uint64_t timestamp_us;
        unsigned char* frame = cam->peekLatestFrame(&timestamp_us);

        if(frame == NULL)
        {
            cerr << "couldn't get frame\n";
            cam->unpeekFrame();
            return NULL;
        }

        Fl::lock();
        if(cameraThread_doTerminate) return NULL;

        camWidget->updateFrame( frame );
        Fl::unlock();
        cam->unpeekFrame();
    }
    return NULL;
}


int main(void)
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first camera. request color
    Camera* cam = new Camera(FRAMESOURCE_COLOR);
    if(! *cam)
    {
        fprintf(stderr, "couldn't open camera\n");
        delete cam;
        return 0;
    }

    Fl_Window window(cam->w(), cam->h());
    camWidget = new FlWidgetImage(0, 0, cam->w(), cam->h(),
                                  WIDGET_COLOR, FAST_DRAW);

    window.resizable(window);
    window.end();
    window.show();


    pthread_t cameraThread_id;
    if(pthread_create(&cameraThread_id, NULL, &cameraThread, cam) != 0)
    {
        cerr << "couldn't start camera thread" << endl;
        return 0;
    }

    while (Fl::wait())
    {
    }

    Fl::unlock();
    cameraThread_doTerminate = true;

    pthread_join(cameraThread_id, NULL);

    delete cam;
    delete camWidget;

    return 0;
}
