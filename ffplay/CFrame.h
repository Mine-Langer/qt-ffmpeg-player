#pragma once

class CFrame
{
public:
	CFrame();
	~CFrame();

	void Clear();

	void CopyYUV(AVFrame* data, int w, int h);

	void CopyPCM(uint8_t *buf, int len);

public:
	double dts;
	double duration;
	int64_t pts;
	int dataChannel;
	int linesize[FRAME_MAX_CHANNEL];
	int dataLen[FRAME_MAX_CHANNEL];
	uint8_t *data[FRAME_MAX_CHANNEL];
};

