#include "ffmpegInterface.hh"

#define OUTPUT_PIX_FMT PIX_FMT_YUV420P

FFmpegTalker::FFmpegTalker()
{
  av_register_all();
  initVars();
}
void FFmpegTalker::initVars(void)
{
  m_bOpen         = false;
  m_bOK           = true;

  m_pOutputFormat = NULL;
  m_pStream       = NULL;
  m_pFormatCtx    = NULL;
  m_pCodecCtx     = NULL;
  m_pFrameYUV     = NULL;
  m_bufferYUV     = NULL;
  m_bufferYUVSize = -1;
  m_pFrameRGB     = NULL;
  m_bufferRGB     = NULL;
  m_bufferRGBSize = -1;
  m_pSWSCtx       = NULL;

  m_videoStream   = -1;
}
FFmpegTalker::~FFmpegTalker()
{
  cerr << "FFmpegTalker::~FFmpegTalker" << endl;
  free();
}
void FFmpegTalker::free(void)
{
  cerr << "FFmpegTalker::free(void)" << endl;
  if(m_bufferRGB)  av_free(m_bufferRGB);
  if(m_pFrameRGB)  av_free(m_pFrameRGB);
  if(m_bufferYUV)  av_free(m_bufferYUV);
  if(m_pFrameYUV)  av_free(m_pFrameYUV);

  if(m_pSWSCtx)    sws_freeContext(m_pSWSCtx);

  if(m_pCodecCtx)
  {
    if(m_pCodecCtx->codec != NULL)
    {
      cerr << "avcodec_close" << endl;
      avcodec_close(m_pCodecCtx);
    }
  }

  if(m_pStream)
    av_free(m_pStream);
}
void FFmpegDecoder::free(void)
{
  cerr << "FFmpegDecoder::free(void)" << endl;
  FFmpegTalker::free();

  if(m_pFormatCtx)
    av_close_input_file(m_pFormatCtx);
  initVars();
}
void FFmpegEncoder::free(void)
{
  cerr << "FFmpegEncoder::free(void)" << endl;

  FFmpegTalker::free();

  if(m_pCodecCtx)
    av_free(m_pCodecCtx);
  if(m_pFormatCtx)
    av_free(m_pFormatCtx);

  initVars();
}

void FFmpegDecoder::close(void)
{
  cerr << "FFmpegDecoder::close(void)" << endl;
  free();
}
void FFmpegEncoder::close(void)
{
  cerr << "FFmpegEncoder::close(void)" << endl;
  if(m_bOpen)
  {
    av_write_trailer(m_pFormatCtx);
    url_fclose(&m_pFormatCtx->pb);
  }
  free();
}


bool FFmpegDecoder::open(const char* filename)
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

  if(av_open_input_file(&m_pFormatCtx, filename, NULL, 0, NULL) != 0)
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
  for(int i=0; i<m_pFormatCtx->nb_streams; i++)
  {
    if(m_pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
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

  if(avcodec_open(m_pCodecCtx, pCodec) < 0)
  {
    cerr << "ffmpeg: couldn't open codec" << endl;
    return false;
  }

  m_pFrameYUV = avcodec_alloc_frame();

  m_pFrameRGB = avcodec_alloc_frame();
  if(m_pFrameRGB == NULL)
  {
    cerr << "ffmpeg: couldn't alloc rgb frame" << endl;
    return false;
  }  
  // Determine required buffer size and allocate buffer
  int m_bufferRGBSize = avpicture_get_size(PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);
  m_bufferRGB         = (uint8_t*)av_malloc(m_bufferRGBSize * sizeof(uint8_t));

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)m_pFrameRGB, m_bufferRGB, PIX_FMT_RGB24,
                 m_pCodecCtx->width, m_pCodecCtx->height);

  m_bOpen = m_bOK = true;
  return true;
}

bool FFmpegDecoder::readFrameGrayscale(unsigned char* pBuffer)
{
  if(!m_bOpen || !m_bOK)
    return false;

  AVPacket packet;
  int frameFinished;

  while(av_read_frame(m_pFormatCtx, &packet) >= 0)
  {
    if(packet.stream_index == m_videoStream)
    {
      avcodec_decode_video(m_pCodecCtx, m_pFrameYUV, &frameFinished, 
                           packet.data, packet.size);
      if(frameFinished)
      {
        if(m_pSWSCtx == NULL)
        {
          m_pSWSCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                                     m_pCodecCtx->width, m_pCodecCtx->height, PIX_FMT_RGB24,
                                     0, NULL, NULL, NULL);
          if(m_pSWSCtx == NULL)
          {
            cerr << "ffmpeg: couldn't create sws context" << endl;
            return false;
          }
        }

        sws_scale(m_pSWSCtx,
                  m_pFrameYUV->data, m_pFrameYUV->linesize, 0, 0,
                  m_pFrameRGB->data, m_pFrameRGB->linesize);

        for(int y=0; y<m_pCodecCtx->height; y++)
        {
          unsigned char* pDat = m_pFrameRGB->data[0] + y*m_pFrameRGB->linesize[0];
          for(int x=0; x<m_pCodecCtx->width; x++)
          {
            *pBuffer = (unsigned char)(((int)pDat[3*x] + (int)pDat[3*x + 1] + (int)pDat[3*x + 2]) / 3);
            pBuffer++;
          }
        }
        av_free_packet(&packet);
        return true;
      }
    }
  }
  return false;
}

bool FFmpegEncoder::open(const char* filename)
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

  m_pOutputFormat = guess_format(NULL, "blah.avi", NULL);
  if(!m_pOutputFormat)
  {
    cerr << "ffmpeg: guess_format couldn't figure it out" << endl;
    return false;
  }

  m_pFormatCtx = av_alloc_format_context();
  if(m_pFormatCtx == NULL)
  {
    cerr << "ffmpeg: couldn't alloc format context" << endl;
    return false;
  }

  // set the output format and filename
  m_pFormatCtx->oformat = m_pOutputFormat;
  strncpy(m_pFormatCtx->filename, filename, sizeof(m_pFormatCtx->filename));

  m_pStream = av_new_stream(m_pFormatCtx, 0);
  if(m_pStream == NULL)
  {
    cerr << "ffmpeg: av_new_stream failed" << endl;
    return false;
  }

  m_pCodecCtx = m_pStream->codec;
  m_pCodecCtx->codec_type   = CODEC_TYPE_VIDEO;
  m_pCodecCtx->codec_id     = CODEC_ID_FFV1;
  m_pCodecCtx->bit_rate     = 1000000;
  m_pCodecCtx->width        = 640;
  m_pCodecCtx->height       = 480;
  m_pCodecCtx->time_base    = (AVRational){1,15}; /* frames per second */
  m_pCodecCtx->gop_size     = 10; /* emit one intra frame every ten frames */
  m_pCodecCtx->max_b_frames = 1;
  m_pCodecCtx->pix_fmt      = OUTPUT_PIX_FMT;

  AVCodec* pCodec = avcodec_find_encoder(m_pCodecCtx->codec_id);
  if(pCodec == NULL)
  {
    cerr << "ffmpeg: couldn't find encoder. Available:" << endl;

    extern AVCodec *first_avcodec;
    pCodec = first_avcodec;
    while(pCodec)
    {
      if (pCodec->encode)
        cerr << pCodec->id << ": " << pCodec->name << endl;
      pCodec = pCodec->next;
    }
    return false;
  }

  if(avcodec_open(m_pCodecCtx, pCodec) < 0)
  {
    cerr << "ffmpeg: couldn't open codec" << endl;
    return false;
  }

  m_pFrameYUV = avcodec_alloc_frame();
  if(m_pFrameYUV == NULL)
  {
    cerr << "ffmpeg: couldn't alloc frame" << endl;
    return false;
  }
  m_bufferYUVSize = avpicture_get_size(m_pCodecCtx->pix_fmt, m_pCodecCtx->width, m_pCodecCtx->height);
  m_bufferYUV     = (uint8_t*)av_malloc(m_bufferYUVSize * sizeof(uint8_t));
  avpicture_fill((AVPicture *)m_pFrameYUV, m_bufferYUV, m_pCodecCtx->pix_fmt,
                 m_pCodecCtx->width, m_pCodecCtx->height);

  m_pFrameRGB = avcodec_alloc_frame();
  if(m_pFrameRGB == NULL)
  {
    cerr << "ffmpeg: couldn't alloc rgb frame" << endl;
    return false;
  }  
  m_bufferRGBSize = avpicture_get_size(PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);
  m_bufferRGB     = (uint8_t*)av_malloc(m_bufferRGBSize * sizeof(uint8_t));
  avpicture_fill((AVPicture *)m_pFrameRGB, m_bufferRGB, PIX_FMT_RGB24,
                 m_pCodecCtx->width, m_pCodecCtx->height);

  // open the file
  if(url_fopen(&m_pFormatCtx->pb, filename, URL_WRONLY) < 0)
  {
    cerr << "ffmpeg: couldn't open file " << filename << endl;
    return false;
  }

  av_write_header(m_pFormatCtx);

  m_pSWSCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, PIX_FMT_RGB24,
                             m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
                             0, NULL, NULL, NULL);
  if(m_pSWSCtx == NULL)
  {
    cerr << "ffmpeg: couldn't create sws context" << endl;
    return false;
  }
  
  m_bOpen = m_bOK = true;
  return true;
}

bool FFmpegEncoder::writeFrameGrayscale(unsigned char* pBuffer)
{
  if(!m_bOpen || !m_bOK)
    return false;




#if 0

  for(int y=0; y<m_pCodecCtx->height; y++)
  {
    unsigned char* pDat = m_pFrameYUV->data[0] + y*m_pFrameYUV->linesize[0];
    for(int x=0; x<m_pCodecCtx->width; x++)
    {
      pDat[4*x] = pDat[4*x + 1] = pDat[4*x + 2] = *pBuffer;
      pDat[4*x + 3] = 255;
      pBuffer++;
    }
  }
#else
  for(int y=0; y<m_pCodecCtx->height; y++)
  {
    unsigned char* pDat = m_pFrameRGB->data[0] + y*m_pFrameRGB->linesize[0];
    for(int x=0; x<m_pCodecCtx->width; x++)
    {
      pDat[3*x] = pDat[3*x + 1] = pDat[3*x + 2] = *pBuffer;
      pBuffer++;
    }
  }

  sws_scale(m_pSWSCtx,
            m_pFrameRGB->data, m_pFrameRGB->linesize, 0, 0,
            m_pFrameYUV->data, m_pFrameYUV->linesize);
#endif


  int outsize = avcodec_encode_video(m_pCodecCtx, m_bufferYUV, m_bufferYUVSize, m_pFrameYUV);
  if(outsize <= 0)
  {
    cerr << "ffmpeg: couldn't write grayscale frame" << endl;
    return false;
  }

  AVPacket packet;
  av_init_packet(&packet);
  packet.stream_index = m_pStream->index;
  packet.data         = m_bufferYUV;
  packet.size         = outsize;
  av_write_frame(m_pFormatCtx, &packet);

  av_free_packet(&packet);
  return true;
}
