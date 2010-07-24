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
    bool             m_loopAtEnd;

    void reset(void);
    bool readFrame(IplImage* image);

public:
    FFmpegDecoder(FrameSource_UserColorChoice _userColorMode, bool loopAtEnd = false)
        : FFmpegTalker(), FrameSource(_userColorMode), m_loopAtEnd(loopAtEnd)
    {}
    FFmpegDecoder(const char* filename, FrameSource_UserColorChoice _userColorMode,
                  bool loopAtEnd = false,
                  CvRect _cropRect = cvRect(-1, -1, -1, -1),
                  double scale = 1.0)
        : FFmpegTalker(), FrameSource(_userColorMode), m_loopAtEnd(loopAtEnd)
    {
        open(filename, _cropRect, scale);
    }
    ~FFmpegDecoder()
    {
        cerr << "~FFmpegDecoder" << endl;
        cleanupThreads();
        close();
    }

    bool open(const char* filename,
              CvRect _cropRect = cvRect(-1, -1, -1, -1),
              double scale = 1.0);
    void close(void);
    void free(void);

    operator bool()
    {
        return m_bOpen && m_bOK;
    }

private:
    // These support the FrameSource API
    bool _getNextFrame  (IplImage* image, uint64_t* timestamp_us = NULL)
    {
        if(!readFrame(image))
            return false;

        if(timestamp_us != NULL)
            // I've seen m_pCodecCtx->time_base.num==0 before. In that case I treat it as 1
            *timestamp_us =
                (uint64_t)m_pCodecCtx->frame_number *
                (m_pCodecCtx->time_base.num == 0 ? 1ul : (uint64_t)m_pCodecCtx->time_base.num) *
                (uint64_t)1000000 / m_pCodecCtx->time_base.den;
        return true;
    }

    // _getLatestFrame() and _getNextFrame() are identical here since I pull off the frames when
    // asked, without regard to the actual framerate
    bool _getLatestFrame(IplImage* image, uint64_t* timestamp_us = NULL)
    {
        return _getNextFrame(image, timestamp_us);
    }

    // static images don't have any hardware on/off switch. Thus these functions are stubs
    bool _stopStream   (void) { return true; }
    bool _resumeStream (void) { return true; }

    bool _restartStream(void)
    {
        // I rewind to the start of the file
        if(0 > av_seek_frame(m_pFormatCtx, m_videoStream,
                             0, AVSEEK_FLAG_BYTE))
        {
            cerr << "_restartStream(): ffmpeg couldn't rewind to the start of the file" << endl;
            return false;
        }
        return true;
    }
};

class FFmpegEncoder : public FFmpegTalker
{
    AVOutputFormat*  m_pOutputFormat;
    AVStream*        m_pStream;
    uint8_t*         m_bufferYUV;
    int              m_bufferYUVSize;
    uint8_t*         m_bufferEncoded;
    int              m_bufferEncodedSize;

    void reset(void);

public:
    FFmpegEncoder()
        : FFmpegTalker()
    {}
    FFmpegEncoder(const char* filename, int width, int height, int fps, enum FrameSource_UserColorChoice sourceColormode)
        : FFmpegTalker()
    {
        open(filename, width, height, fps, sourceColormode);
    }
    ~FFmpegEncoder()
    {
        cerr << "~FFmpegEncoder" << endl;
        close();
    }

    bool open(const char* filename, int width, int height, int fps, enum FrameSource_UserColorChoice sourceColormode);
    bool writeFrameGrayscale(IplImage* image);
    void close(void);
    void free(void);

    operator bool()
    {
        return m_bOpen && m_bOK;
    }
};

#endif
