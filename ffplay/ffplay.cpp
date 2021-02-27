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
	szFilter.append(tr("Common media formats(*.avi;*.wmv;*.wmp;*.wm;*.asf;*.rm;*.ram;*.rmvb;*.ra;*.mpg;*.mpeg;*.mpe;*.m1v;*.m2v;*.mpv2);;"));
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

	m_player.SetWndConfig((const void*)ui.labelVideoView->winId(), ui.labelVideoView->width(), ui.labelVideoView->height());

	if (!m_player.Start(filename))
		return;
	
	ui.btnOpenFile->setVisible(false);	

	/*if (!OpenFile(filename))
		return;

	m_bRun = true;
	m_packet = av_packet_alloc();
	SDL_CreateThread(WaitSdlThread, nullptr, this);

	m_playThread = std::thread(&ffplay::PlayThread, this);*/
}
