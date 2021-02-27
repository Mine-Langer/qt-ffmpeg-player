#include "CVideoDecoder.h"

CVideoDecoder::CVideoDecoder()
{
	m_avCodecCtx = avcodec_alloc_context3(nullptr);
}

CVideoDecoder::~CVideoDecoder()
{
	avcodec_free_context(&m_avCodecCtx);
}


bool CVideoDecoder::Init(AVFormatContext* pFormatCtx, int mediaIndex)
{
	AVStream* pStream = pFormatCtx->streams[mediaIndex];

	avcodec_parameters_to_context(m_avCodecCtx, pStream->codecpar);

	m_avCodec = avcodec_find_decoder(m_avCodecCtx->codec_id);
	if (m_avCodec == nullptr)
		return false;

	if (0 > avcodec_open2(m_avCodecCtx, m_avCodec, nullptr))
		return false;

	m_srcFrame = av_frame_alloc();
	m_dstFrame = av_frame_alloc();

	return true;
}

bool CVideoDecoder::SendPacket(AVPacket* pkt)
{
	AVPacket *packet = av_packet_clone(pkt);
	m_queueData.MaxSizePush(packet, &m_bRun);
	return true;
}

bool CVideoDecoder::SetParam(int width/*=-1*/, int height/*=-1*/, AVPixelFormat fmt/*=AV_PIX_FMT_NONE*/, int scaleFlag/*=SWS_FAST_BILINEAR*/)
{
	if (m_bRun) 
		return false;

	AVPixelFormat swsFmt = fmt != AV_PIX_FMT_NONE ? fmt : m_avCodecCtx->pix_fmt;
	int swsWidth = width != -1 ? width : m_avCodecCtx->width;
	int swsHeight = height != -1 ? height : m_avCodecCtx->height;

	m_swsCtx = sws_getContext(m_avCodecCtx->width, m_avCodecCtx->height, m_avCodecCtx->pix_fmt,
		swsWidth, swsHeight, swsFmt, scaleFlag, nullptr, nullptr, nullptr);
	if (m_swsCtx == nullptr)
		return false;

	m_dstBuffer = (uint8_t*)av_malloc(av_image_get_buffer_size(swsFmt, swsWidth, swsHeight, 1));
	av_image_fill_arrays(m_dstFrame->data, m_dstFrame->linesize, m_dstBuffer, swsFmt, swsWidth, swsHeight, 1);
	
	return true;
}

bool CVideoDecoder::Start()
{
	if (m_bRun)
		return false;

	m_bRun = true;

	m_decodeThread = std::thread(&CVideoDecoder::DecodeThreadFunc, this);

	return true;
}

void CVideoDecoder::DecodeThreadFunc()
{

}
