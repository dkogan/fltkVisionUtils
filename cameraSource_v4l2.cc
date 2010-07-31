#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>

#include "cameraSource_v4l2.hh"

static int ioctl_persistent( int fd, int request, void* arg)
{
    int r;

    do
    {
        r = ioctl( fd, request, arg);
    } while( r == -1 && errno == EINTR);

    return r;
}

static int getPixfmtCost(uint32_t pixfmt)
{
    // pixel formats in order of decreasing desireability. Sorta arbitrary. I favor colors, higher
    // bitrates and lower compression
    static uint32_t pixfmts[] =
        {
            /* RGB formats */
            V4L2_PIX_FMT_BGR32, /* 32  BGR-8-8-8-8   */
            V4L2_PIX_FMT_RGB32, /* 32  RGB-8-8-8-8   */
            V4L2_PIX_FMT_BGR24, /* 24  BGR-8-8-8     */
            V4L2_PIX_FMT_RGB24, /* 24  RGB-8-8-8     */
            V4L2_PIX_FMT_RGB444, /* 16  xxxxrrrr ggggbbbb */
            V4L2_PIX_FMT_RGB555, /* 16  RGB-5-5-5     */
            V4L2_PIX_FMT_RGB565, /* 16  RGB-5-6-5     */
            V4L2_PIX_FMT_RGB555X, /* 16  RGB-5-5-5 BE  */
            V4L2_PIX_FMT_RGB565X, /* 16  RGB-5-6-5 BE  */
            V4L2_PIX_FMT_RGB332, /*  8  RGB-3-3-2     */

            /* Palette formats */
            V4L2_PIX_FMT_PAL8, /*  8  8-bit palette */

            /* Luminance+Chrominance formats */
            V4L2_PIX_FMT_YUV32, /* 32  YUV-8-8-8-8   */
            V4L2_PIX_FMT_YUV444, /* 16  xxxxyyyy uuuuvvvv */
            V4L2_PIX_FMT_YUV555, /* 16  YUV-5-5-5     */
            V4L2_PIX_FMT_YUV565, /* 16  YUV-5-6-5     */
            V4L2_PIX_FMT_YUYV, /* 16  YUV 4:2:2     */
            V4L2_PIX_FMT_YYUV, /* 16  YUV 4:2:2     */
            V4L2_PIX_FMT_YVYU, /* 16 YVU 4:2:2 */
            V4L2_PIX_FMT_UYVY, /* 16  YUV 4:2:2     */
            V4L2_PIX_FMT_VYUY, /* 16  YUV 4:2:2     */
            V4L2_PIX_FMT_YUV422P, /* 16  YVU422 planar */
            V4L2_PIX_FMT_YUV411P, /* 16  YVU411 planar */
            V4L2_PIX_FMT_YVU420, /* 12  YVU 4:2:0     */
            V4L2_PIX_FMT_Y41P, /* 12  YUV 4:1:1     */
            V4L2_PIX_FMT_YUV420, /* 12  YUV 4:2:0     */
            V4L2_PIX_FMT_YVU410, /*  9  YVU 4:1:0     */
            V4L2_PIX_FMT_YUV410, /*  9  YUV 4:1:0     */
            V4L2_PIX_FMT_HI240, /*  8  8-bit color   */
            V4L2_PIX_FMT_HM12, /*  8  YUV 4:2:0 16x16 macroblocks */

            /* two planes -- one Y, one Cr + Cb interleaved  */
            V4L2_PIX_FMT_NV12, /* 12  Y/CbCr 4:2:0  */
            V4L2_PIX_FMT_NV21, /* 12  Y/CrCb 4:2:0  */
            V4L2_PIX_FMT_NV16, /* 16  Y/CbCr 4:2:2  */
            V4L2_PIX_FMT_NV61, /* 16  Y/CrCb 4:2:2  */

            /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
            V4L2_PIX_FMT_SGRBG10, /* 10bit raw bayer */
            /*
             * 10bit raw bayer, expanded to 16 bits
             * xxxxrrrrrrrrrrxxxxgggggggggg xxxxggggggggggxxxxbbbbbbbbbb...
             */
            V4L2_PIX_FMT_SBGGR16, /* 16  BGBG.. GRGR.. */
            /* 10bit raw bayer DPCM compressed to 8 bits */
            V4L2_PIX_FMT_SGRBG10DPCM8,
            V4L2_PIX_FMT_SBGGR8, /*  8  BGBG.. GRGR.. */
            V4L2_PIX_FMT_SGBRG8, /*  8  GBGB.. RGRG.. */
            V4L2_PIX_FMT_SGRBG8, /*  8  GRGR.. BGBG.. */

            /* compressed formats */
            V4L2_PIX_FMT_MJPEG, /* Motion-JPEG   */
            V4L2_PIX_FMT_JPEG, /* JFIF JPEG     */
            V4L2_PIX_FMT_DV, /* 1394          */
            V4L2_PIX_FMT_MPEG, /* MPEG-1/2/4    */

            /*  Vendor-specific formats   */
            V4L2_PIX_FMT_WNVA, /* Winnov hw compress */
            V4L2_PIX_FMT_SN9C10X, /* SN9C10x compression */
            V4L2_PIX_FMT_SN9C20X_I420, /* SN9C20x YUV 4:2:0 */
            V4L2_PIX_FMT_PWC1, /* pwc older webcam */
            V4L2_PIX_FMT_PWC2, /* pwc newer webcam */
            V4L2_PIX_FMT_ET61X251, /* ET61X251 compression */
            V4L2_PIX_FMT_SPCA501, /* YUYV per line */
            V4L2_PIX_FMT_SPCA505, /* YYUV per line */
            V4L2_PIX_FMT_SPCA508, /* YUVY per line */
            V4L2_PIX_FMT_SPCA561, /* compressed GBRG bayer */
            V4L2_PIX_FMT_PAC207, /* compressed BGGR bayer */
            V4L2_PIX_FMT_MR97310A, /* compressed BGGR bayer */
            V4L2_PIX_FMT_SQ905C, /* compressed RGGB bayer */
            V4L2_PIX_FMT_PJPG, /* Pixart 73xx JPEG */
            V4L2_PIX_FMT_OV511, /* ov511 JPEG */
            V4L2_PIX_FMT_OV518, /* ov518 JPEG */

            /* Grey formats */
            V4L2_PIX_FMT_Y16, /* 16  Greyscale     */
            V4L2_PIX_FMT_GREY, /*  8  Greyscale     */
        };

    for(int i=0; i<sizeof(pixfmts) / sizeof(pixfmts[0]); i++)
        if(pixfmts[i] == pixfmt)
            return i;

    return -1;
}

CameraSource_V4L2::CameraSource_V4L2(FrameSource_UserColorChoice _userColorMode,
                                     char* device,
                                     CvRect _cropRect,
                                     double scale)
    : camera_fd(-1),
      numBuffers(0),
      buffers(NULL)
{
    struct stat st; 

    camera_fd = open( device, O_RDWR, 0);
    if( camera_fd < 0)
    {
        perror("Couldn't open video device");
        uninit();
        return;
    }

#define ioctl_try(fd, request, arg)             \
do {                                            \
    if(ioctl_persistent(fd,request,arg) < 0)    \
    {                                           \
        perror( "Couldn't "#request );          \
        uninit();                               \
        return;                                 \
    }                                           \
} while(0)

    struct v4l2_capability cap;
    ioctl_try(camera_fd, VIDIOC_QUERYCAP, &cap);

    if( !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf( stderr, "%s is no video capture device\n", device);
        uninit();
        return;
    }

    if( !(cap.capabilities & V4L2_CAP_READWRITE))
    {
        fprintf( stderr, "%s does not support read i/o\n", device);
        uninit();
        return;
    }

    // if the camera can crop its output, reset the cropping window
    struct v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( ioctl_persistent( camera_fd, VIDIOC_CROPCAP, &cropcap) == 0)
    {
        struct v4l2_crop crop;
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
        ioctl_try( camera_fd, VIDIOC_S_CROP, &crop);
    }

    // Now find the best pixel format
    int      bestPixfmtCost = INT_MAX;
    uint32_t bestPixfmt;
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));

    while(true)
    {
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        int res = ioctl_persistent( camera_fd, VIDIOC_ENUM_FMT, &fmtdesc);
        if(res < 0)
        {
            if(errno == EINVAL)
                // no more formats left
                break;

            // some bad error occurred
            perror("Error enumerating V4L2 formats");
            uninit();
            return;
        }

        int cost = getPixfmtCost(fmtdesc.pixelformat);
        if(cost < bestPixfmtCost)
        {
            bestPixfmtCost = cost;
            bestPixfmt = fmtdesc.pixelformat;
        }

        fmtdesc.index++;
    }

    // I get the current image format
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( ioctl_persistent( camera_fd, VIDIOC_G_FMT, &fmt) < 0 )
    {
        perror( "VIDIOC_G_FMT");
        uninit();
        return;
    }

    // I now set the image format to an upper bound of what I want. The driver will change the
    // passed-in parameters to whatever it is actually capable of
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 1000000; // request an unreasonable resolution. The driver
    fmt.fmt.pix.height      = 1000000; // will reduce this to whatever it actually can do
    fmt.fmt.pix.pixelformat = bestPixfmt;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE; // don't want interlacing ideally

    if( ioctl_persistent( camera_fd, VIDIOC_S_FMT, &fmt) < 0 )
    {
        perror( "VIDIOC_S_FMT");
        uninit();
        return;
    }

    /* Buggy driver paranoia. */
    if(fmt.fmt.pix.bytesperline < fmt.fmt.pix.width)
    {
        fprintf(stderr, "bytesperline < width. This can't be right. Driver bug?\n");
        uninit();
        return;
    }
    if( fmt.fmt.pix.sizeimage < fmt.fmt.pix.bytesperline * fmt.fmt.pix.height)
    {
        fprintf(stderr, "sizeimage < bytesperline * height. This can't be right. Driver bug?\n");
        uninit();
        return;
    }

    buffers = calloc( 1, sizeof (*buffers));
    if( buffers == NULL )
    {
        fprintf( stderr, "Out of memory for buffers\n");
        uninit();
        return;
    }

    buffers->length = fmt.fmt.pix.sizeimage;
    buffers->start  = malloc( fmt.fmt.pix.sizeimage);

    if( buffers->start == NULL)
    {
        fprintf( stderr, "Out of memory for buffers->start\n");
        uninit();
        return;
    }

    width  = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;
}

void CameraSource_V4L2::uninit(void)
{
    if(buffers)
    {
        if( buffers->start )
            free( buffers->start );
        free( buffers );
        buffers = NULL;
    }

    if( camera_fd > 0 )
    {
        close( camera_fd );
        camera_fd = -1;
    }
}

CameraSource_V4L2::~CameraSource_V4L2()
{
    uninit();
}

bool CameraSource_V4L2::_getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL)
{
}

bool CameraSource_V4L2::_getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL)
{
    struct v4l2_buffer buf;
    unsigned int i;

    if( read( camera_fd, buffers->start, buffers->length) < 0 )
    {
        perror ("camera read");
        return false;
    }

    process_image( buffers->start);

    return true;
}

bool CameraSource_V4L2::_stopStream(void)
{
    enum v4l2_buf_type type;

    if(DC1394_SUCCESS == dc1394_video_set_transmission(camera, DC1394_OFF))
    {
        purgeBuffer();
        return true;
    }
    return false;
}

bool CameraSource_V4L2::_resumeStream(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    purgeBuffer();
    return DC1394_SUCCESS == dc1394_video_set_transmission(camera, DC1394_ON);
}
