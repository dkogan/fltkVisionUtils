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

    virtual void close(void) = 0;
    bool isOpen(void)
    {
        return m_bOpen;
    }
};

class FFmpegDecoder : public FFmpegTalker, public FrameSource
{
    int              m_videoStream;

    void reset(void);
    bool readFrame(unsigned char* pBuffer);

public:
    FFmpegDecoder(FrameSource_UserColorChoice _userColorMode)
        : FFmpegTalker(), FrameSource(_userColorMode)
    {}
    FFmpegDecoder(const char* filename, FrameSource_UserColorChoice _userColorMode)
        : FFmpegTalker(), FrameSource(_userColorMode)
    {
        open(filename);
    }
    ~FFmpegDecoder()
    {
        cerr << "~FFmpegDecoder" << endl;
        cleanupThreads();
        close();
    }

    bool open(const char* filename);
    void close(void);
    void free(void);


    // These support the FrameSource API
    unsigned char* peekNextFrame  (uint64_t* timestamp_us __attribute__((unused)) = NULL)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return NULL;
    }

    unsigned char* peekLatestFrame(uint64_t* timestamp_us __attribute__((unused)) = NULL)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return NULL;
    }

    void unpeekFrame(void)
    {
        cerr << "The FFMPEG decoder does not yet support peeking. use get...Frame()\n";
        return;
    }

    bool getNextFrame  (uint64_t* timestamp_us, unsigned char* buffer)
    {
        if(!readFrame(buffer))
            return false;

        if(timestamp_us != NULL)
            // I've seen m_pCodecCtx->time_base.num==0 before. In that case I treat it as 1
            *timestamp_us =
                (uint64_t)m_pCodecCtx->frame_number *
                (m_pCodecCtx->time_base.num == 0 ? 1ul : (uint64_t)m_pCodecCtx->time_base.num) *
                (uint64_t)1000000 / m_pCodecCtx->time_base.den;
        return true;
    }

    bool getLatestFrame(uint64_t* timestamp_us, unsigned char* buffer)
    {
        return getNextFrame(timestamp_us, buffer);
    }

    operator bool()
    {
        return m_bOpen && m_bOK;
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
    FFmpegEncoder(const char* filename, int width, int height, int fps)
        : FFmpegTalker()
    {
        open(filename, width, height, fps);
    }
    ~FFmpegEncoder()
    {
        cerr << "~FFmpegEncoder" << endl;
        close();
    }

    bool open(const char* filename, int width, int height, int fps);
    bool writeFrameGrayscale(unsigned char* pBuffer);
    void close(void);
    void free(void);

    bool operator<<(unsigned char* pBuffer)
    {
        return writeFrameGrayscale(pBuffer);
    }

    operator bool()
    {
        return m_bOpen && m_bOK;
    }
};

#endif
