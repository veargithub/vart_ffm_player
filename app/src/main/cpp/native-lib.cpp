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
}

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, "player", FORMAT, ##__VA_ARGS__);

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_myndkapplication2_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());
    return env->NewStringUTF(avcodec_configuration());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myndkapplication2_MainActivity_playVideo(JNIEnv *env, jobject instance, jstring path_, jobject surface) {
    int result; //记录结果
    const char * path = env->GetStringUTFChars(path_, 0); //java string 转 c string
//    av_register_all(); //4.0之后不需要这个方法了
    AVFormatContext *format_context = avformat_alloc_context(); //初始化format context上下文
    result = avformat_open_input(&format_context, path, NULL, NULL); //打开视频文件
    if (result < 0) {
        LOGE("打开视频文件失败");
        return;
    }
    result = avformat_find_stream_info(format_context, NULL);
    if (result < 0) {
        LOGE("获取流信息失败");
        return;
    }
    //查找视频编码器
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//匹配视频流
            video_stream_index = i;
        }
    }
    if (video_stream_index == -1) {
        LOGE("没找到视频流");
        return;
    }
    AVCodecContext *video_codec_context = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(video_codec_context, format_context->streams[video_stream_index]->codecpar);

    AVCodec *video_codec = avcodec_find_decoder(video_codec_context->codec_id);
    if (video_codec == NULL) {
        LOGE("找不到视频解码器");
        return;
    }
    result = avcodec_open2(video_codec_context, video_codec, NULL);
    if (result < 0) {
        LOGE("无法打开视频解码器");
        return;
    }

    int videoWidth = video_codec_context->width;
    int videoHeight = video_codec_context->height;

    ANativeWindow *native_window = ANativeWindow_fromSurface(env, surface);
    if (native_window == NULL) {
        LOGE("无法创建surface");
        return;
    }
    result = ANativeWindow_setBuffersGeometry(native_window, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
    if (result < 0) {
        LOGE("无法创建缓冲");
        ANativeWindow_release(native_window);
        return;
    }

    ANativeWindow_Buffer windowBuffer;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *rgbaFrame = av_frame_alloc();

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    uint8_t  *outBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, outBuffer, AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    struct SwsContext *dataConvertContext = sws_getContext(videoWidth, videoHeight, video_codec_context->pix_fmt,
            videoWidth, videoHeight, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    while (av_read_frame(format_context, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            result = avcodec_send_packet(video_codec_context, packet);
            if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                LOGE("解码失败1");
                return;
            }
            result = avcodec_receive_frame(video_codec_context, frame);
            if (result == AVERROR(EAGAIN)) {//无法读取当前帧，可以尝试去读下一帧
                LOGE("eagain");
                continue;
            }
            if (result < 0 && result != AVERROR_EOF) {
                LOGE("解码失败2====%d %d %d", result, AVERROR_DECODER_NOT_FOUND, AVERROR_DEMUXER_NOT_FOUND);
                return;
            }
            result = sws_scale(dataConvertContext,
                               (const uint8_t* const*) frame->data, frame->linesize,
                               0, videoHeight,
                               rgbaFrame->data, rgbaFrame->linesize);
            if(result < 0) {
                LOGE("数据转换失败");
                return;
            }
            //播放
            result = ANativeWindow_lock(native_window, &windowBuffer, NULL);
            if (result < 0) {
                LOGE("lock native error");
            } else {
                uint8_t *bits = (uint8_t *) windowBuffer.bits;
                for (int h = 0; h < videoHeight; h++) {
                    mempcpy(bits + h * windowBuffer.stride * 4,
                            outBuffer + h * rgbaFrame->linesize[0],
                            rgbaFrame->linesize[0]);
                }
                ANativeWindow_unlockAndPost(native_window);
            }
        }
        av_packet_unref(packet);
    }

    sws_freeContext(dataConvertContext);

    av_free(outBuffer);

    av_frame_free(&rgbaFrame);

    av_frame_free(&frame);

    av_packet_free(&packet);

    ANativeWindow_release(native_window);

    avcodec_close(video_codec_context);

    avformat_close_input(&format_context);

    env->ReleaseStringUTFChars(path_, path);


}