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

	static int WaitSdlThread(void *param);


protected:
	void resizeEvent(QResizeEvent *event);


	void RescaleLayout(QWidget* pWndParent); // 调整布局


private slots:
	void OnBtnOpenFileClicked();

private:
    Ui::ffplayClass ui;
	CPlayer m_player;


	int m_videoIndex = -1;
	int m_audioIndex = -1;

	bool m_bRun = false;


};
