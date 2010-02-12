#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include "camera.hh"
#include "pthread.h"

#define CAMERA_W         1024
#define CAMERA_H         768

#warning do I need this?

static bool cameraThread_doTerminate = false;
void* cameraThread(void *pArg)
{
    Camera* cam = (Camera*)pArg;

    while(!cameraThread_doTerminate)
    {
        struct timespec timestamp;
        unsigned char* frame = cam->getFrame(&timestamp);

        if(frame == NULL)
        {
            cerr << "couldn't get frame\n";
            return NULL;
        }
        else
        {
            // DO SOMETHING WITH THE FRAME HERE

            Fl::lock();
            if(cameraThread_doTerminate) return NULL;
            fl_draw_image_mono(frame, 0, 0, CAMERA_W, CAMERA_H);
            Fl::unlock();
        }
    }
    return NULL;
}


int main(void)
{
    Fl::lock();
    Fl::visual(FL_RGB);

    // open the first camera we find
    Camera cam(0);

    if(!cam)
        return 0;

    Fl_Window* w = new Fl_Window(CAMERA_W,CAMERA_H);

    pthread_t cameraThread_id;
    if(pthread_create(&cameraThread_id, NULL, &cameraThread, &cam) != 0)
    {
        cerr << "couldn't start thread" << endl;
        return 0;
    }

    w->resizable(w);
    w->end();
    w->show();
    Fl::run();

    Fl::unlock();
    cameraThread_doTerminate = true;
    pthread_join(cameraThread_id, NULL);

    return 0;
}
