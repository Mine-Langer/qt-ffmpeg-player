#pragma once
#include "Common.h"

class AVSync
{
public:
	AVSync() :audio_clock(0.0f), last_video_pts(0.0f), video_show_start_time(0.0f) {}
	~AVSync() {}

	// 设定当前音频时钟
	void SetAudioClock(double _pts)
	{
		audio_clock = _pts;
	}

	// 获取本周期图像显示的起始时间
	void SetVideoShowTime()
	{
		video_show_start_time = av_gettime_relative() / AV_TIME_BASE * 1.0;
	}

	// 计算代表帧率控制的延迟(微秒)
	int64_t CalDelay(double _pts)
	{
		int64_t i64_delay = 0;
		double elapsed_time = 0.0;

		if (video_show_start_time == 0.0)
			SetVideoShowTime(); 

		double diff = _pts - audio_clock;
		double delay = _pts - last_video_pts;
		int series = std::to_string(static_cast<int64_t>(diff * AV_TIME_BASE)).size();

		last_video_pts = _pts;

		if (diff > AVSYNC_DYNAMIC_THRESHOLD)
		{
			diff = diff * (std::pow(1.0 + AVSYNC_DYNAMIC_COEFFICIENT, series) - 1.0);
		}
		else if (diff < -AVSYNC_DYNAMIC_THRESHOLD)
		{
			diff = diff * (std::pow(1.0 + AVSYNC_DYNAMIC_COEFFICIENT, series) - 1.0);
		}

		if ((delay > 0.0) && ((delay + diff) > 0.0))
		{
			delay += diff;

			elapsed_time = av_gettime_relative() / AV_TIME_BASE * 1.0 - video_show_start_time;

			i64_delay = static_cast<int64_t>((delay - elapsed_time) * AV_TIME_BASE);
		}
		else
		{
			i64_delay = AVSYNC_SKIP_FRAME;
		}

		printf("%lf\t%lf\n", delay, diff);
		return i64_delay;
	}

private:
	volatile double audio_clock;			
	volatile double last_video_pts;	
	volatile double video_show_start_time;
};

