package com.media.ffmpeg.config;

public class FFMpegConfig {

	/** mpeg4的编码方式 */
	public static final String CODEC_MPEG4 = "mpeg4";
	/** 高位率 */
	public static final String BITRATE_HIGH = "1024000";// 1024Kbps
	/** 中位率 */
	public static final String BITRATE_MEDIUM = "512000";// 512Kbps
	/** 低位率 */
	public static final String BITRATE_LOW = "128000";// 128Kbps
	/** 比率 3：2 */
	public static final int[] RATIO_3_2 = new int[] { 3, 2 };
	/** 比率 4：3 */
	public static final int[] RATIO_4_3 = new int[] { 4, 3 };
	/** 分辨率 */
	public int[] resolution = new int[] { 800, 600 };
	/** 编码方式 */
	public String codec = CODEC_MPEG4;
	/** 位率 */
	public String bitrate = BITRATE_LOW;
	/** 视频比率 */
	public int[] ratio = RATIO_4_3;
	/** 音频位率 */
	public int audioRate = 44000; // 44Kbps/2（通道） = 22Kb
	/** 帧率 */
	public int frameRate = 24; // 帧
	/** 音频通道 */
	public int audioChannels = 2; // 双通道

}
