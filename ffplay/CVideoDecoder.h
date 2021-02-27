#pragma once
#include "Common.h"
#include "SafeQueue.h"

class CVideoDecoder
{
public:
	CVideoDecoder();
	~CVideoDecoder();

	bool Init(AVFormatContext* pFormatCtx, int mediaIndex);

	bool SendPacket(AVPacket* pkt);

	bool SetParam(int width=-1, int height=-1, AVPixelFormat fmt=AV_PIX_FMT_NONE, int scaleFlag=SWS_FAST_BILINEAR);

	bool Start();

private:
	void DecodeThreadFunc();

private:
	volatile bool m_bRun = false;
	AVCodecContext* m_avCodecCtx = nullptr;
	AVCodec* m_avCodec = nullptr;
	SwsContext* m_swsCtx = nullptr;
	
	AVFrame* m_srcFrame = nullptr;
	AVFrame* m_dstFrame = nullptr;
	uint8_t* m_dstBuffer = nullptr;

	std::thread m_decodeThread;

	SafeQueue<AVPacket*> m_queueData;
};

