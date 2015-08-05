#include <assert.h>
#include "ffmpegInterface.hh"

#define OUTPUT_PIX_FMT      PIX_FMT_RGB32 /* pixel format always uses color. ffv1 doesn't support grayscale */
#define OUTPUT_CODEC        AV_CODEC_ID_FFV1
#define OUTPUT_GOP_SIZE     0
#define OUTPUT_MAX_B_FRAMES 0
#define OUTPUT_FLAGS        0
#define OUTPUT_FLAGS2       0
#define OUTPUT_THREADS      2

FFmpegTalker::FFmpegTalker()
{
    av_register_all();
    reset();
}
void FFmpegTalker::reset(void)
{
    m_bOpen         = false;
    m_bOK           = true;

    m_pFormatCtx      = NULL;
    m_pCodecCtx       = NULL;
    m_pFrameYUV       = NULL;
    m_pSWSCtx         = NULL;
}

void FFmpegEncoder::reset(void)
{
    m_pOutputFormat     = NULL;
    m_pStream           = NULL;
    m_bufferYUV         = NULL;
    m_bufferYUVSize     = -1;
    m_bufferEncoded     = NULL;
    m_bufferEncodedSize = -1;
    FFmpegTalker::reset();
}

void FFmpegDecoder::reset(void)
{
    m_videoStream   = -1;

    FFmpegTalker::reset();
}

FFmpegTalker::~FFmpegTalker()
{
    free();
}
void FFmpegTalker::free(void)
{
    if(m_pFrameYUV)    av_free(m_pFrameYUV);

    if(m_pSWSCtx)      sws_freeContext(m_pSWSCtx);

    if(m_pCodecCtx)
    {
        if(m_pCodecCtx->codec != NULL)
        {
            avcodec_close(m_pCodecCtx);
        }
    }
}
void FFmpegDecoder::free(void)
{
    FFmpegTalker::free();

    if(m_pFormatCtx)
        avformat_close_input(&m_pFormatCtx);
    reset();
}
void FFmpegEncoder::free(void)
{
    FFmpegTalker::free();

    if(m_bufferYUV)
        av_free(m_bufferYUV);
    if(m_bufferEncoded)
        av_free(m_bufferEncoded);
    if(m_pStream)
        av_free(m_pStream);
    if(m_pCodecCtx)
        av_free(m_pCodecCtx);
    if(m_pFormatCtx)
        av_free(m_pFormatCtx);

    reset();
}

void FFmpegDecoder::close(void)
{
    free();
    isRunningNow.reset();
}
void FFmpegEncoder::close(void)
{
    if(m_bOpen)
    {
        av_write_trailer(m_pFormatCtx);
        avio_close(m_pFormatCtx->pb);
        m_nChannels = -1;
    }
    free();
}


bool FFmpegDecoder::open(const char* filename,
                         CvRect _cropRect, double scale)
{
    if(m_bOpen)
    {
        cerr << "FFmpegDecoder: trying to open a file while we're already open. Doing nothing." << endl;
        return true;
    }
    if(!m_bOK)
    {
        cerr << "FFmpegDecoder: trying to open a file while we're not ok. Reseting and trying again." << endl;
        close();
    }
    m_bOK = false;

    if(avformat_open_input(&m_pFormatCtx, filename, NULL, NULL) != 0)
    {
        cerr << "ffmpeg: couldn't open input file" << endl;
        return false;
    }

    // this dumps info into stdout
    //   if(av_find_stream_info(m_pFormatCtx) < 0)
    //   {
    //     cerr << "ffmpeg: couldn't find stream info" << endl;
    //     return false;
    //   }
    //   dump_format(pFormatCtx, 0, filename, 0);

    // Find the first video stream
    m_videoStream = -1;
    for(unsigned int i=0; i<m_pFormatCtx->nb_streams; i++)
    {
        if(m_pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
            break;
        }
    }
    if(m_videoStream == -1)
    {
        cerr << "ffmpeg: couldn't find first video stream" << endl;
        return false;
    }
    m_pCodecCtx = m_pFormatCtx->streams[m_videoStream]->codec;

    AVCodec* pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        cerr << "ffmpeg: couldn't find decoder" << endl;
        return false;
    }

    if(avcodec_open2(m_pCodecCtx, pCodec, NULL) < 0)
    {
        cerr << "ffmpeg: couldn't open codec" << endl;
        return false;
    }

    m_pFrameYUV = av_frame_alloc();

    m_bOpen = m_bOK = true;

    width  = m_pCodecCtx->width;
    height = m_pCodecCtx->height;

    setupCroppingScaling(_cropRect, scale);

    isRunningNow.setTrue();

    return true;
}

bool FFmpegDecoder::readFrame(IplImage* image)
{
    if(!m_bOpen || !m_bOK)
        return false;

    AVPacket packet;
    int frameFinished;

    // I keep reading frames as long as I can. If asked, I start over from the beginning when I
    // reach the end
    while(av_read_frame(m_pFormatCtx, &packet) >= 0 ||
          (m_loopAtEnd && _restartStream() && av_read_frame(m_pFormatCtx, &packet) >= 0))
    {
        if(packet.stream_index == m_videoStream)
        {
            if(avcodec_decode_video2(m_pCodecCtx, m_pFrameYUV, &frameFinished,
                                     &packet) < 0)
            {
                cerr << "ffmpeg error avcodec_decode_video()" << endl;
                return false;
            }

            if(frameFinished)
            {
                if(m_pSWSCtx == NULL)
                {
                    // I do this here instead of in the constructor because I was seeing the codec
                    // pixel format not being defined at the time the constructor runs. Maybe it
                    // needs to read at least one frame to figure it out. If we can, this SHOULD go
                    // to the constructor
                    m_pSWSCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                                               m_pCodecCtx->width, m_pCodecCtx->height,
                                               userColorMode == FRAMESOURCE_COLOR ? PIX_FMT_RGB24 : PIX_FMT_GRAY8,
                                               SWS_POINT, NULL, NULL, NULL);
                    if(m_pSWSCtx == NULL)
                    {
                        cerr << "ffmpeg: couldn't create sws context" << endl;
                        return false;
                    }
                }

                IplImage* buffer;
                if(preCropScaleBuffer == NULL) buffer = image;
                else                           buffer = preCropScaleBuffer;

                assert( (userColorMode == FRAMESOURCE_COLOR     && buffer->nChannels == 3) ||
                        (userColorMode == FRAMESOURCE_GRAYSCALE && buffer->nChannels == 1) );
                assert( buffer->width == (int)m_pCodecCtx->width && buffer->height == (int)m_pCodecCtx->height );

                sws_scale(m_pSWSCtx,
                          m_pFrameYUV->data, m_pFrameYUV->linesize,
                          0, m_pCodecCtx->height,
                          (unsigned char**)&buffer->imageData, &buffer->widthStep);

                if(preCropScaleBuffer != NULL)
                {
#warning this isnt very efficient. The cropping and scaling should be a part of the ffmpeg calls above. av_picture_crop looks promising
                    applyCroppingScaling(preCropScaleBuffer, image);
                }

                av_free_packet(&packet);
                return true;
            }
        }
    }
    return false;
}

bool FFmpegEncoder::open(const char* filename, int width, int height, int fps,
                         enum FrameSource_UserColorChoice sourceColormode)
{
    if(m_bOpen)
    {
        cerr << "FFmpegEncoder: trying to open a file while we're already open. Doing nothing." << endl;
        return true;
    }
    if(!m_bOK)
    {
        cerr << "FFmpegDecoder: trying to open a file while we're not ok. Reseting and trying again." << endl;
        close();
    }
    m_bOK = false;

    m_pOutputFormat = av_guess_format(NULL, "blah.avi", NULL);
    if(!m_pOutputFormat)
    {
        cerr << "ffmpeg: guess_format couldn't figure it out" << endl;
        return false;
    }

    m_pFormatCtx = avformat_alloc_context();
    if(m_pFormatCtx == NULL)
    {
        cerr << "ffmpeg: couldn't alloc format context" << endl;
        return false;
    }

    // set the output format and filename
    m_pFormatCtx->oformat = m_pOutputFormat;
    strncpy(m_pFormatCtx->filename, filename, sizeof(m_pFormatCtx->filename));

    m_pStream = avformat_new_stream(m_pFormatCtx, NULL);
    if(m_pStream == NULL)
    {
        cerr << "ffmpeg: av_new_stream failed" << endl;
        return false;
    }

    m_pCodecCtx                = m_pStream->codec;
    m_pCodecCtx->codec_type    = AVMEDIA_TYPE_VIDEO;
    m_pCodecCtx->codec_id      = OUTPUT_CODEC;
    m_pCodecCtx->bit_rate      = 1000000;
    m_pCodecCtx->flags         = OUTPUT_FLAGS;
    m_pCodecCtx->flags2        = OUTPUT_FLAGS2;
    m_pCodecCtx->thread_count  = OUTPUT_THREADS;
    m_pCodecCtx->width         = width;
    m_pCodecCtx->height        = height;
    m_pCodecCtx->time_base.num = 1;
    m_pCodecCtx->time_base.den = fps; // frames per second
    m_pCodecCtx->gop_size      = OUTPUT_GOP_SIZE;
    m_pCodecCtx->max_b_frames  = OUTPUT_MAX_B_FRAMES;
    m_pCodecCtx->pix_fmt       = OUTPUT_PIX_FMT;

    AVCodec* pCodec = avcodec_find_encoder(m_pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        cerr << "ffmpeg: couldn't find encoder. Available:" << endl;

        while( (pCodec = av_codec_next(pCodec)) != NULL )
        {
            if (pCodec->encode2)
                cerr << pCodec->id << ": " << pCodec->name << endl;
        }
        return false;
    }

    if(avcodec_open2(m_pCodecCtx, pCodec, NULL) < 0)
    {
        cerr << "ffmpeg: couldn't open codec" << endl;
        return false;
    }

    m_pFrameYUV = av_frame_alloc();
    if(m_pFrameYUV == NULL)
    {
        cerr << "ffmpeg: couldn't alloc frame" << endl;
        return false;
    }
    m_bufferYUVSize = avpicture_get_size(m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height);
    m_bufferYUV     = (uint8_t*)av_malloc(m_bufferYUVSize * sizeof(uint8_t));

    // The ffv1 encoder uses this much data. It seems like too much, but I just
    // give it what it wants
    m_bufferEncodedSize = m_pCodecCtx->width * m_pCodecCtx->height *
      ((8 * 2 + 1 + 1) * 4) / 8 + FF_MIN_BUFFER_SIZE;
    m_bufferEncoded     = (uint8_t*)av_malloc(m_bufferEncodedSize);

    avpicture_fill((AVPicture *)m_pFrameYUV, m_bufferYUV, m_pCodecCtx->pix_fmt,
                   m_pCodecCtx->width, m_pCodecCtx->height);

    // open the file
    if(avio_open(&m_pFormatCtx->pb, filename, AVIO_FLAG_WRITE) < 0)
    {
        cerr << "ffmpeg: couldn't open file " << filename << endl;
        return false;
    }

    avformat_write_header(m_pFormatCtx, NULL);

    m_nChannels = sourceColormode == FRAMESOURCE_GRAYSCALE ? 1 : 3;
    m_pSWSCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height,
                               sourceColormode == FRAMESOURCE_GRAYSCALE ? PIX_FMT_GRAY8 : PIX_FMT_RGB24,
                               m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                               SWS_POINT, NULL, NULL, NULL);
    if(m_pSWSCtx == NULL)
    {
        cerr << "ffmpeg: couldn't create sws context" << endl;
        return false;
    }

    m_bOpen = m_bOK = true;
    return true;
}

bool FFmpegEncoder::writeFrame(IplImage* image)
{
    if(!m_bOpen || !m_bOK)
        return false;

    assert(image->width  == m_pCodecCtx->width &&
           image->height == m_pCodecCtx->height);
    assert( image->nChannels == m_nChannels );
    assert( image->depth == IPL_DEPTH_8U );

    sws_scale(m_pSWSCtx,
              (unsigned char**)&image->imageData, &image->widthStep,
              0, m_pCodecCtx->height,
              m_pFrameYUV->data, m_pFrameYUV->linesize);


    AVPacket packet;
    av_init_packet(&packet);
    packet.stream_index = m_pStream->index;
    packet.data         = m_bufferEncoded;
    packet.size         = m_bufferEncodedSize;

    int got_packet_ptr;

    int outsize = avcodec_encode_video2(m_pCodecCtx,
                                        &packet,
                                        m_pFrameYUV,
                                        &got_packet_ptr);
    if(outsize != 0 || got_packet_ptr == 0)
    {
        cerr << "ffmpeg: couldn't write frame. Error: " << outsize << endl;
        return false;
    }

    av_write_frame(m_pFormatCtx, &packet);
    return true;
}
