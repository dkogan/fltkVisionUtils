#include "ffmpegInterface.hh"
#include <iostream>
using namespace std;

FFmpegTalker::FFmpegTalker()
{
  av_register_all();

  m_pFormatCtx = NULL;
  m_videoStream = -1;
  m_pCodecCtx = NULL;
  m_pFrame = NULL;
  m_pFrameRGB = NULL;
  m_buffer = NULL;
}
FFmpegTalker::~FFmpegTalker()
{
  if(m_buffer)
    av_free(m_buffer);
  if(m_pFrameRGB)
    av_free(m_pFrameRGB);
  if(m_pFrame)
    av_free(m_pFrame);
  if(m_pCodecCtx)
    avcodec_close(m_pCodecCtx);
  if(m_pFormatCtx)
    av_close_input_file(m_pFormatCtx);
}

bool FFmpegTalker::open(const char* filename)
{
  if(av_open_input_file(&m_pFormatCtx, filename, NULL, 0, NULL) != 0)
  {
    cerr << "ffmpeg: couldn't open input file" << endl;
    return false;
  }

  if(av_find_stream_info(m_pFormatCtx) < 0)
  {
    cerr << "ffmpeg: couldn't find stream info" << endl;
    return false;
  }

  // this dumps info into stdout
  //  dump_format(pFormatCtx, 0, filename, 0);

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
  m_pFrame = avcodec_alloc_frame();

  m_pFrameRGB = avcodec_alloc_frame();
  if(m_pFrameRGB == NULL)
  {
    cerr << "ffmpeg: couldn't alloc rgb frame" << endl;
    return false;
  }  
  // Determine required buffer size and allocate buffer
  int numBytes = avpicture_get_size(PIX_FMT_RGB24, m_pCodecCtx->width, m_pCodecCtx->height);
  m_buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)m_pFrameRGB, m_buffer, PIX_FMT_RGB24,
                 m_pCodecCtx->width, m_pCodecCtx->height);

  return true;
}

bool FFmpegTalker::readFrameGrayscale(unsigned char* pBuffer)
{
  AVPacket packet;
  int frameFinished;

  while(av_read_frame(m_pFormatCtx, &packet) >= 0)
  {
    if(packet.stream_index == m_videoStream)
    {
      avcodec_decode_video(m_pCodecCtx, m_pFrame, &frameFinished, 
                           packet.data, packet.size);
      if(frameFinished)
      {
        img_convert((AVPicture *)m_pFrameRGB, PIX_FMT_RGB24, 
                    (AVPicture*)m_pFrame, m_pCodecCtx->pix_fmt, m_pCodecCtx->width, 
                    m_pCodecCtx->height);

        for(int y=0; y<m_pCodecCtx->height; y++)
        {
          unsigned char* pDat = m_pFrameRGB->data[0] + y*m_pFrameRGB->linesize[0];
          for(int x=0; x<m_pCodecCtx->width; x++)
          {
            *pBuffer = (unsigned char)(((int)pDat[3*x] + (int)pDat[3*x + 1] + (int)pDat[3*x + 2]) / 3);
            pBuffer++;
          }
        }
        break;
  
      }
    }
    
    av_free_packet(&packet);
  }
  
  return true;
}
