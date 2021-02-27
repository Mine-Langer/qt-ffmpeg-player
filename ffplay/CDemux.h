#pragma once
#include "CVideoDecoder.h"

// 解复用模块
class CDemux
{
public: 
	CDemux();
	~CDemux();

	bool Open(const char* szInput);

	// 关闭解复用器 释放资源
	void Close();

private:
	void DemuxThreadFunc();

private:
	volatile bool m_bRun = false;
	AVFormatContext* m_avFormatCtx = nullptr;
	int m_videoIndex = -1;
	int m_audioIndex = -1;

	std::thread m_demuxThread; // 解复用线程

	CVideoDecoder m_videoDecoder; // 视频解码器
};

