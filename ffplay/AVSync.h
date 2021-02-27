#pragma once
#include "Common.h"
#include <string>

class AVSync
{
public:
	AVSync()
	{
		audioClock = 0.0f;
		lastVideoPts = 0.0f;
		videoShowStartTime = 0.0f;
	}

	~AVSync() {}

	void SetAudioClock(double pts)
	{
		audioClock = pts;
	}

	void SetVideoShowTime()
	{
		videoShowStartTime = av_gettime_relative() / AV_TIME_BASE * 1.0f;
	}

	int64_t CalcDelay(double pts)
	{
		int64_t i64Delay = 0;
		double elapsedTime = 0.0f;

		if (videoShowStartTime == 0.0f)
			SetVideoShowTime();

		double diff = pts - audioClock;
		double delay = pts - lastVideoPts;
		int series = std::to_string(static_cast<int64_t>(diff*AV_TIME_BASE)).size();
	
		lastVideoPts = pts;

		if (diff> AVSYNC_DYNAMIC_THRESHOLD)
			diff = diff * (std::pow(1.0 + AVSYNC_DYNAMIC_COEFFICIENT, series) - 1.0);
		else if (diff < - AVSYNC_DYNAMIC_THRESHOLD)
			diff = diff * (std::pow(1.0 + AVSYNC_DYNAMIC_COEFFICIENT, series) - 1.0);

		if (delay > 0.0f && (delay + diff) > 0.0f)
		{
			delay += diff;
			elapsedTime = av_gettime_relative() / AV_TIME_BASE * 1.0 - videoShowStartTime;
			i64Delay = static_cast<int64_t>((delay - elapsedTime) * AV_TIME_BASE);
		}
		else
			i64Delay = AVSYNC_SKIP_FRAME;

		return i64Delay;
	}

private:
	volatile double audioClock;			// 音频时钟，主时钟
	volatile double lastVideoPts;		// 上一帧的视频PTS
	volatile double videoShowStartTime; // 视频帧显示周期的起始时间
};