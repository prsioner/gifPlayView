package com.prsioner.gifplayview;

import android.content.Context;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * @ClassName: FileUtils
 * @Author: qinglin
 * @CreateDate: 2021/2/15 11:15
 * @Description: 文件处理
 * 尝试将assets 内gif 文件拷贝到外部存储路径：sdcard
 * getExternalCacheDir().getAbsolutePath() = /storage/emulated/0/Android/data/packname/cache
 *
 */
public class FileUtils {

    private final String TAG = FileUtils.class.getSimpleName();

    private final Context context;

    public FileUtils(Context context){
        this.context = context;
    }
    /**
     * 将测试asset文件写入缓存
     */
    public String copyAssetAndWrite(String fileName){

        try {
            File cacheDir = new File(context.getExternalCacheDir().getAbsolutePath()+"/gif");
            if (!cacheDir.exists()){
                cacheDir.mkdirs();
            }
            File outFile =new File(cacheDir,fileName);
            if (!outFile.exists()){
                Log.e(TAG,"---!outFile.exists():");
                boolean res=outFile.createNewFile();
                if (!res){
                    return null;
                }
            }else {
                Log.d(TAG,"outFile.exists():");

                //gifFilePath = outFile.getAbsolutePath();
                if (outFile.length()>10){//表示已经写入一次
                    Log.d(TAG,"---gif缓存路径:"+outFile.getAbsolutePath());
                    return outFile.getAbsolutePath();
                }
            }
            InputStream is=context.getAssets().open(fileName);
            FileOutputStream fos = new FileOutputStream(outFile);
            byte[] buffer = new byte[1024];
            int byteCount;
            while ((byteCount = is.read(buffer)) != -1) {
                fos.write(buffer, 0, byteCount);
            }
            fos.flush();
            is.close();
            fos.close();
            Log.d(TAG,"gif copy completed1");

            return outFile.getAbsolutePath();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }



}
