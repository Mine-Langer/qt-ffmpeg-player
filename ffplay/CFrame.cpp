#include "CFrame.h"

CFrame::CFrame()
{
	dts = 0.0f;
	duration = 0.0f;
	pts = 0.0f;

	dataChannel = 0;
	memset(linesize, 0, sizeof(int)*FRAME_MAX_CHANNEL);
	memset(dataLen, 0, sizeof(int)*FRAME_MAX_CHANNEL);
	memset(data, 0, sizeof(uint8_t*)*FRAME_MAX_CHANNEL);
}

CFrame::~CFrame()
{
	Clear();
}

void CFrame::Clear()
{
	for (int i=0; i<dataChannel; i++)
	{
		if (data[i] != nullptr)
		{
			delete[] data[i];
			data[i] = nullptr;
		}
	}

	dts = 0.0f;
	pts = 0.0f;
	duration = 0.0f;
	memset(linesize, 0, sizeof(int)*FRAME_MAX_CHANNEL);
	memset(dataLen, 0, sizeof(int)*FRAME_MAX_CHANNEL);
	memset(data, 0, sizeof(uint8_t*)*FRAME_MAX_CHANNEL);
}

void CFrame::CopyYUV(AVFrame *data, int w, int h)
{
	this->dataChannel = 3;
	memcpy(this->linesize, data->linesize, sizeof(int) * 3);

	this->dataLen[0] = w * h;
	this->data[0] = new uint8_t[this->dataLen[0]];
	memcpy(this->data[0], data->data[0], this->dataLen[0] * sizeof(uint8_t));

	this->dataLen[1] = w * h / 4;
	this->data[1] = new uint8_t[this->dataLen[1]];
	memcpy(this->data[1], data->data[1], this->dataLen[1] * sizeof(uint8_t));

	this->dataLen[2] = w * h / 4;
	this->data[2] = new uint8_t[this->dataLen[2]];
	memcpy(this->data[2], data->data[2], this->dataLen[2] * sizeof(uint8_t));
}

void CFrame::CopyPCM(uint8_t* buf, int len)
{
	this->dataChannel = 1;
	
	this->dataLen[0] = len;
	this->data[0] = new uint8_t[this->dataLen[0]];
	memcpy(this->data[0], buf, this->dataLen[0] * sizeof(uint8_t));
}
