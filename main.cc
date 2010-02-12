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
            dat << "P4\n1024 768\n255\n";
            ofstream dat(filename.str().c_str());
            dat.write((char*)frame, 1024*768);
        }
    }

}


extern "C" void raw1394_set_iso_handler()
{
    cerr << "raw1394_set_iso_handler" << endl;
}

extern "C" void raw1394_start_iso_rcv()
{
    cerr << "raw1394_start_iso_rcv" << endl;
}

extern "C" void raw1394_stop_iso_rcv()
{
    cerr << "raw1394_stop_iso_handler" << endl;
}
