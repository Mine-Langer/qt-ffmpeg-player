#pragma once
#include "CVideoDecoder.h"
#include "CAudioDecoder.h"
#include "CDemultiplexer.h"
#include "CFrame.h"
#include "AVSync.h"

class CPlayer
{
public:
	bool Start(const char* szInput);

	void Stop();

	void SetWndConfig(const void* pwnd, int width, int height);

	void VideoEvent(AVFrame* img, CVideoDecoder* pDecoder);
	void AudioEvent(uint8_t* buf, int len, int64_t pts, CAudioDecoder* pDecoder);

public:
	CPlayer();
	~CPlayer();

private:
	bool initSdl2();

	bool initFFmpeg(const char* szInput);

	void sdl2EventFunc();

	static void sdlAudioCallback(void *userdata, Uint8 * stream, int len);

	void playThreadFunc();

private:
	bool m_bRun;

	SDL_AudioSpec m_audioSpec;
	SDL_Rect m_rect;
	SDL_Window *m_window;
	SDL_Renderer *m_render;
	SDL_Texture *m_texture;

	CDemultiplexer m_demux;
	CVideoDecoder m_videoDecoder;
	CAudioDecoder m_audioDecoder;
	AVSync	m_sync;

	CQueue<CFrame*> m_queueVideoData;
	CQueue<CFrame*> m_queueAudioData;

	std::thread m_playThread;

	int m_nWidth;
	int m_nHeight;
	const void* m_pWindow;
};

