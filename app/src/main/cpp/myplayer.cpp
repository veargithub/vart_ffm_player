//
// Created by vartc on 2023/12/15.
//

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include "myqueue.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
}

// Android 打印 Log
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);
// 状态码
#define SUCCESS_CODE 1
#define FAIL_CODE -1

typedef struct _Player {
    // Env
    JavaVM *javaVm;
    jobject instance;
    jobject surface;
    AVFormatContext *formatContext;// 上下文
    //视频
    int videoStreamIndex; //视频解码器的index
    AVCodecContext *videoCodecContext; //视频解码器上下文
    ANativeWindow *nativeWindow;
    ANativeWindow_Buffer windowBuffer;
    uint8_t *videoOutBuffer;
    struct SwsContext *swsContext;
    AVFrame *rgbaFrame;
    Queue  *videoQueue;
    //音频
    int audioStreamIndex;
    AVCodecContext *audioCodecContext;
    uint8_t *audioOutBuffer;
    struct SwrContext *swrContext;
    int outChannels;
    jmethodID playAudioTrackMethodId;
    Queue *audioQueue;
} Player;

void playerInit(Player **player, JNIEnv *env, jobject instance, jobject surface) {
    *player = (Player*) malloc(sizeof(Player));
    JavaVM* javaVm;
    env->GetJavaVM(&javaVm);
    (*player)->javaVm = javaVm;
    (*player)->instance = env->NewGlobalRef(instance);
    (*player)->surface = env->NewGlobalRef(surface);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_myndkapplication2_Player_play(JNIEnv *env, jobject thiz, jstring _path, jobject surface) {
    //todo
}

int formatInit(Player *player, const char* path) { //初始化AVFormat
    int result;
    player->formatContext = avformat_alloc_context();
    result = avformat_open_input(&(player->formatContext), path, NULL, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open video file");
        return result;
    }
    result = avformat_find_stream_info(player->formatContext, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not find video file stream info");
        return result;
    }
    return SUCCESS_CODE;
}

int findStreamIndex(Player *player, AVMediaType type) { //查找解码器
    AVFormatContext* formatContext = player->formatContext;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == type) {
            return i;
        }
    }
    return -1;
}

int codecInit(Player* player, AVMediaType type) { //解码器初始化
    int result;
    AVFormatContext *formatContext = player->formatContext;
    int index = findStreamIndex(player, type);
    if (index == -1) {
        LOGE("Player Error : Can not find stream");
        return FAIL_CODE;
    }
    AVCodecContext  *codecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codecContext, formatContext->streams[index]->codecpar);
    AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
    result = avcodec_open2(codecContext, codec, NULL);
    if (result < 0) {
        LOGE("Player Error : Can not open codec");
        return FAIL_CODE;
    }
    if (type == AVMEDIA_TYPE_VIDEO) {
        player->videoStreamIndex = index;
        player->videoCodecContext = codecContext;
    } else if (type == AVMEDIA_TYPE_AUDIO) {
        player->audioStreamIndex = index;
        player->audioCodecContext = codecContext;
    }
    return SUCCESS_CODE;
}

int videoPrepare(Player *player, JNIEnv *env) { //播放视频准备
    AVCodecContext *codecContext = player->videoCodecContext;
    int videoWidth = codecContext->width;
    int videoHeight = codecContext->height;
    player->nativeWindow = ANativeWindow_fromSurface(env, player->surface);
    if (player->nativeWindow == NULL) {
        LOGE("Player Error : Can not create native window");
        return FAIL_CODE;
    }
    int result = ANativeWindow_setBuffersGeometry(player->nativeWindow, videoWidth,
                                                  videoHeight, WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        LOGE("Player Error : Can not set native window buffer");
        ANativeWindow_release(player->nativeWindow);
        return FAIL_CODE;
    }
    player->rgbaFrame = av_frame_alloc();
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    player->videoOutBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
    av_image_fill_arrays(player->rgbaFrame->data, player->rgbaFrame->linesize, player->videoOutBuffer,
                         AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    player->swsContext = sws_getContext(videoWidth, videoHeight, codecContext->pix_fmt,
                                        videoWidth, videoHeight, AV_PIX_FMT_RGBA,
                                        SWS_BICUBIC, NULL, NULL, NULL);
    return SUCCESS_CODE;
}

int audioPrepare(Player *player, JNIEnv *env) {
    AVCodecContext *codecContext = player->audioCodecContext;
    player->swrContext = swr_alloc();
    player->audioOutBuffer = (uint8_t *) av_malloc(44100 * 2);
    uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat outFormat = AV_SAMPLE_FMT_S16;
    int outSampleRate = player->audioCodecContext->sample_rate;
    swr_alloc_set_opts(player->swrContext, outChannelLayout, outFormat, outSampleRate,
                       codecContext->channel_layout, codecContext->sample_fmt, codecContext->sample_rate,
                       0, NULL);
    swr_init(player->swrContext);
    player->outChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    jclass  playerClass = env->GetObjectClass(player->instance);
    jmethodID createAudioTrackMethodId = env->GetMethodID(playerClass, "createAudioTrack", "(II)V");
    env->CallVoidMethod(player->instance, createAudioTrackMethodId, 44100, player->outChannels);
    player->playAudioTrackMethodId = env->GetMethodID(playerClass, "playAudioTrack", "([BI)V");
    return SUCCESS_CODE;
}

void videoPlay(Player* player, AVFrame *frame, JNIEnv *env) {
    int videoHeight = player->videoCodecContext->height;
    int result = sws_scale(player->swsContext, (const uint8_t* const*) frame->data, frame->linesize,
                           0, videoHeight, player->rgbaFrame->data, player->rgbaFrame->linesize);
    if (result <= 0) {
        LOGE("Player Error : video data convert fail");
        return;
    }
    result = ANativeWindow_lock(player->nativeWindow, &(player->windowBuffer), NULL);
    if (result < 0) {
        LOGE("Player Error : Can not lock native window");
    } else {
        uint8_t *bits = (uint8_t *)player->windowBuffer.bits;
        for (int h = 0; h < videoHeight; h++) {
            memcpy(bits + h * player->windowBuffer.stride * 4,
                   player->videoOutBuffer + h * player->rgbaFrame->linesize[0],
                   player->rgbaFrame->linesize[0]);
        }
        ANativeWindow_unlockAndPost(player->nativeWindow);
    }
}

