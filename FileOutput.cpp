//
// Created by yuansu on 2019-04-16.
//
/*
采样率:
采样率（也称为采样速度或者采样频率）定义了每秒从连续信号中提取并组成离散信号的采样个数，它用赫兹（Hz）来表示。
 采样频率的倒数叫作采样周期或采样时间，它是采样之间的时间间隔。

采样数:
AVFrame -> nb_samples
number of audio samples (per channel) described by this frame

采样率除以采样数 = (个/秒) / (个/每包) = 包/秒
*/

#include "FileOutput.h"

#define error(msg) fprintf(stderr,"%s\n",msg);


const int FPS = 30;
const int AAC_FRAME_SIZE = 1024;
const int GOP_SIZE = 10;

bool FileOutput::OpenOutputStream() {
    int w,h,s,c;
    deviceInput->GetInfo(w,h,c,s);
    int ret;
    oFormatCtx = nullptr;

    ret = avformat_alloc_output_context2(&oFormatCtx,nullptr,"mpegts", nullptr);
    if(ret < 0) { error("fail to alloc output format context."); return false;}

//-----video------
    AVCodec *videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(videoCodec == nullptr) {error("Can not find encoder H264."); return false;}

    videoCodecCtx = avcodec_alloc_context3(videoCodec);

    videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    videoCodecCtx->width   = w;
    videoCodecCtx->height  = h;

    // maybe no use, because encoder will overwrite it. so just use stream->timebase in set pts,dts
    videoCodecCtx->time_base.num = 1;
    videoCodecCtx->time_base.den = 30;
    videoCodecCtx->gop_size = GOP_SIZE;


    if(oFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
        videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    AVDictionary *dict=nullptr;
    av_dict_set(&dict, "preset", "fast", 0);
    av_dict_set(&dict, "tune", "zerolatency", 0);
    ret = avcodec_open2(videoCodecCtx,videoCodec,&dict);
    if(ret <0) {error("fail to open video encoder.");return false;}

    videoStream = avformat_new_stream(oFormatCtx,videoCodec);
    if(videoStream== nullptr){error("fail to create video stream.");return false;}

    avcodec_parameters_from_context(videoStream->codecpar,videoCodecCtx);
//--------audio------
    AVCodec *audioCodec = avcodec_find_encoder_by_name("libfdk_aac");
    //AVCodec *audioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(audioCodec == nullptr){error("Can not find encoder AAC."); return false;}

    audioCodecCtx = avcodec_alloc_context3(audioCodec);

    audioCodecCtx->channels = c;
    audioCodecCtx->channel_layout = (uint64_t)av_get_default_channel_layout(c);
    audioCodecCtx->sample_rate = s;
    audioCodecCtx->sample_fmt = audioCodec->sample_fmts[0];
    audioCodecCtx->bit_rate = 32000;

    audioCodecCtx->time_base.num = 1;
    audioCodecCtx->time_base.den = s;

    if(oFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
        audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    ret = avcodec_open2(audioCodecCtx,audioCodec,nullptr);
    if(ret <0) {error("fail to open audio encoder.");return false;}

    audioStream = avformat_new_stream(oFormatCtx,audioCodec);
    if(audioStream== nullptr) {error("fail to create audio stream.");return false;}

    avcodec_parameters_from_context(audioStream->codecpar,audioCodecCtx);

    // for pts
    videoFrameCount = audioFrameCount = 0;

    startTime = time(nullptr); // get current timestamp by seconds

    return true;
}

bool FileOutput::OpenFile(const string &path) {

    if (avio_open(&oFormatCtx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
        error("Failed to open output file.");
        return false;
    }
    if(avformat_write_header(oFormatCtx, nullptr) < 0){
        error("fail to write header");
        return false;
    }
    return true;
}

bool FileOutput::CloseFile() {
    av_write_trailer(oFormatCtx);
    avio_flush(oFormatCtx->pb);
    bool ret = avio_close(oFormatCtx->pb) == 0;
    if(!ret) fprintf(stderr,"fail to close file!");
    return ret;
}



int FileOutput::WriteVideoFrame(AVFrame *frame) {
    int ret;

    frame->pts = (uint64_t)videoFrameCount * videoStream->time_base.den / FPS + 10;
    videoFrameCount++;

    ret = avcodec_send_frame(videoCodecCtx,frame);
    assert(ret==0);

    av_init_packet(&videoPkt);
    ret = avcodec_receive_packet(videoCodecCtx,&videoPkt);
    if(ret !=0 ){
        if(ret == AVERROR(EAGAIN)){
            av_packet_unref(&videoPkt);
            printf("buffer video!\n");
            return 0;
        }else{
            fprintf(stderr, "fail to receive video packet!\n");
            exit(-1);
        }
    }

    // frame->pts equals to videoPkt->pts
    videoPkt.stream_index = videoStream->index;
    videoPkt.duration = (uint64_t)videoStream->time_base.den / FPS;
    videoPkt.dts=  videoPkt.pts;

    fileMutex.lock();
    ret = av_interleaved_write_frame(oFormatCtx,&videoPkt);
    fileMutex.unlock();
    av_packet_unref(&videoPkt);


    return 1;
}

int FileOutput::WriteAudioFrame(AVFrame *frame) {
    int ret;

    frame->pts = (uint64_t)audioFrameCount * audioStream->time_base.den * 1024 / audioCodecCtx->sample_rate+10;
    audioFrameCount++;
    ret = avcodec_send_frame(audioCodecCtx, frame);
    if(ret!=0){
        char tmpErrString[128] = { 0 };
        printf("Could not write audio frame, error: %s\n",
                av_make_error_string(tmpErrString, AV_ERROR_MAX_STRING_SIZE, ret));
        exit(-10);
    }

    av_init_packet(&audioPkt);
    ret = avcodec_receive_packet(audioCodecCtx,&audioPkt);
    if(ret !=0 ){
        if(ret == AVERROR(EAGAIN)){
            av_packet_unref(&audioPkt);
            return 0;
        }else{
            fprintf(stderr, "fail to receive audio packet!\n");
            exit(-1);
        }
    }

    audioPkt.stream_index = audioStream->index;
    //audioPkt.pts = audioFrameCount * audioStream->time_base.den / (audioCodecCtx->sample_rate / 1024);
    audioPkt.duration = (uint64_t)audioStream->time_base.den * 1024 / audioCodecCtx->sample_rate;
    audioPkt.dts =  audioPkt.pts = (uint64_t)audioFrameCount * audioPkt.duration ;
    //printf("audio pts: %lld\n",audioPkt.pts);

    fileMutex.lock();
    ret = av_interleaved_write_frame(oFormatCtx,&audioPkt);
    fileMutex.unlock();
    av_packet_unref(&audioPkt);


    return 1;
}

void FileOutput::StartWriteFileLoop(){

    writeFileLoopThread = new thread([this](){
        char path[512]={0};
        uint64_t fileCount = 0;
        while(running){
            snprintf(path,512,"%s/%lld.ts",filePath.c_str(),fileCount + startTime);
            WriteAFile(path,fileCount);
            printf("->%s\n",path);
            fileCount++;

        }
        puts("writefile loop exit");
    });

}
//index start with 1
void FileOutput::WriteAFile(const string& filename,int index) {

    this->OpenFile(filename);

    thread t1([this,index](){
        //puts("start");
        while(videoFrameCount.load() <  FPS * index){
            AVFrame* frame = this->deviceInput->GetVideoFrame();
            if(frame){
                //printf("frame count: %lld\n",videoFrameCount.load());
                this->WriteVideoFrame(frame);
                av_frame_free(&frame);
            }else{
                this_thread::sleep_for(chrono::nanoseconds(1000*1000*30));
            }

        }

    });

    thread t2([this,index](){
        // one second audio is not exact integer packet, is 44100 / 1024 ~= 43.07
        while(audioFrameCount < index * audioCodecCtx->sample_rate / AAC_FRAME_SIZE){
            AVFrame* frame = this->deviceInput->GetAudioFrame();
            if(frame){
                this->WriteAudioFrame(frame);
                av_frame_free(&frame);
            }else{
                this_thread::sleep_for(chrono::nanoseconds(1000*1000*30));
            }

        }
    });

    t1.join();
    t2.join();

    this->CloseFile();

}

void FileOutput::Close() {
    bool r = running.exchange(false);
    if(!r){
        return;
    }

    writeFileLoopThread->join();

    avcodec_free_context(&videoCodecCtx);
    avcodec_free_context(&audioCodecCtx);

    avformat_free_context(oFormatCtx);
    oFormatCtx = nullptr;
    deviceInput = nullptr;

}






















