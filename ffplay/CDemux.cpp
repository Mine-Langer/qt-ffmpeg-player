#include "CDemux.h"

CDemux::CDemux()
{
	m_avFormatCtx = avformat_alloc_context();
}

CDemux::~CDemux()
{
	avformat_free_context(m_avFormatCtx);
}

bool CDemux::Open(const char* szInput)
{
	if (0 != avformat_open_input(&m_avFormatCtx, szInput, nullptr, nullptr))
		return false;

	if (0 > avformat_find_stream_info(m_avFormatCtx, nullptr))
		return false;

	for (int i = 0; i < m_avFormatCtx->nb_streams; i++)
	{
		if (m_avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			m_videoIndex = i;
		else if (m_avFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			m_audioIndex = i;
	}

	// 初始化视频解码器
	if (m_videoDecoder.Init(m_avFormatCtx, m_videoIndex) == false)
		return false;
	if (m_videoDecoder.SetParam() == false)
		return false;
	// 初始化音频解码器


	m_bRun = true; // 开始解析视频流
	m_demuxThread = std::thread(&CDemux::DemuxThreadFunc, this);

	return true;
}

void CDemux::Close()
{
	m_bRun = false;

	if (m_demuxThread.joinable())
		m_demuxThread.join();

	avformat_close_input(&m_avFormatCtx);
}

void CDemux::DemuxThreadFunc()
{
	AVPacket packet;
	while (m_bRun)
	{
		int iRet = av_read_frame(m_avFormatCtx, &packet);
		if (iRet == 0)
		{
			if (packet.stream_index == m_videoIndex)
			{
				m_videoDecoder.SendPacket(&packet);
			}
			else if (packet.stream_index == m_audioIndex)
			{

			}
		}
		else if (iRet == AVERROR_EOF)
		{
			m_bRun = false;
		}
		av_packet_unref(&packet);
	}
}
