#include <android/log.h>
#include "decoder_audio.h"

#define TAG "FFMpegAudioDecoder"

DecoderAudio::DecoderAudio(AVStream* stream) : IDecoder(stream)
{
}

DecoderAudio::~DecoderAudio()
{
}
/**
 * 准备
 */
bool DecoderAudio::prepare()
{
    mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;//初始化为192000bit
    mSamples = (int16_t *) av_malloc(mSamplesSize); //动态分配192000bit空间给一个指向2B的指针（应该是指针移动一次是两个字节）
    if(mSamples == NULL) {
    	return false;
    }
    return true;
}
/**
 * 处理包中的数据
 */
bool DecoderAudio::process(AVPacket *packet)
{
    int size = mSamplesSize;
    //取出192000b的大小数据，
    int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, packet);

    //call handler for posting buffer to os audio driver
    onDecode(mSamples, size); //这儿会调用这个指针被赋值的函数地址

    return true;
}
/**
 * 解码音频
 */
bool DecoderAudio::decode(void* ptr)
{
    AVPacket        pPacket;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

    while(mRunning)
    {
        if(mQueue->get(&pPacket, true) < 0)//通过 互斥锁 去队列里 取数据，如果队列被取消了认为解码错误
        {
            mRunning = false;
            return false;
        }
        if(!process(&pPacket))
        {
            mRunning = false;
            return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket); //这儿在取出192kb的数据就释放了，可能会造成数据丢失啊
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio ended");

    // Free audio samples buffer
    av_free(mSamples);
    return true;
}
