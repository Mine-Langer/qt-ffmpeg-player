#pragma once

extern "C" {
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/file.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
}

#include <thread>
#include <string>

#pragma comment (lib, "SDL2.lib")
#pragma comment (lib, "SDL2main.lib")

#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "avdevice.lib")
#pragma comment (lib, "avfilter.lib")
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "postproc.lib")
#pragma comment (lib, "swresample.lib")
#pragma comment (lib, "swscale.lib")

#define INT_INDEX_NONE		-1
#define UINT_INDEX_NODE		0xFFFFFFFF
#define FRAME_MAX_CHANNEL	8
#define AVSYNC_DYNAMIC_COEFFICIENT 0.0160119 //动态帧率算法的系数 解方程(1+x)^6=1.1 即在相差时间为6为数的时候
//控制帧率的延时会在标准延时下增加或减少相差时间的(1.1-1)倍	

#define AVSYNC_DYNAMIC_THRESHOLD 0.003		//音视频同步动态帧率进行干预的二者当前时间差的阈值

#define AVSYNC_SKIP_FRAME	-0x1001