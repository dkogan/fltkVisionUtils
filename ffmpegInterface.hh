#ifndef _FFMPEG_TALKER_H_
#define _FFMPEG_TALKER_H_

#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

class FFmpegTalker
{
  AVFormatContext* m_pFormatCtx;
  int              m_videoStream;
  AVCodecContext*  m_pCodecCtx;
  AVFrame*         m_pFrame; 
  AVFrame*         m_pFrameRGB;
  uint8_t*         m_buffer;

public:
  FFmpegTalker();
  ~FFmpegTalker();
  bool open(const char* filename);
  bool readFrameGrayscale(unsigned char* pBuffer);
};

#endif //_FFMPEG_TALKER_H_
