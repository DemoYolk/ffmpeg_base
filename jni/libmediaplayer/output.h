#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <jni.h>

#include <android/audiotrack.h>
#include <android/surface.h>
/**
 * 输出类；用于音频和视频的输出。 从网上了解到，使用了android自带的音频流和图像的类，所以这儿只是对libjniaudio.so和libjnivideo.so的封装。
 * 这个有个问题是，现在使用的这两个库文件是在2.2的android源目录树下编译生成的，
 * 如果要在其他android版本中使用需要重新在源目录树编译（或许有其他解决办法。）。
 *
 * 所以这儿的cpp都是对android相同功能的封装。
 */
class Output
{
public:	
	static int					AudioDriver_register();
    static int					AudioDriver_set(int streamType,
												uint32_t sampleRate,
												int format,
												int channels);
    static int					AudioDriver_start();
    static int					AudioDriver_flush();
	static int					AudioDriver_stop();
    static int					AudioDriver_reload();
	static int					AudioDriver_write(void *buffer, int buffer_size);
	static int					AudioDriver_unregister();
	
	static int					VideoDriver_register(JNIEnv* env, jobject jsurface);
    static int					VideoDriver_getPixels(int width, int height, void** pixels);
    static int					VideoDriver_updateSurface();
    static int					VideoDriver_unregister();
};

#endif //FFMPEG_DECODER_H
