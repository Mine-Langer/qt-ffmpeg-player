#include "Common.h"
#include "CAudioDecoder.h"
#include "CPlayer.h"
#include "CDemultiplexer.h"

CAudioDecoder::CAudioDecoder()
{

}

CAudioDecoder::~CAudioDecoder()
{

}

bool CAudioDecoder::Init(CDemultiplexer* pDemux, CPlayer *player)
{
	if (m_bRun)
		return false;

	m_player = player;

	AVStream* pStream = pDemux->GetFormatContext()->streams[pDemux->GetAudioIndex()];
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

	m_rate = m_pCodecCtx->sample_rate;

	m_srcFrame = av_frame_alloc();

	return true;
}

bool CAudioDecoder::SetSwr(int64_t chLayout, AVSampleFormat sampleFmt, int sampleRate)
{
	if (m_bRun)
		return false;

	m_swrChLayout = m_pCodecCtx->channel_layout;
	m_swrSampleFmt = AV_SAMPLE_FMT_S16;
	m_swrSampleRate = m_pCodecCtx->sample_rate;

	m_swrCtx = swr_alloc_set_opts(nullptr, m_swrChLayout, m_swrSampleFmt, m_swrSampleRate,
		m_pCodecCtx->channel_layout, m_pCodecCtx->sample_fmt, m_pCodecCtx->sample_rate, 0, nullptr);
	if (!m_swrCtx)
		return false;

	if (0 > swr_init(m_swrCtx))
		return false;

	m_swrNbSamples = av_rescale_rnd(swr_get_delay(m_swrCtx, m_pCodecCtx->sample_rate) + m_pCodecCtx->frame_size,
		m_swrSampleRate, m_pCodecCtx->sample_rate, AV_ROUND_INF);

	m_nChannels = av_get_channel_layout_nb_channels(m_swrChLayout);

	return true;
}

bool CAudioDecoder::Start()
{
	if (m_bRun)
		return false;

	m_bRun = true;

	m_audioThread = std::thread(&CAudioDecoder::DecodeAudioThread, this);

	return true;
}

bool CAudioDecoder::SendPacket(AVPacket* pkt)
{
	AVPacket* packet = av_packet_clone(pkt);
	m_queueAudioPkt.MaxSizePush(packet, &m_bRun);

	return true;
}

void CAudioDecoder::Close()
{
	m_bRun = false;

	if (m_audioThread.joinable())
		m_audioThread.join();

	if (m_srcFrame)
	{
		av_frame_free(&m_srcFrame);
		m_srcFrame = nullptr;
	}

	if (m_pCodecCtx)
	{
		avcodec_close(m_pCodecCtx);
		avcodec_free_context(&m_pCodecCtx);
		m_pCodecCtx = nullptr;
		m_pCodec = nullptr;
	}

	if (m_swrCtx)
	{
		swr_close(m_swrCtx);
		swr_free(&m_swrCtx);
		m_swrCtx = nullptr;
	}

	AVPacket *pkt = nullptr;
	while (m_queueAudioPkt.TryPop(pkt))
	{
		if (pkt)
			av_packet_free(&pkt);
	}
}

double CAudioDecoder::GetTimebase()
{
	return m_timebase;
}

double CAudioDecoder::GetRate()
{
	return m_rate;
}

int CAudioDecoder::GetSampleRate()
{
	return m_swrSampleRate;
}

int CAudioDecoder::GetNbSamples()
{
	return m_swrNbSamples;
}

int CAudioDecoder::GetChannels()
{
	return m_nChannels;
}

AVSampleFormat CAudioDecoder::GetSampleFmt()
{
	return m_swrSampleFmt;
}

void CAudioDecoder::DecodeAudioThread()
{
	int iRet = 0;
	AVPacket* pkt = nullptr;
	while (m_bRun)
	{
		if (m_queueAudioPkt.TryPop(pkt))
		{
			if (pkt == nullptr)
				m_bRun = false;
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
							uint8_t* pDstBuffer = nullptr;
							int dstBufSize = 0;

							if (0 > av_samples_alloc(&pDstBuffer, &dstBufSize,
								av_get_channel_layout_nb_channels(m_swrChLayout), m_swrNbSamples, m_swrSampleFmt, 1))
								m_bRun = false;
							else
							{
								iRet = swr_convert(m_swrCtx, &pDstBuffer, m_swrNbSamples,
									(const uint8_t**)m_srcFrame->data, m_srcFrame->nb_samples);

								m_player->AudioEvent(pDstBuffer, dstBufSize, m_srcFrame->best_effort_timestamp, this);;

								av_free(pDstBuffer);
							}
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
