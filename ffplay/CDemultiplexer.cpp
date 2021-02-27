#include "Common.h"
#include "CDemultiplexer.h"

CDemultiplexer::CDemultiplexer()
{

}

CDemultiplexer::~CDemultiplexer()
{
	Close();
}

bool CDemultiplexer::Open(const char* szInput)
{
	if (m_bRun)
		return false;

	m_pFormatContext = avformat_alloc_context();
	if (m_pFormatContext == nullptr)
		return false;

	if (0 != avformat_open_input(&m_pFormatContext, szInput, nullptr, nullptr))
		return false;

	if (0 > avformat_find_stream_info(m_pFormatContext, nullptr))
		return false;

	for (int i=0; i<m_pFormatContext->nb_streams; i++)
	{
		if (m_pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			m_videoIndex = i;
		else if (m_pFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			m_audioIndex = i;
	}

	return true;
}

bool CDemultiplexer::Start(CVideoDecoder* pVideoDecoder, CAudioDecoder* pAudioDecoder)
{
	if (m_bRun) return false;

	m_pVideoDecoder = pVideoDecoder;
	m_pAudioDecoder = pAudioDecoder;

	m_bRun = true;
	m_demuxThread = std::thread(&CDemultiplexer::OnDemuxThread, this);

	return true;
}

void CDemultiplexer::Close()
{
	m_bRun = false;

	if (m_demuxThread.joinable())
		m_demuxThread.join();

	if (m_pFormatContext)
	{
		avformat_close_input(&m_pFormatContext);
		avformat_free_context(m_pFormatContext);
		m_pFormatContext = nullptr;
	}
}

int CDemultiplexer::GetVideoIndex()
{
	return m_videoIndex;
}

int CDemultiplexer::GetAudioIndex()
{
	return m_audioIndex;
}

AVFormatContext* CDemultiplexer::GetFormatContext()
{
	return m_pFormatContext;
}

void CDemultiplexer::OnDemuxThread()
{
	int iRet = 0;
	AVPacket *pkt = av_packet_alloc();

	while (m_bRun)
	{
		iRet = av_read_frame(m_pFormatContext, pkt);
		if (iRet == 0)
		{
			if (pkt->stream_index == m_videoIndex)
			{
				m_pVideoDecoder->SendPacket(pkt);
			}
			else if (pkt->stream_index == m_audioIndex)
			{
				m_pAudioDecoder->SendPacket(pkt);
			}
		}
		else if (iRet == AVERROR_EOF)
		{
			m_bRun = false;
		}
		av_packet_unref(pkt);
	}
}
