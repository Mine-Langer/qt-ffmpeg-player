#pragma once
#include "CQueue.h"

class CDemultiplexer;
class CPlayer;
class CAudioDecoder
{
public:
	CAudioDecoder();
	~CAudioDecoder();

	bool Init(CDemultiplexer* pDemux, CPlayer *player);

	bool SetSwr(int64_t chLayout, AVSampleFormat sampleFmt, int simpleRate);

	bool Start();

	bool SendPacket(AVPacket* pkt);

	void Close();

	double GetTimebase();
	double GetRate();
	int GetSampleRate();
	int GetNbSamples();
	int GetChannels();
	AVSampleFormat GetSampleFmt();

private:
	void DecodeAudioThread();

private:
	volatile bool m_bRun = false;
	CPlayer *m_player = nullptr;

	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;

	double m_timebase;
	double m_duration;
	double m_rate;

	std::thread m_audioThread;
	CQueue<AVPacket*> m_queueAudioPkt;

	AVFrame* m_srcFrame = nullptr;
	SwrContext* m_swrCtx = nullptr;
	int64_t m_swrChLayout = 0;
	AVSampleFormat m_swrSampleFmt = AV_SAMPLE_FMT_NONE;
	int m_swrSampleRate = 0;
	int m_swrNbSamples = 0;
	int m_nChannels = 0;
};

