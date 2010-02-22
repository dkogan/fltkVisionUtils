#ifndef __FFMPEG_TALKER_H__
#define __FFMPEG_TALKER_H__

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <iostream>
using namespace std;

#include "frameSource.hh"

class FFmpegTalker
{
protected:
    AVFormatContext* m_pFormatCtx;
    AVCodecContext*  m_pCodecCtx;
    AVFrame*         m_pFrameYUV;
    SwsContext*      m_pSWSCtx;

    bool             m_bOpen;
    bool             m_bOK;

    virtual void reset(void);
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

class FFmpegDecoder : public FFmpegTalker, public FrameSource
{
    int              m_videoStream;

    void reset(void);

public:
    FFmpegDecoder()
        : FFmpegTalker(), FrameSource(FRAMESOURCE_GRAYSCALE)
    {}
    FFmpegDecoder(const char* filename)
        : FFmpegTalker(), FrameSource(FRAMESOURCE_GRAYSCALE)
    {
        open(filename);
    }
    ~FFmpegDecoder()
    {
        cerr << "~FFmpegDecoder" << endl;
        close();
    }

    bool open(const char* filename);
    bool readFrameGrayscale(unsigned char* pBuffer);
    void close(void);
    void free(void);

    bool operator>>(unsigned char* pBuffer)
    {
        return readFrameGrayscale(pBuffer);
    }


    // These support the FrameSource API
    unsigned char* peekNextFrame  (uint64_t* timestamp_us)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return NULL;
    }

    unsigned char* peekLatestFrame(uint64_t* timestamp_us)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return NULL;
    }

    void unpeekFrame(void)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return NULL;
    }

    bool getNextFrame  (uint64_t* timestamp_us, unsigned char* buffer)
    {
        if(!readFrameGrayscale(buffer))
            return false;

        *timestamp_us =
            (uint64_t)m_pCodecCtx->frame_number *
            (uint64_t)m_pCodecCtx->time_base.num *
            1000000ull / m_pCodecCtx->time_base.den;
    }

    bool getLatestFrame(uint64_t* timestamp_us, unsigned char* buffer)
    {
        return getNextFrame(timestamp_us, buffer);
    }
};

class FFmpegEncoder : public FFmpegTalker
{
    AVOutputFormat*  m_pOutputFormat;
    AVStream*        m_pStream;
    uint8_t*         m_bufferYUV;
    int              m_bufferYUVSize;

    void reset(void);

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

    bool open(const char* filename);
    bool writeFrameGrayscale(unsigned char* pBuffer);
    void close(void);
    void free(void);

    bool operator<<(unsigned char* pBuffer)
    {
        return writeFrameGrayscale(pBuffer);
    }
};

#endif
