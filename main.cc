#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_RGB_Image.H>

#include "camera.hh"
#include "threadUtils.h"

#define CAMERA_W        1024
#define CAMERA_H        768
#define CAMERA_PERIOD_S 1

#warning do I need this?
void* cameraThread(void *pArg)
{
    Camera* cam = (Camera*)pArg;

    while(1)
    {
        sleep(CAMERA_PERIOD_S);

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

//             static int i = 0;
//             ostringstream filename;
//             filename << "dat" << i << ".pgm";
//             ofstream dat(filename.str().c_str());
//             dat << "P4\n" << CAMERA_W << ' ' << CAMERA_H << "\n255\n";
//             dat.write((char*)frame, CAMERA_W*CAMERA_H);
//             i++;

        }
    }
    return NULL;
}


int main(void)
{
    // open the first camera we find
    Camera cam(0);

    if(!cam)
        return 0;

    pthread_t cameraThread_id;
    if(pthread_create(&cameraThread_id, NULL, &cameraThread, NULL) != 0)
    {
        cerr << "couldn't start thread" << endl;
        return 0;
    }


    Fl_Double_Window* w = new Fl_Double_Window(CAMERA_W,CAMERA_H);
    Fl_Box box(0,0,CAMERA_W,CAMERA_H);

    Fl_RGB_Image RGBimage(cam.getFrame(), CAMERA_W, CAMERA_H);

    box.image(RGBimage);

    w->resizable(w);
    w->end();
    w->show();
    return Fl::run();
}
