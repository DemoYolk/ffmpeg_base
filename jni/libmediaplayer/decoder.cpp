#include <android/log.h>
#include "decoder.h"

#define TAG "FFMpegIDecoder"

IDecoder::IDecoder(AVStream* stream)
{
	mQueue = new PacketQueue();
	mStream = stream;
}

IDecoder::~IDecoder()
{
	if(mRunning)
    {
        stop();
    }
	free(mQueue);
	avcodec_close(mStream->codec); //释放流解码器
}

void IDecoder::enqueue(AVPacket* packet)
{
	mQueue->put(packet);
}
/**
 * 得到待解码包个数
 */
int IDecoder::packets()
{
	return mQueue->size();
}

void IDecoder::stop()
{
    mQueue->abort();
    __android_log_print(ANDROID_LOG_INFO, TAG, "waiting on end of decoder thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel IDecoder: %i", ret);
        return;
    }
}
/**
 * 实现基类的虚函数，通过自生对象或派生类对象调用该方法，会从这儿进入
 */
void IDecoder::handleRun(void* ptr)
{
	if(!prepare())//找到对应的prepare派生类实现
    {
		__android_log_print(ANDROID_LOG_INFO, TAG, "Couldn't prepare decoder");
        return;
    }
	decode(ptr);//找到对应的decode派生类实现
}
/**
 * 因为这儿是虚函数，所以会被其派生类的实现所动态绑定方法体，这个方法理论上是不会被调用的。
 */
bool IDecoder::prepare()
{
    return false;
}
/**
 * 因为这儿是虚函数，所以会被其派生类的实现所动态绑定方法体，这个方法理论上是不会被调用的。
 */
bool IDecoder::process(AVPacket *packet)
{
	return false;
}
/**
 * 因为这儿是虚函数，所以会被其派生类的实现所动态绑定方法体，这个方法理论上是不会被调用的。
 */
bool IDecoder::decode(void* ptr)
{
    return false;
}
