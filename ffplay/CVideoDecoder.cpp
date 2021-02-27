#include "CPlayer.h"
#include "CVideoDecoder.h"


CVideoDecoder::CVideoDecoder()
{

}

CVideoDecoder::~CVideoDecoder()
{

}

bool CVideoDecoder::Init(CDemultiplexer* pDemux, CPlayer *player)
{
	if (m_bRun)	return false;

	m_player = player;

	AVStream *pStream = pDemux->GetFormatContext()->streams[pDemux->GetVideoIndex()];

	m_timebase = av_q2d(pStream->time_base);
	m_duration = m_timebase * pStream->duration;

	m_pCodecCtx = avcodec_alloc_context3(nullptr);
	if (m_pCodecCtx == nullptr)
		return false;

	avcodec_parameters_to_context(m_pCodecCtx, pStream->codecpar);

	m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if (m_pCodec == nullptr)
		return false;

	if (0 > avcodec_open2(m_pCodecCtx, m_pCodec, nullptr))
		return false;

	m_srcFrame = av_frame_alloc();
	m_dstFrame = av_frame_alloc();

	m_rate = av_q2d(pStream->avg_frame_rate);
	
	return true;
}

bool CVideoDecoder::SetConfig(int dstWidth /*= -1*/, int dstHeight /*= -1*/, AVPixelFormat dstFmt /*= AV_PIX_FMT_NONE*/, int scaleFlag /*= SWS_FAST_BILINEAR*/)
{
	if (m_bRun)	return false;

	m_swsFmt = dstFmt != AV_PIX_FMT_NONE ? dstFmt : m_pCodecCtx->pix_fmt;
	m_swsWidth = dstWidth != -1 ? dstWidth : m_pCodecCtx->width;
	m_swsHeight = dstHeight != -1 ? dstHeight : m_pCodecCtx->height;

	m_swsCtx = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
		m_swsWidth, m_swsHeight, m_swsFmt, scaleFlag, nullptr, nullptr, nullptr);
	if (m_swsCtx == nullptr)
		return false;

	m_dstBuffer = (uint8_t *)av_malloc(av_image_get_buffer_size(m_swsFmt, m_swsWidth, m_swsHeight, 1));
	av_image_fill_arrays(m_dstFrame->data, m_dstFrame->linesize, m_dstBuffer, m_swsFmt, m_swsWidth, m_swsHeight, 1);

	return true;
}

bool CVideoDecoder::Start()
{
	if (m_bRun)	return false;

	m_bRun = true;

	m_videoThread = std::thread(&CVideoDecoder::DecodeVideoThread, this);

	return true;
}

void CVideoDecoder::Close()
{
	m_bRun = false;
	
	if (m_videoThread.joinable())
		m_videoThread.join();

	if (m_dstBuffer)
	{
		av_free(m_dstBuffer);
		m_dstBuffer = nullptr;
	}
	
	if (m_srcFrame)
	{
		av_frame_free(&m_srcFrame);
		m_srcFrame = nullptr;
	}

	if (m_dstFrame)
	{
		av_frame_free(&m_dstFrame);
		m_dstFrame = nullptr;
	}

	if (m_pCodecCtx)
	{
		avcodec_close(m_pCodecCtx);
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = nullptr;
		m_pCodec = nullptr;
	}

	if (m_swsCtx)
	{
		sws_freeContext(m_swsCtx);
		m_swsCtx = nullptr;
	}

	AVPacket *pkt = nullptr;
	while (m_queuePacket.TryPop(pkt))
	{
		if (pkt)
			av_packet_free(&pkt);
	}
}

bool CVideoDecoder::SendPacket(AVPacket *pkt)
{
	AVPacket *packet = av_packet_clone(pkt);
	m_queuePacket.MaxSizePush(packet, &m_bRun);

	return true;
}

void CVideoDecoder::DecodeVideoThread()
{
	int iRet = 0;
	AVPacket *pkt = nullptr;
	while (m_bRun)
	{
		if (m_queuePacket.TryPop(pkt))
		{
			if (pkt == nullptr)
			{
				m_bRun = false;
			}
			else
			{
				if (0 > avcodec_send_packet(m_pCodecCtx, pkt))
					m_bRun = false;
				else
				{
					while (iRet = avcodec_receive_frame(m_pCodecCtx, m_srcFrame), m_bRun)
					{
						if (iRet == AVERROR(EAGAIN) || iRet == AVERROR_EOF)
							break;
						else if (iRet < 0)
							m_bRun = false;
						else if (iRet == 0)
						{
							sws_scale(m_swsCtx, m_srcFrame->data, m_srcFrame->linesize, 0,
								m_pCodecCtx->height, m_dstFrame->data, m_dstFrame->linesize);

							m_dstFrame->pts = m_srcFrame->pts;
							m_dstFrame->best_effort_timestamp = m_srcFrame->best_effort_timestamp;

							// show video
							m_player->VideoEvent(m_dstFrame, this);
						}
					}
				}
				av_packet_free(&pkt);
			}
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
}

int CVideoDecoder::GetDstWidth()
{
	return m_swsWidth;
}
int CVideoDecoder::GetDstHeight()
{
	return m_swsHeight;
}
AVPixelFormat CVideoDecoder::GetDstFormat()
{
	return m_swsFmt;
}
double CVideoDecoder::GetTimebase()
{
	return m_timebase;
}
double CVideoDecoder::GetRate()
{
	return m_rate;
}