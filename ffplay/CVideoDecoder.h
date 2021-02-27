#pragma once
#include "Common.h"
#include "CQueue.h"

class CPlayer;
class CDemultiplexer;
class CVideoDecoder
{
public:
	CVideoDecoder();
	~CVideoDecoder();

	bool Init(CDemultiplexer* pDemux, CPlayer *player);

	bool SetConfig(int dstWidth = -1, int dstHeight = -1, AVPixelFormat dstFmt = AV_PIX_FMT_NONE, int scaleFlag = SWS_FAST_BILINEAR);;

	bool Start();

	void Close();

	bool SendPacket(AVPacket *pkt);

	int GetDstWidth();
	int GetDstHeight();
	AVPixelFormat GetDstFormat();
	double GetTimebase();
	double GetRate();
private:
	void DecodeVideoThread();

private:
	bool m_bRun = false;
	CPlayer *m_player = nullptr;

	AVCodecContext *m_pCodecCtx = nullptr;
	AVCodec *m_pCodec = nullptr;
	
	double m_timebase = 0.0f;
	double m_duration = 0.0f;
	double m_rate = 0.0f;

	AVFrame *m_srcFrame = nullptr;
	AVFrame *m_dstFrame = nullptr;
	uint8_t *m_dstBuffer = nullptr;
	SwsContext *m_swsCtx = nullptr;
	AVPixelFormat m_swsFmt = AV_PIX_FMT_NONE;	// 图像转换目标格式
	int m_swsWidth = 0;		//图像转换目标宽
	int m_swsHeight = 0;	//图像转换目标高

	CQueue<AVPacket*> m_queuePacket;

	std::thread m_videoThread;
};

