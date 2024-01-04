//
// Created by vartc on 2023/12/7.
//

#include <jni.h>
#include <string>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
}

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myndkapplication2_AudioPlayer_playAudio(JNIEnv *env, jobject thiz, jstring path_) {
    int result;
    const char *path = env->GetStringUTFChars(path_, 0);
    AVFormatContext *formatContext = avformat_alloc_context();
    avformat_open_input(&formatContext, path, NULL, NULL);
    result = avformat_find_stream_info(formatContext, NULL);
    if (result < 0) {
        LOGE("can not find video file stream info");
        return;
    }
    int audioStreamIndex = -1;
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        LOGE("can not find audio stream");
        return;
    }

    AVCodecContext *audioCodecContext = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(audioCodecContext, formatContext->streams[audioStreamIndex]->codecpar);
    AVCodec *audioCodec = avcodec_find_decoder(audioCodecContext->codec_id);
    if (audioCodec == NULL) {
        LOGE("can not find audio decoder");
        return;
    }

    result = avcodec_open2(audioCodecContext, audioCodec, NULL);
    if (result < 0) {
        LOGE("can not open audio decoder");
        return;
    }

    struct SwrContext *swrContext = swr_alloc(); //重采样上下文
    uint8_t *outBuffer = (uint8_t *) av_malloc(44100 * 2); //缓冲区
    uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO; //输出的声道布局（双通道，立体声）
    enum AVSampleFormat outFormat = AV_SAMPLE_FMT_S16; //输出采样位数16位
    int outSampleRate = audioCodecContext->sample_rate;
    swr_alloc_set_opts(swrContext,
                       outChannelLayout, outFormat, outSampleRate,
                       audioCodecContext->channel_layout, audioCodecContext->sample_fmt, audioCodecContext->sample_rate,
                       0, NULL);
    swr_init(swrContext);
    int outChannels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    jclass audioPlayerClass = env->GetObjectClass(thiz);
    jmethodID  createAudioTrackMethodId = env->GetMethodID(audioPlayerClass, "createAudioTrack", "(II)V");
    env->CallVoidMethod(thiz, createAudioTrackMethodId, 44100, outChannels);
    jmethodID  playAudioTrackMethodId = env->GetMethodID(audioPlayerClass, "playAudioTrack", "([BI)V");
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == audioStreamIndex) {
            result = avcodec_send_packet(audioCodecContext, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 1 fail");
                return;
            }
            result = avcodec_receive_frame(audioCodecContext, frame);
            if (result == AVERROR(EAGAIN)) {//无法读取当前帧，可以尝试去读下一帧
                LOGE("eagain audio");
                continue;
            }
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("Player Error : codec step 2 fail");
                return;
            }
            swr_convert(swrContext, &outBuffer, 44100 * 2, (const uint8_t **) frame->data, frame->nb_samples);
            int size = av_samples_get_buffer_size(NULL, outChannels, frame->nb_samples, AV_SAMPLE_FMT_S16, 1);
            jbyteArray  audioSampleArray = env->NewByteArray(size);
            env->SetByteArrayRegion(audioSampleArray, 0, size, (const jbyte*) outBuffer);
            env->CallVoidMethod(thiz, playAudioTrackMethodId, audioSampleArray, size);
            env->DeleteLocalRef(audioSampleArray);
        }
        av_packet_unref(packet);
    }

    jmethodID  releaseAudioTrackMethodId = env->GetMethodID(audioPlayerClass, "releaseAudioTrack", "()V");
    env->CallVoidMethod(thiz, releaseAudioTrackMethodId);
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_close(audioCodecContext);
    avformat_close_input(&formatContext);
    avformat_free_context(formatContext);
    env->ReleaseStringUTFChars(path_, path);
}