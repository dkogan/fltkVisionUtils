#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <list>
#include <string.h>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Value_Slider.H>

#include "FLTK_camera.hh"

#include "camera.hh"
#include "pthread.h"

#define CAMERA_W         1024
#define CAMERA_H         768
#define CAMERA_PERIOD_NS 100000000

#warning do I need this?

static CameraWidget*    camWidget;
static Fl_Value_Slider* cameraSelector;

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
        unsigned char* frame = cam->getFrame(&timestamp_us);
        camWidget->updateFrame( frame );

        if(frame == NULL)
        {
            cerr << "couldn't get frame\n";
            cam->releaseFrame();
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
        cam->releaseFrame();
    }
    return NULL;
}


int main(void)
{
    Fl::lock();
    Fl::visual(FL_RGB);

    list<Camera*> cameras;

    // keep opening the cameras as long as we can
    while(1)
    {
        Camera* cam = new Camera( cameras.size() );
        if(*cam)
            cameras.push_back(cam);
        else
        {
            delete cam;
            break;
        }
    }

    if( cameras.size() == 0)
    {
        fprintf(stderr, "no cameras found\n");
        return 0;
    }

    Fl_Window window(CAMERA_W,CAMERA_H+30);
    camWidget = new CameraWidget(0,0,CAMERA_W,CAMERA_H,
                                 CAMERA_W,CAMERA_H);

    cameraSelector = new Fl_Value_Slider(0, CAMERA_H+10, 300, 20, "Displayed camera");
    cameraSelector->type(FL_HOR_NICE_SLIDER);
    cameraSelector->range(0, cameras.size()-1);
    cameraSelector->precision(0); // integer

    window.resizable(window);
    window.end();
    window.show();


    list<pthread_t> cameraThread_ids;
    for(list<Camera*>::iterator itr = cameras.begin();
        itr != cameras.end();
        itr++)
    {
        pthread_t cameraThread_id;
        if(pthread_create(&cameraThread_id, NULL, &cameraThread, *itr) != 0)
        {
            cerr << "couldn't start camera thread" << endl;
            return 0;
        }
        cameraThread_ids.push_back(cameraThread_id);
    }

    Fl::run();

    Fl::unlock();
    cameraThread_doTerminate = true;

    for(list<pthread_t>::iterator itr = cameraThread_ids.begin();
        itr != cameraThread_ids.end();
        itr++)
    {
        pthread_join(*itr, NULL);
    }

    for(list<Camera*>::iterator itr = cameras.begin();
        itr != cameras.end();
        itr++)
    {
        delete *itr;
    }

    delete camWidget;
    delete cameraSelector;

    return 0;
}
