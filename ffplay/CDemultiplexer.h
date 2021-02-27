#pragma once
#include "CVideoDecoder.h"
#include "CAudioDecoder.h"
#include "CDemultiplexer.h"

// 解复用器
class CDemultiplexer
{
public:
	CDemultiplexer();
	~CDemultiplexer();

	bool Open(const char* szInput);

	bool Start(CVideoDecoder* pVideoDecoder, CAudioDecoder* pAudioDecoder);

	void Close();

	int GetVideoIndex();
	int GetAudioIndex();
	AVFormatContext* GetFormatContext();

private:
	void OnDemuxThread();

private:
	volatile bool m_bRun = false;
	AVFormatContext* m_pFormatContext = nullptr;
	int m_videoIndex = -1;
	int m_audioIndex = -1;
	std::thread m_demuxThread;	// 解复用线程

	CVideoDecoder* m_pVideoDecoder = nullptr;
	CAudioDecoder* m_pAudioDecoder = nullptr;
};

