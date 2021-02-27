#include "ffplay.h"
#include <QFileDialog>
#include <QTextCodec>

ffplay::ffplay(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	RescaleLayout(this);

	ui.labelVideoView->setFrameShape(QFrame::Box);
	ui.labelVideoView->setFrameShadow(QFrame::Raised);
	ui.labelVideoView->setLineWidth(5);

	connect(ui.btnOpenFile, SIGNAL(clicked()), this, SLOT(OnBtnOpenFileClicked()));

	InitPlay();
}

bool ffplay::InitPlay()
{
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	m_pWindow = SDL_CreateWindowFrom((void*)ui.labelVideoView->winId());
	if (m_pWindow == nullptr)
		return false;

	m_pRender = SDL_CreateRenderer(m_pWindow, -1, 0);
	if (m_pRender == nullptr)
		return false;

	m_pTexture = SDL_CreateTexture(m_pRender, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 
		ui.labelVideoView->width(), ui.labelVideoView->height());
	if (m_pTexture == nullptr)
		return false;

	return true;
}

bool ffplay::OpenFile(const char* szInput)
{
	m_pFormatCtx = avformat_alloc_context();
	if (0 != avformat_open_input(&m_pFormatCtx, szInput, nullptr, nullptr))
		return false;
	
	if (0 > avformat_find_stream_info(m_pFormatCtx, nullptr))
		return false;

	for (int i=0; i<m_pFormatCtx->nb_streams; i++)
	{
		if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			m_videoIndex = i;
		else if (m_pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			m_audioIndex = i;
	}
	
	m_pCodecCtx = avcodec_alloc_context3(nullptr);
	if (m_pCodecCtx == nullptr)
		return false;

	avcodec_parameters_to_context(m_pCodecCtx, m_pFormatCtx->streams[m_videoIndex]->codecpar);
	m_pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
	if (m_pCodec == nullptr)
		return false;

	if (0 > avcodec_open2(m_pCodecCtx, m_pCodec, nullptr))
		return false;

	m_pSrcFrame = av_frame_alloc();
	m_pDstFrame = av_frame_alloc();

	m_pSwsContext = sws_getContext(m_pCodecCtx->width, m_pCodecCtx->height, m_pCodecCtx->pix_fmt,
		ui.labelVideoView->width(), ui.labelVideoView->height(), m_pCodecCtx->pix_fmt, 
		SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
	if (m_pSwsContext == nullptr)
		return false;

	m_pDstBuffer = (uint8_t*)av_malloc(av_image_get_buffer_size(m_pCodecCtx->pix_fmt, 
		ui.labelVideoView->width(), ui.labelVideoView->height(), 1));

	av_image_fill_arrays(m_pDstFrame->data, m_pDstFrame->linesize, m_pDstBuffer,
		m_pCodecCtx->pix_fmt, ui.labelVideoView->width(), ui.labelVideoView->height(), 1);

	return true;
}

int ffplay::WaitSdlThread(void *param)
{
	ffplay* pThis = (ffplay*)param;
	SDL_Event evt;
	while (pThis->m_bRun)
	{
		memset(&evt, 0, sizeof(SDL_Event));
		SDL_WaitEventTimeout(&evt, 50);

		if (evt.type == SDL_QUIT)
			pThis->m_bRun = false;
	}
	return 0;
}

void ffplay::PlayThread()
{
	while (m_bRun)
	{
		int iRet = av_read_frame(m_pFormatCtx, m_packet);
		if (0 == iRet)
		{
			if (m_packet->stream_index == m_videoIndex)
			{
				if (0 > avcodec_send_packet(m_pCodecCtx, m_packet))
					m_bRun = false;
				else
				{
					while (iRet = avcodec_receive_frame(m_pCodecCtx, m_pSrcFrame), m_bRun)
					{
						if (iRet == AVERROR(EAGAIN) || iRet == AVERROR_EOF)
							break;
						else if (iRet < 0)
							m_bRun = false;
						else if (iRet == 0)
						{
							sws_scale(m_pSwsContext, m_pSrcFrame->data, m_pSrcFrame->linesize, 0,
								m_pCodecCtx->height, m_pDstFrame->data, m_pDstFrame->linesize);

							m_pDstFrame->pts = m_pSrcFrame->pts;
							m_pDstFrame->best_effort_timestamp = m_pSrcFrame->best_effort_timestamp;

							SDL_UpdateTexture(m_pTexture, nullptr, m_pDstFrame->data[0], m_pDstFrame->linesize[0]);
							SDL_RenderClear(m_pRender);

							SDL_RenderCopy(m_pRender, m_pTexture, nullptr, nullptr);
							SDL_RenderPresent(m_pRender);
							SDL_Delay(40);
						}

					}
				}
				av_packet_unref(m_packet);
			}
		}
		else if (iRet == AVERROR_EOF)
			m_bRun = false;
	}
}

void ffplay::resizeEvent(QResizeEvent *event)
{
	RescaleLayout(this);
}

void ffplay::RescaleLayout(QWidget* pWndParent)
{
	if (pWndParent == nullptr)
		return;

	int cx, cy;	
	cx = (pWndParent->width() - ui.btnOpenFile->width()) / 2;
	cy = (pWndParent->height() - ui.btnOpenFile->height()) / 2;
	
	ui.btnOpenFile->move(cx, cy);

	cx = 0;
	cy = pWndParent->height() - ui.ControlStatusBar->height();
	ui.ControlStatusBar->move(cx, cy);
	ui.ControlStatusBar->resize(pWndParent->width(), ui.ControlStatusBar->height());

	cx = pWndParent->width();
	cy = pWndParent->height() - ui.ControlStatusBar->height() - 8;
	ui.labelVideoView->resize(cx, cy);
}

void ffplay::OnBtnOpenFileClicked()
{
	QString szFilter;
	szFilter.append(tr("Common media formats|*.avi;*.wmv;*.wmp;*.wm;*.asf;*.rm;*.ram;*.rmvb;*.ra;*.mpg;*.mpeg;*.mpe;*.m1v;*.m2v;*.mpv2;;"));
	szFilter.append(tr("*.mp2v;*.dat;*.mp4;*.m4v;*.m4p;*.vob;*.ac3;*.dts;*.mov;*.qt;*.mr;*.3gp;*.3gpp;*.3g2;*.3gp2;*.swf;*.ogg;*.wma;*.wav;;"));
	szFilter.append(tr(".mid;*.midi;*.mpa;*.mp2;*.mp3;*.m1a;*.m2a;*.m4a;*.aac;*.mkv;*.ogm;*.m4b;*.tp;*.ts;*.tpr;*.pva;*.pss;*.wv;*.m2ts;*.evo;;"));
	szFilter.append(tr("*.rpm;*.realpix;*.rt;*.smi;*.smil;*.scm;*.aif;*.aiff;*.aifc;*.amr;*.amv;*.au;*.acc;*.dsa;*.dsm;*.dsv;*.dss;*.pmp;*.smk;*.flic;;"));
	szFilter.append(tr("Windows Media Video(*.avi;*wmv;*wmp;*wm;*asf)|*.avi;*.wmv;*.wmp;*.wm;*.asf;;"));
	szFilter.append(tr("Windows Media Audio(*.wma;*wav;*aif;*aifc;*aiff;*mid;*midi;*rmi)|*.wma;*.wav;*.aif;*.aifc;*.aiff;*.mid;*.midi;*.rmi;;"));
	szFilter.append(tr("Real(*.rm;*ram;*rmvb;*rpm;*ra;*rt;*rp;*smi;*smil;*.scm)|*.rm;*.ram;*.rmvb;*.rpm;*.ra;*.rt;*.rp;*.smi;*.smil;*.scm;;"));
	szFilter.append(tr("MPEG Video(*.mpg;*mpeg;*mpe;*m1v;*m2v;*mpv2;*mp2v;*dat;*mp4;*m4v;*m4p;*m4b;*ts;*tp;*tpr;*pva;*pss;*.wv;);;"));
	szFilter.append(tr("*.mpg;*.mpeg;*.mpe;*.m1v;*.m2v;*.mpv2;*.mp2v;*.dat;*.mp4;*.m4v;*.m4p;*.m4b;*.ts;*.tp;*.tpr;*.pva;*.pss;*.wv;;;"));
	szFilter.append(tr("MPEG Audio(*.mpa;*mp2;*m1a;*m2a;*m4a;*aac;*.m2ts;*.evo)|*.mpa;*.mp2;*.m1a;*.m2a;*.m4a;*.aac;*.m2ts;*.evo;;"));
	szFilter.append(tr("DVD(*.vob;*ifo;*ac3;*dts)|*.vob;*.ifo;*.ac3;*.dts|MP3(*.mp3)|*.mp3|CD Tracks(*.cda)|*.cda;;"));
	szFilter.append(tr("Quicktime(*.mov;*qt;*mr;*3gp;*3gpp;*3g2;*3gp2)|*.mov;*.qt;*.mr;*.3gp;*.3gpp;*.3g2;*.3gp2;;"));
	szFilter.append(tr("Flash Files(*.flv;*swf;*.f4v)|*.flv;*.swf;*.f4v|Playlist(*.smpl;*.asx;*m3u;*pls;*wvx;*wax;*wmx;*mpcpl)|*.smpl;*.asx;*.m3u;*.pls;*.wvx;*.wax;*.wmx;*.mpcpl;;"));
	szFilter.append(tr("Others(*.ivf;*au;*snd;*ogm;*ogg;*fli;*flc;*flic;*d2v;*mkv;*pmp;*mka;*smk;*bik;*ratdvd;*roq;*drc;*dsm;*dsv;*dsa;*dss;*mpc;*divx;*vp6;*.ape;*.flac;*.tta;*.csf);;"));
	szFilter.append(tr("|*.ivf;*.au;*.snd;*.ogm;*.ogg;*.fli;*.flc;*.flic;*.d2v;*.mkv;*.pmp;*.mka;*.smk;*.bik;*.ratdvd;*.roq;*.drc;*.dsm;*.dsv;*.dsa;*.dss;*.mpc;*.divx;*.vp6;*.ape;*.amr;*.flac;*.tta;*.csf;;"));
	szFilter.append(tr("All Files(*.*)|*.*||"));

	QString szFilePath = QFileDialog::getOpenFileName(nullptr, nullptr, nullptr, szFilter);
	if (szFilePath.isEmpty())
		return;
	QString szFileName = szFilePath.right(szFilePath.length() - szFilePath.lastIndexOf(tr("/")) - 1);
	setWindowTitle(szFileName);

	QByteArray ba = szFilePath.toLocal8Bit();
	const char* filename = ba.data();

	if (m_player.Start(filename)) {
		ui.btnOpenFile->setVisible(false);
	}

	/*if (!OpenFile(filename))
		return;

	m_bRun = true;
	m_packet = av_packet_alloc();
	SDL_CreateThread(WaitSdlThread, nullptr, this);

	m_playThread = std::thread(&ffplay::PlayThread, this);*/
}
