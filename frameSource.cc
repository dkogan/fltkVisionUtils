#include "frameSource.hh"
#include <string.h>

// All users of FrameSource interact with it in terms of 8-bits-per-pixel RGB images. Source data is
// converted to this format before being returned
unsigned char* FrameSource::copyFrame(uint64_t* timestamp_us, unsigned char* frame)
{
    unsigned char* buf = peekFrame(timestamp_us);
    if(!buf)
        return NULL;

    memcpy(frame, buf, width*height * 3); // I'm assuming 24-bit RGB

    return frame;
}
