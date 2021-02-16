#include <jni.h>
#include <string>
#include <android/log.h>
#include "gif_lib.h"
#include <android/bitmap.h>


#define  LOG_TAG    "Prsioner"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  dispose(ext) (((ext)->Bytes[0] & 0x1c) >> 2)
#define  trans_index(ext) ((ext)->Bytes[3])
#define  transparency(ext) ((ext)->Bytes[0] & 1)

//将色值字节数据组装成一个int 类型
#define  argb(a,r,g,b) ( ((a) & 0xff) << 24 ) | ( ((b) & 0xff) << 16 ) | ( ((g) & 0xff) << 8 ) | ((r) & 0xff)


//定义一个结构体保存播放状态
struct GifBean{
    int current_frame;
    int total_frame;
    int *delays;
};

/**
 * extern "C" : 表明C++ 代码中引用的C相关函数或者库需要使用C的编译器来编译
 */

int drawFrame1(GifFileType *pType, AndroidBitmapInfo param, void *pInt);


int drawFrame(GifFileType *pType, AndroidBitmapInfo info, void *pInt, bool b);

extern "C" JNIEXPORT jstring JNICALL
Java_com_prsioner_gifplayview_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}



extern "C"
JNIEXPORT jlong JNICALL
Java_com_prsioner_gifplayview_GifHelper_loadGif(JNIEnv *env, jclass clazz, jstring path) {

    //env 用于Java 与c/c++之间的数据转化，是本线程环境 ,这里new 了内存空间，
    const  char* gifPath = env->GetStringUTFChars(path,0);

    __android_log_print(ANDROID_LOG_INFO, "loadGif", "String:%s", gifPath); //log i类型

    /**
     * gif_lib.h 中提供的gif 文件操作相关函数
     * GifFileType 是一个结构体，包含了gif文件的详细信息,这里声明为一个指针，（所以这个指针指向的地址就是java 需要的gifPointer）
     * GifWord SWidth, SHeight;  gif 的宽，高
     * ColorMapObject *SColorMap;  gif 的位图
     * int ImageCount;   gif 有多少帧
     * SavedImage *SavedImages; 指针类型，指向一个数组，里面的元素是saveImage(包含一帧的像素数据)
     * void *UserData;  类似与java 中的view.setTag("xx"),用于与用户数据绑定
     */
    int ERROR; //打开成功还是失败
    GifFileType* gifFileType = DGifOpenFileName(gifPath,&ERROR);

    //初始化缓冲区用于保存  数组SaveImages
    DGifSlurp(gifFileType);

    GifBean * gifBean = static_cast<GifBean *>(malloc(sizeof(GifBean)));
    //新malloc的内存空间内部有一些脏数据,需要清空一下
    memset(gifBean,0,sizeof(gifBean));

    gifBean->total_frame = gifFileType->ImageCount;
    gifBean->current_frame = 0;
    gifFileType->UserData = gifBean; //gif指针与我们需要的数据进行绑定

    //jlong jlong1 = 1000000;
    //回传数据给java

    //释放new 的 gifpath内存空间
    env->ReleaseStringUTFChars(path,gifPath);
    return reinterpret_cast<jlong>(gifFileType);

}extern "C"
JNIEXPORT jint JNICALL
Java_com_prsioner_gifplayview_GifHelper_getWidth(JNIEnv *env, jclass clazz, jlong gif_pointer) {
    GifFileType * gifFileType = reinterpret_cast<GifFileType *>(gif_pointer);
    return gifFileType->SWidth;
}extern "C"
JNIEXPORT jint JNICALL
Java_com_prsioner_gifplayview_GifHelper_getHeight(JNIEnv *env, jclass clazz, jlong gif_pointer) {

    GifFileType * gifFileType = reinterpret_cast<GifFileType *>(gif_pointer);
    return gifFileType->SHeight;
}
int drawFrame1(GifFileType *gifFileType, AndroidBitmapInfo bitmapInfo, void *pInt) {

    //1.取出gifFileType 内的当前帧数据
    GifBean *gifBean = static_cast<GifBean *>(gifFileType->UserData);
    SavedImage savedImage = gifFileType->SavedImages[gifBean->current_frame];

    //2.当前帧数据并不是直接存的像素，不能直接用于渲染bitmap, 需要解压gif 内的位图数据
    // 这个跟gif 图片格式原理有关，看 gif编码原理.md

    //图像数据分为两个部分: 描述 top 、right
    GifImageDesc frameInfo = savedImage.ImageDesc;
    ColorMapObject * colorMapObject = frameInfo.ColorMap;

    int *line; //记录像素二维数组行首地址
    int *px = static_cast<int *>(pInt);

    int pointPixel;
    GifByteType  gifByteType;
    GifColorType  colorType;
    for (int y = frameInfo.Top; y <frameInfo.Top+frameInfo.Height ; ++y) {

        line = px;
        for (int x = frameInfo.Left; x < frameInfo.Left+frameInfo.Width; ++x) {
        //定位到像素点
        pointPixel = (y-frameInfo.Top)*frameInfo.Width+(x-frameInfo.Left);
        //得到压缩的像素数据
        gifByteType = savedImage.RasterBits[pointPixel];
        //h还需要通过colorMap 得到真正这个像素点的色值数据
        colorType = colorMapObject->Colors[gifByteType];
        // 真正渲染java 传过来的bitmap ,给每个像素点赋值上色值数据
        line[x] = argb(255,colorType.Red,colorType.Green,colorType.Blue);

        }
        //跳到下一行 ,stride: 每一行的字节数
        px = reinterpret_cast<int *>((char *) px + bitmapInfo.stride);

    }


    GraphicsControlBlock gcb;//获取控制信息
    DGifSavedExtensionToGCB(gifFileType,gifBean->current_frame,&gcb);
    int delay=gcb.DelayTime * 10;
    LOGE("delay %d",delay);
    return delay;


}


extern "C"
JNIEXPORT jint JNICALL
Java_com_prsioner_gifplayview_GifHelper_updateFrame(JNIEnv *env, jclass clazz, jlong gif_pointer,
                                                    jobject bitmap) {

    //处理bitmap 的库 android/bitmap.h,在ndk 安装目录下面（ndk->platforms->usr->lib）
    //下的jnigraphics.so 库中，所以需要配置CmakeList find_library(jnigraphics-lib jnigraphics)
    // C/C++里面操作音频、视频、图片等都认为是数组，所以需要把bitmap转化成数组

    GifFileType * gifFileType = reinterpret_cast<GifFileType *>(gif_pointer);
    //int width = gifFileType->SWidth;
    //int height = gifFileType->SHeight;

    //1.获取bitmap的宽高
    //获取图片宽高可以通过AndroidBitmapInfo 来获取，也可以通过gifFileType指针
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env,bitmap,&info);
    int width = info.width;
    int height = info.height;
    //2.将bitmap 转化为数组
    void *pixels;
    //并且加锁当前bitmap 使得其他线程无法操作
    AndroidBitmap_lockPixels(env, bitmap, reinterpret_cast<void **>(&pixels));

    //3.开始绘制bitmap
    //int delay = drawFrame1(gifFileType, info, pixels);

    //兼容 87a 和89a gif 格式的写法
    int delay=drawFrame(gifFileType, info, pixels ,false);


    AndroidBitmap_unlockPixels(env,bitmap);//解锁
    //4.当前帧绘制完成，并解锁，就需要跳到下一帧
    GifBean *gifBean = static_cast<GifBean *>(gifFileType->UserData);
    gifBean->current_frame ++;
    if(gifBean->current_frame >=gifBean->total_frame-1){
        //循环播放
        gifBean->current_frame = 0;
    }



    return delay;
}

int drawFrame(GifFileType *gif, AndroidBitmapInfo info, void* pixels, bool force_dispose_1) {
    GifColorType *bg;

    GifColorType *color;

    SavedImage * frame;

    ExtensionBlock * ext = 0;

    GifImageDesc * frameInfo;

    ColorMapObject * colorMap;

    int *line;

    int width, height,x,y,j,loc,n,inc,p;

    void* px;

    GifBean *gifBean = static_cast<GifBean *>(gif->UserData);

    width = gif->SWidth;

    height = gif->SHeight;
    frame = &(gif->SavedImages[gifBean->current_frame]);

    frameInfo = &(frame->ImageDesc);

    if (frameInfo->ColorMap) {

        colorMap = frameInfo->ColorMap;

    } else {

        colorMap = gif->SColorMap;

    }



    bg = &colorMap->Colors[gif->SBackGroundColor];



    for (j=0; j<frame->ExtensionBlockCount; j++) {

        if (frame->ExtensionBlocks[j].Function == GRAPHICS_EXT_FUNC_CODE) {

            ext = &(frame->ExtensionBlocks[j]);

            break;

        }

    }
    // For dispose = 1, we assume its been drawn
    px = pixels;

    //根据gif编码判断是否有扩展块来区分89a 和87a 版本， 87a 没有扩展块
    if (ext && dispose(ext) == 1 && force_dispose_1 && gifBean->current_frame > 0) {
        gifBean->current_frame=gifBean->current_frame-1,
                drawFrame(gif , info, pixels,  true);
    }

    else if (ext && dispose(ext) == 2 && bg) {

        for (y=0; y<height; y++) {

            line = (int*) px;

            for (x=0; x<width; x++) {

                line[x] = argb(255, bg->Red, bg->Green, bg->Blue);

            }

            px = (int *) ((char*)px + info.stride);

        }

    } else if (ext && dispose(ext) == 3 && gifBean->current_frame > 1) {
        gifBean->current_frame=gifBean->current_frame-2,
                drawFrame(gif,  info, pixels,  true);

    }
    px = pixels;

    if (frameInfo->Interlace) {
        LOGE("frameInfo->Interlace:true");
        n = 0;

        inc = 8;

        p = 0;

        px = (int *) ((char*)px + info.stride * frameInfo->Top);

        for (y=frameInfo->Top; y<frameInfo->Top+frameInfo->Height; y++) {

            for (x=frameInfo->Left; x<frameInfo->Left+frameInfo->Width; x++) {

                loc = (y - frameInfo->Top)*frameInfo->Width + (x - frameInfo->Left);

                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {

                    continue;

                }



                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg : &colorMap->Colors[frame->RasterBits[loc]];

                if (color)

                    line[x] = argb(255, color->Red, color->Green, color->Blue);

            }

            px = (int *) ((char*)px + info.stride * inc);

            n += inc;

            if (n >= frameInfo->Height) {

                n = 0;

                switch(p) {

                    case 0:

                        px = (int *) ((char *)pixels + info.stride * (4 + frameInfo->Top));

                        inc = 8;

                        p++;

                        break;

                    case 1:

                        px = (int *) ((char *)pixels + info.stride * (2 + frameInfo->Top));

                        inc = 4;

                        p++;

                        break;

                    case 2:

                        px = (int *) ((char *)pixels + info.stride * (1 + frameInfo->Top));

                        inc = 2;

                        p++;

                }

            }

        }

    }

    else {
        LOGE("frameInfo->Interlace:false");
        px = (int *) ((char*)px + info.stride * frameInfo->Top);

        for (y=frameInfo->Top; y<frameInfo->Top+frameInfo->Height; y++) {

            line = (int*) px;

            for (x=frameInfo->Left; x<frameInfo->Left+frameInfo->Width; x++) {

                loc = (y - frameInfo->Top)*frameInfo->Width + (x - frameInfo->Left);

                if (ext && frame->RasterBits[loc] == trans_index(ext) && transparency(ext)) {

                    continue;

                }

                color = (ext && frame->RasterBits[loc] == trans_index(ext)) ? bg : &colorMap->Colors[frame->RasterBits[loc]];

                if (color)

                    line[x] = argb(255, color->Red, color->Green, color->Blue);

            }

            px = (int *) ((char*)px + info.stride);

        }
    }
    GraphicsControlBlock gcb;//获取控制信息
    DGifSavedExtensionToGCB(gif,gifBean->current_frame,&gcb);
    int delay=gcb.DelayTime * 10;
    LOGE("delay %d",delay);
    return delay;
}
