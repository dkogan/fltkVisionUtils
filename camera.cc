extern "C"
{
#include "camera.hh"
}

#include <iostream>
using namespace std;

#define BYTES_PER_PIXEL 2
static Camwire_state settings(void)
{
    Camwire_state set;
    set.num_frame_buffers = 2;
    set.blue_gain         = 1.0;
    set.red_gain          = 1.0;
    set.left              = 0;
    set.top               = 0;
    set.width             = 1024;
    set.height            = 768;
    set.coding            = CAMWIRE_PIXEL_MONO8;
    set.frame_rate        = 32;
    set.shutter           = 0.022240; // to get shutter=1112 in coriander. scale factor is 50000 for some reason
    set.external_trigger  = 0;
    set.single_shot       = 0;
    set.running           = 1;
    set.shadow            = 0;
    return set;
}

Camera::Camera(int cameraIndex)
{
    cameraHandle = NULL;
    frame        = NULL;
    inited       = false;

    int num_handles;
    Camwire_handle *handle_array = NULL;

//     camwire_bus_reset();

    sleep(1);

    handle_array = camwire_bus_create(&num_handles);
    if(num_handles <= 0 || handle_array == NULL)
    {
        cerr << "camwire_bus_create failed" << endl;
        return;
    }
    if(num_handles <= 0)
    {
        cerr << "camwire_bus_create got fewer than 1 handle...." << endl;
        return;
    }

    if(cameraIndex >= num_handles)
    {
        cerr << "tried to use camera handle " << cameraIndex << " while there were only " <<
            num_handles << " handles available" << endl;
        return;
    }

    cameraHandle = handle_array[cameraIndex];

    Camwire_state set = settings();
    if(camwire_create_from_struct(cameraHandle, &set) != CAMWIRE_SUCCESS)
        return;

    frame = new unsigned char[set.width*set.height * BYTES_PER_PIXEL];
    inited = true;
    cerr << "init done" << endl;
}

Camera::~Camera(void)
{
    if (cameraHandle != NULL)
    {
        // stop the camera, and sleep 2 frames worth of time
        camwire_set_run_stop(cameraHandle, 0);
        double framerate;
        camwire_get_framerate(cameraHandle, &framerate);
        double period = 2.0 / framerate;
        struct timespec sleeptime;
        sleeptime.tv_sec  = period;
        sleeptime.tv_nsec = (period - sleeptime.tv_sec) * 1.0e9;
        nanosleep(&sleeptime, NULL);
        camwire_destroy(cameraHandle);
        camwire_bus_destroy();

        cameraHandle = NULL;
    }

    if(frame != NULL)
        delete[] frame;
}

unsigned char* Camera::getFrame(struct timespec* timestamp)
{
    if(camwire_copy_next_frame(cameraHandle, (void*)frame, NULL) != CAMWIRE_SUCCESS)
        return NULL;

    if(timestamp != NULL)
        camwire_get_timestamp(cameraHandle, timestamp);

    return frame;
}
