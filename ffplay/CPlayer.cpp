#include "Common.h"
#include "CPlayer.h"

void CPlayer::sdlAudioCallback(void *userdata, Uint8 * stream, int len)
{
	CPlayer *pThis = (CPlayer*)userdata;
	CFrame* pFrame = nullptr;
	if (pThis->m_queueAudioData.TryPop(pFrame))
	{
		int writeLen = len < pFrame->dataLen[0] ? len : pFrame->dataLen[0];
		SDL_memset(stream, 0, len);
		SDL_MixAudio(stream, pFrame->data[0], writeLen, SDL_MIX_MAXVOLUME);
		pThis->m_sync.SetAudioClock(pFrame->dts);
		delete pFrame;
	}
	else
	{
		SDL_memset(stream, 0, len);
	}
}

bool CPlayer::Start(const char* szInput)
{
	if (m_bRun)
		return false;

	if (initFFmpeg(szInput) == false)
		return false;

	if (initSdl2() == false)
		return false;

	m_bRun = true;

	m_playThread = std::thread(&CPlayer::playThreadFunc, this);

	//sdl2EventFunc();

	return true;
}

void CPlayer::Stop()
{
	m_bRun = false;

	if (m_playThread.joinable())
		m_playThread.join();

	m_demux.Close();
	m_videoDecoder.Close();
	m_audioDecoder.Close();

	if (m_window)
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;
	}

	if (m_texture)
	{
		SDL_DestroyTexture(m_texture);
		m_texture = nullptr;
	}

	if (m_render)
	{
		SDL_DestroyRenderer(m_render);
		m_render = nullptr;
	}
}

void CPlayer::SetWndConfig(const void* pwnd, int width, int height)
{
	m_pWindow = pwnd;
	m_nWidth = width;
	m_nHeight = height;
}

void CPlayer::VideoEvent(AVFrame* img, CVideoDecoder* pDecoder)
{
	int width = pDecoder->GetDstWidth();
	int height = pDecoder->GetDstHeight();

	CFrame* pFrame = new CFrame();
	pFrame->CopyYUV(img, width, height);
	pFrame->pts = img->best_effort_timestamp;
	pFrame->dts = img->pts * pDecoder->GetTimebase();
	pFrame->duration = 1.0 / pDecoder->GetRate();

	m_queueVideoData.MaxSizePush(pFrame, &m_bRun);
}

void CPlayer::AudioEvent(uint8_t* buf, int len, int64_t pts, CAudioDecoder* pDecoder)
{
	CFrame* pFrame = new CFrame;
	pFrame->CopyPCM(buf, len);

	pFrame->pts = pts;
	pFrame->dts = pts * pDecoder->GetTimebase();
	pFrame->duration = 1.0 / pDecoder->GetRate();

	m_queueAudioData.MaxSizePush(pFrame, &m_bRun);
}

CPlayer::CPlayer()
{
	memset(&m_audioSpec, 0, sizeof(SDL_AudioSpec));
	memset(&m_rect, 0, sizeof(SDL_Rect));
	m_bRun = false;
	m_window = nullptr;
	m_render = nullptr;
	m_texture = nullptr;
}

CPlayer::~CPlayer()
{
	Stop();
}

bool CPlayer::initSdl2()
{
	int width = m_videoDecoder.GetDstWidth(), height = m_videoDecoder.GetDstHeight();
	AVPixelFormat fmt = m_videoDecoder.GetDstFormat();

	//int sampleRate = -1, nbSample = -1, channel = -1;
	AVSampleFormat sampleFmt = AV_SAMPLE_FMT_NONE;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

	m_window = SDL_CreateWindowFrom(m_pWindow);
	if (m_window == nullptr)
		return false;
	
	m_render = SDL_CreateRenderer(m_window, -1, 0);
	if (m_render == nullptr)
		return false;

	m_rect = { 0,0,width,height };

	m_texture = SDL_CreateTexture(m_render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (m_texture == nullptr)
		return false;

	m_audioSpec.freq = m_audioDecoder.GetSampleRate();
	m_audioSpec.format = AUDIO_S16SYS;
	m_audioSpec.channels = m_audioDecoder.GetChannels();
	m_audioSpec.silence = 0;
	m_audioSpec.samples = m_audioDecoder.GetNbSamples();
	m_audioSpec.userdata = this;
	m_audioSpec.callback = sdlAudioCallback;
	if (0 > SDL_OpenAudio(&m_audioSpec, nullptr))
	{
		printf("SDL_OpenAudio: %s \n", SDL_GetError());
		return false;
	}

	SDL_PauseAudio(0);
	return true;
}

bool CPlayer::initFFmpeg(const char* szInput)
{
	if (m_bRun)
		return false;

	m_demux.Open(szInput);
	m_demux.Start(&m_videoDecoder, &m_audioDecoder);

	m_videoDecoder.Init(&m_demux, this);
	m_videoDecoder.SetConfig(m_nWidth, m_nHeight);
	m_videoDecoder.Start();

	m_audioDecoder.Init(&m_demux, this);
	m_audioDecoder.SetSwr(0, AV_SAMPLE_FMT_NONE, 0); // 参数缺省
	m_audioDecoder.Start();

	return true;
}

void CPlayer::sdl2EventFunc()
{
	SDL_Event evt;
	while (m_bRun)
	{
		memset(&evt, 0, sizeof(SDL_Event));
		SDL_WaitEventTimeout(&evt, 50);

		if (evt.type == SDL_QUIT)
			m_bRun = false;
	}
}

void CPlayer::playThreadFunc()
{
	CFrame* pFrame = nullptr;
	int64_t idelay = 0;

	while (m_bRun)
	{
		if (m_queueVideoData.TryPop(pFrame))
		{
			SDL_UpdateYUVTexture(m_texture, nullptr, pFrame->data[0], pFrame->linesize[0],
				pFrame->data[1], pFrame->linesize[1], pFrame->data[2], pFrame->linesize[2]);
			SDL_RenderClear(m_render);
			SDL_RenderCopy(m_render, m_texture, nullptr, &m_rect);

			idelay = m_sync.CalDelay(pFrame->dts);
			if (idelay > 0)
				std::this_thread::sleep_for(std::chrono::microseconds(idelay));

			SDL_RenderPresent(m_render);

			m_sync.SetVideoShowTime();
			delete pFrame;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}
