package com.prsioner.gifplayview;

import android.graphics.Bitmap;

/**
 * @ClassName: GifHelper
 * @Author: qinglin
 * @CreateDate: 2021/2/14 14:53
 * @Description: gif 工具类，定义jni方法
 */
public class GifHelper {

    static {
        System.loadLibrary("native-lib");
    }

    private static GifHelper gifHelper;

    //gif的内存地址,只能用16位保存
    private long gifPointer;

    private GifHelper(String gifFilePath){
        gifPointer = loadGif(gifFilePath);
    }

    public static GifHelper getInstance(String gifFilePath){
        if(gifHelper ==null){
            synchronized (GifHelper.class){
                if(gifHelper ==null){
                    gifHelper = new GifHelper(gifFilePath);
                }
            }
        }

        return gifHelper;
    }

    /**
     * 获取gif图片宽度 （一个gif 文件的宽和高是固定的）
     * @return
     */
    public int getWidth(){
        return getWidth(gifPointer);
    }

    public int getHeight(){
        return getHeight(gifPointer);
    }

    public int updateFrame(Bitmap bitmap){
        return updateFrame(gifPointer,bitmap);
    }

    /**
     * 返回gif 内存地址
     * @param path
     * @return
     */
    public static native long loadGif(String path);

    public static native int getWidth(long gifPointer);

    public static native int getHeight(long gifPointer);

    public static native int updateFrame(long gifPointer,Bitmap bitmap);
}
