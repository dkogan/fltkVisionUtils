#ifndef _FFMPEG_TALKER_H_
#define _FFMPEG_TALKER_H_

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <iostream>
using namespace std;

class FFmpegTalker
{
protected:
    AVFormatContext* m_pFormatCtx;
    AVCodecContext*  m_pCodecCtx;
    AVFrame*         m_pFrameYUV;
    SwsContext*      m_pSWSCtx;

    bool             m_bOpen;
    bool             m_bOK;

    virtual void initVars(void);
    virtual void free(void);

public:
    FFmpegTalker();
    virtual ~FFmpegTalker();

    virtual bool open(const char* filename) = 0;
    virtual void close(void) = 0;
    bool operator!()
    {
        return !m_bOK;
    }
    bool isOpen(void)
    {
        return m_bOpen;
    }
};

class FFmpegDecoder : public FFmpegTalker
{
    int              m_videoStream;

public:
    FFmpegDecoder()
        : FFmpegTalker()
    {}
    FFmpegDecoder(const char* filename)
        : FFmpegTalker()
    {
        open(filename);
    }
    ~FFmpegDecoder()
    {
        cerr << "~FFmpegDecoder" << endl;
        close();
    }

    void initVars(void);

    bool open(const char* filename);
    bool readFrameGrayscale(unsigned char* pBuffer);
    void close(void);
    void free(void);

    bool operator>>(unsigned char* pBuffer)
    {
        return readFrameGrayscale(pBuffer);
    }
};

class FFmpegEncoder : public FFmpegTalker
{
    AVOutputFormat*  m_pOutputFormat;
    AVStream*        m_pStream;
    uint8_t*         m_bufferYUV;
    int              m_bufferYUVSize;

public:
    FFmpegEncoder()
        : FFmpegTalker()
    {}
    FFmpegEncoder(const char* filename)
        : FFmpegTalker()
    {
        open(filename);
    }
    ~FFmpegEncoder()
    {
        cerr << "~FFmpegEncoder" << endl;
        close();
    }

    void initVars(void);

    bool open(const char* filename);
    bool writeFrameGrayscale(unsigned char* pBuffer);
    void close(void);
    void free(void);

    bool operator<<(unsigned char* pBuffer)
    {
        return writeFrameGrayscale(pBuffer);
    }
};

#endif //_FFMPEG_TALKER_H_
