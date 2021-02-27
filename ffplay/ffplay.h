#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ffplay.h"
#include "CPlayer.h"

class ffplay : public QMainWindow
{
    Q_OBJECT

public:
    ffplay(QWidget *parent = Q_NULLPTR);


private:
	bool InitPlay();
	bool OpenFile(const char* szInput);

	static int WaitSdlThread(void *param);

	void PlayThread();

protected:
	void resizeEvent(QResizeEvent *event);


	void RescaleLayout(QWidget* pWndParent); // 调整布局


private slots:
	void OnBtnOpenFileClicked();

private:
    Ui::ffplayClass ui;
	CPlayer m_player;

	SDL_Window* m_pWindow = nullptr;
	SDL_Renderer* m_pRender = nullptr;
	SDL_Texture* m_pTexture = nullptr;
	SDL_Rect m_rect;

	AVFormatContext *m_pFormatCtx = nullptr;
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
	SwsContext* m_pSwsContext = nullptr;

	AVFrame* m_pSrcFrame = nullptr;
	AVFrame* m_pDstFrame = nullptr;
	uint8_t* m_pDstBuffer = nullptr;
	AVPacket* m_packet = nullptr;

	int m_videoIndex = -1;
	int m_audioIndex = -1;

	bool m_bRun = false;

	std::thread m_playThread;
};
