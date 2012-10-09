package com.media.ffmpeg.android;

import android.content.Context;
import android.view.Display;
import android.view.WindowManager;

import com.media.ffmpeg.config.FFMpegConfig;

public class FFMpegConfigAndroid extends FFMpegConfig {

	public FFMpegConfigAndroid(Context context) {
		overrideParametres(context);
	}

	/**
	 * 重写 解码后的参数
	 * 
	 * @param context
	 */
	private void overrideParametres(Context context) {
		resolution = getScreenResolution(context);
		// ratio = RATIO_3_2; // 3:2
		// audioRate = 16000; // 16Kbps
		// frameRate = 13; // 13 帧？
	}

	/**
	 * 得到屏幕分辨率
	 * 
	 * @param context
	 * @return
	 */
	private int[] getScreenResolution(Context context) {
		Display display = ((WindowManager) context
				.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
		int[] res = new int[] { display.getHeight(), display.getWidth() };
		return res;
	}

}
