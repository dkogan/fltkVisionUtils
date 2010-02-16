#include "frameSource.hh"
#include <string.h>

unsigned char* FrameSource::copyFrame(uint64_t* timestamp_us, unsigned char* frame)
{
    unsigned char* buf = peekFrame(timestamp_us);
    if(!buf)
        return NULL;

    memcpy(frame, buf, width*height*bitsPerPixel / 8);
    return frame;
}
