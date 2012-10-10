#ifndef FFMPEG_PACKETQUEUE_H
#define FFMPEG_PACKETQUEUE_H

#include <pthread.h>

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
	
} // end of extern C

class PacketQueue
{
public:
	PacketQueue();
	~PacketQueue();
	
	/**
	 * 清空 队列
	 */
    void flush();
    /**
     * 将AVPacket放入队列
     */
	int put(AVPacket* pkt);
	
	/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
	int get(AVPacket *pkt, bool block);
	/**
	 * 得到队列的 当前的大小
	 */
	int size();
	
	void abort();
	
private:
	AVPacketList*		mFirst; //队列中第一个 packet
	AVPacketList*		mLast; //队列中 最后一个 packet
    int					mNbPackets; // 队列中的packet个数
    int					mSize; // 队列大小
    bool				mAbortRequest; //
	pthread_mutex_t     mLock; // 互斥锁
	pthread_cond_t		mCondition; // 条件锁
};

#endif // FFMPEG_MEDIAPLAYER_H
