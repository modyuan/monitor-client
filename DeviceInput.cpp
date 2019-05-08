//
// Created by yuansu on 2019-04-16.
//

#include "DeviceInput.h"

#define error(msg) fprintf(stderr,"%s\n",msg)

const int BUF_SECONDS = 2;
const int AAC_FRAME_SIZE = 1024;
const int FPS = 30;

//format in buffer.
const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;
const AVPixelFormat requireVideoFmt = AV_PIX_FMT_YUV420P; // fill video buffer is still hardcode


bool DeviceInput::OpenVideoDevice() {
    AVDictionary *options = nullptr;
    videoFormatCtx = nullptr;
    int ret;

#ifdef MACOS
    AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    av_dict_set(&options, "framerate", "30", 0);
    av_dict_set(&options, "video_size", "1280x720", 0);
    av_dict_set(&options, "pixel_format", "uyvy422", 0);
    //"0" for video , ":0" for audio
    ret = avformat_open_input(&videoFormatCtx, "0", inputFormat, &options);

#else
    av_dict_set(&options,"framerate","30",0);
    AVInputFormat *inputFormat=av_find_input_format("video4linux2");
    ret = avformat_open_input(&videoFormatCtx, "/dev/video0", inputFormat, &options);
#endif
    if (ret != 0) {
        error("Couldn't open input video stream.");
        return false;
    }

    if (avformat_find_stream_info(videoFormatCtx, nullptr) < 0) {
        error("Couldn't find video stream information.");
        return false;
    }

    videoStream = nullptr;
    for (int i = 0; i < videoFormatCtx->nb_streams; i++) {
        if (videoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = videoFormatCtx->streams[i];
            break;
        }
    }
    if (!videoStream) {
        error("Couldn't find a video stream.");
        return false;
    }

    AVCodec *pCodec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    videoCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(videoCodecCtx, videoStream->codecpar);

    if (avcodec_open2(videoCodecCtx, pCodec, nullptr) < 0) {
        error("Could not open video codec.");
        return false;
    }

    // format convert
    videoConverter = sws_getContext(videoStream->codecpar->width, videoStream->codecpar->height,
                                   (AVPixelFormat) videoStream->codecpar->format,
                                   videoStream->codecpar->width, videoStream->codecpar->height, requireVideoFmt,
                                   SWS_BICUBIC, nullptr, nullptr, nullptr);


    return true;
}


bool DeviceInput::OpenAudioDevice() {
    AVDictionary *options = nullptr;
    audioFormatCtx = nullptr;
    int ret;

#ifdef MACOS
    AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    //"0" for video , ":0" for audio
    ret = avformat_open_input(&audioFormatCtx, ":0", inputFormat, &options);

#else
    AVInputFormat *inputFormat=av_find_input_format("pulse");
    ret = avformat_open_input(&audioFormatCtx, "default", inputFormat, &options);
#endif
    if (ret != 0) {
        error("Couldn't open input audio stream.");
        return false;
    }

    if (avformat_find_stream_info(audioFormatCtx, nullptr) < 0) {
        error("Couldn't find audio stream information.");
        return false;
    }

    audioStream = nullptr;
    for (int i = 0; i < audioFormatCtx->nb_streams; i++) {
        if (audioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = audioFormatCtx->streams[i];
            break;
        }
    }
    if (!audioStream) {
        error("Couldn't find a audio stream.");
        return false;
    }

    AVCodec *pCodec = avcodec_find_decoder(audioStream->codecpar->codec_id);
    audioCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(audioCodecCtx, audioStream->codecpar);

    if (avcodec_open2(audioCodecCtx, pCodec, nullptr) < 0) {
        error("Could not open video codec.");
        return false;
    }

    // audio converter, convert other fmt to requireAudioFmt
    audioConverter = swr_alloc_set_opts(nullptr,
                                         av_get_default_channel_layout(audioCodecCtx->channels),
                                         requireAudioFmt,  // aac encoder only receive this format
                                         audioCodecCtx->sample_rate,
                                         av_get_default_channel_layout(audioCodecCtx->channels),
                                         (AVSampleFormat) audioStream->codecpar->format,
                                         audioStream->codecpar->sample_rate,
                                         0, nullptr);
    swr_init(audioConverter);

    // few seconds FIFO buffer
    audioFifo = av_audio_fifo_alloc(requireAudioFmt, audioCodecCtx->channels,
                                    audioCodecCtx->sample_rate * BUF_SECONDS);

    return true;
}

void DeviceInput::StartRecord() {
    isRecording = true;
    vt = new thread(&DeviceInput::ThreadRecordVideo, this);
    at = new thread(&DeviceInput::ThreadRecordAudio, this);
}

void DeviceInput::StopRecord() {
    bool r = isRecording.exchange(false);

    if(r){
        vt->join();
        at->join();
    }
}

void DeviceInput::ThreadRecordAudio() {
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame *frame = av_frame_alloc();
    int ret;
    while (isRecording.load()) {
        ret = av_read_frame(audioFormatCtx, &packet);
        if (ret < 0) {
            error("fail to av_read_frame in audio.");
            return;
        }

        ret = avcodec_send_packet(audioCodecCtx, &packet);
        if (ret != 0) {
            error("fail to avcodec_send_packet in audio.");
            return;
        }

        ret = avcodec_receive_frame(audioCodecCtx, frame);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                continue;
            } else {
                error("fail to avcodec_receive_frame in audio.");
                return;
            }
        }
        av_packet_unref(&packet);

        uint8_t **convertedBuffer = nullptr;
        ret = av_samples_alloc_array_and_samples(&convertedBuffer, nullptr, audioCodecCtx->channels, frame->nb_samples, requireAudioFmt, 0);

        if(ret<0){
            error("fail to alloc sample");
            exit(-11);
        }

        ret = swr_convert(audioConverter,convertedBuffer,frame->nb_samples,(const uint8_t**)frame->extended_data,frame->nb_samples);


        afLock.lock();
        if (av_audio_fifo_space(audioFifo) < frame->nb_samples) {
            //space is not enought to put frame
            int cutSize = frame->nb_samples - av_audio_fifo_space(audioFifo);
            av_audio_fifo_drain(audioFifo, cutSize);
            fprintf(stderr, "audioBuffer is full, discard %d samples.\n", cutSize);
        }
        av_audio_fifo_write(audioFifo, (void **) convertedBuffer, frame->nb_samples);
        afLock.unlock();
        av_freep(&convertedBuffer[0]);

        if (audioCB) audioCB(frame);

    }
    av_frame_free(&frame);
}

void DeviceInput::ThreadRecordVideo() {
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame *frame = av_frame_alloc();
    int ret;
    while (isRecording.load()) {
        ret = av_read_frame(videoFormatCtx, &packet);
        if (ret < 0) {
            error("fail to av_read_frame in video.");
            return;
        }

        ret = avcodec_send_packet(videoCodecCtx, &packet);
        if (ret != 0) {
            error("fail to avcodec_send_packet in video.");
            return;
        }

        ret = avcodec_receive_frame(videoCodecCtx, frame);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                continue;
            } else {
                error("fail to avcodec_receive_frame in video.");
                return;
            }
        }
        av_packet_unref(&packet);

        AVFrame *cFrame = av_frame_alloc();
        av_image_alloc(cFrame->data, cFrame->linesize, videoStream->codecpar->width, videoStream->codecpar->height,
                       requireVideoFmt, 1);
        cFrame->width = videoStream->codecpar->width;
        cFrame->height = videoStream->codecpar->height;
        cFrame->format = requireVideoFmt;
        sws_scale(videoConverter, frame->data, frame->linesize, 0, videoStream->codecpar->height, cFrame->data,
                  cFrame->linesize);

        if (videoCB) videoCB(cFrame);

        vfLock.lock();
        if(videoFifo.size() >= BUF_SECONDS * FPS){
            videoFifo.pop();
        }
        videoFifo.push(cFrame);
        vfLock.unlock();



    }
    av_frame_free(&frame);
}

void DeviceInput::Close() {

    StopRecord();
    ClearBuffer();
    sws_freeContext(videoConverter);
    swr_free(&audioConverter);

    av_audio_fifo_free(audioFifo);

    avcodec_free_context(&videoCodecCtx);
    avcodec_free_context(&audioCodecCtx);

    avformat_close_input(&videoFormatCtx);
    avformat_close_input(&audioFormatCtx);

}

// get one frame
AVFrame * DeviceInput::GetVideoFrame() {

    vfLock.lock();
    if (!videoFifo.empty()) {
        AVFrame* frame = videoFifo.front();
        videoFifo.pop();
        vfLock.unlock();
        return frame;
    } else {
        vfLock.unlock();
        return nullptr;
    }

}

// get AAC_FRAME_SIZE(1024) samples audio data which is a packet in AAC encoder
AVFrame * DeviceInput::GetAudioFrame() {
    int ret;
    afLock.lock();
    if (av_audio_fifo_size(audioFifo) >= AAC_FRAME_SIZE) {
        AVFrame *frame = av_frame_alloc();
        frame->nb_samples= AAC_FRAME_SIZE;
        frame->sample_rate = audioStream->codecpar->sample_rate;
        frame->format = requireAudioFmt;
        frame->channel_layout = av_get_default_channel_layout(audioStream->codecpar->channels);

        ret = av_frame_get_buffer(frame,0);
        assert(ret==0);

        av_audio_fifo_read(audioFifo, (void **) frame->data, AAC_FRAME_SIZE);
        afLock.unlock();
        return frame;
    } else {
        afLock.unlock();
        return nullptr;
    }

}

void DeviceInput::ClearBuffer() {
    vfLock.lock();
    while(!videoFifo.empty()){
        AVFrame* f = videoFifo.front();
        av_frame_free(&f);
        videoFifo.pop();
    }
    vfLock.unlock();

    afLock.lock();
    av_audio_fifo_drain(audioFifo, av_audio_fifo_space(audioFifo));
    afLock.unlock();
    printf("cleared audio buffer and video buffer.\n");
}

