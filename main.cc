#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include "camera.hh"

int main(void)
{
    // open the first camera we find
    Camera cam(0);

    if(!cam)
        return 0;

    for(int i=0; i<10; i++)
    {
        sleep(1);
        struct timespec timestamp;
        unsigned char* frame = cam.getFrame(&timestamp);

        if(frame == NULL)
        {
            cerr << "couldn't get frame\n";
        }
        else
        {
            cerr << "got frame at " << timestamp.tv_sec << endl;
            static int i = 0;
            ostringstream filename; 
            filename << "dat" << i << ".pgm";
            ofstream dat(filename.str().c_str());
            dat << "P4\n1024 768\n255\n";
            dat.write((char*)frame, 1024*768);
            i++;
        }
    }

}
