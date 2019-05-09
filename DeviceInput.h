#ifndef FFMPEG_TEST_DEVICEINPUT_H
#define FFMPEG_TEST_DEVICEINPUT_H

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include "ffmpeg.h"
using namespace std;
using DeviceCB = void(*)(const AVFrame* frame);

class DeviceInput {
private:
    thread *vt,*at;
    atomic_bool isRecording;
    void ThreadRecordAudio();
    void ThreadRecordVideo();

    AVFormatContext *videoFormatCtx;
    AVFormatContext *audioFormatCtx;
    AVCodecContext *videoCodecCtx;
    AVCodecContext *audioCodecCtx;

    AVStream *videoStream;
    AVStream *audioStream;

    int frameSize;
    int pointSize;

    //fifo lock
    mutex afLock;
    mutex vfLock;
    AVAudioFifo  *audioFifo;
    queue<AVFrame*> videoFifo;


    SwsContext *videoConverter;
    SwrContext *audioConverter;

    DeviceCB videoCB;
    DeviceCB audioCB;

public:

    DeviceInput():videoCB(nullptr),audioCB(nullptr),vt(nullptr),at(nullptr),
    videoFormatCtx(nullptr),audioFormatCtx(nullptr),videoCodecCtx(nullptr),
    audioCodecCtx(nullptr),videoStream(nullptr),audioStream(nullptr),
    isRecording(false),videoConverter(nullptr),audioConverter(nullptr){};
    bool OpenVideoDevice();
    bool OpenAudioDevice();
    void StartRecord();
    void StopRecord();
    void SetVideoCB(DeviceCB cb){this->videoCB = cb;}
    void SetAudioCB(DeviceCB cb){this->audioCB = cb;}

    AVFrame* GetVideoFrame();
    AVFrame* GetAudioFrame();
    void ClearBuffer();
    void Close();
    void GetInfo(int &width,int &height,int &channels, int& sample_rate){
        width = videoStream->codecpar->width;
        height = videoStream->codecpar->height;
        channels = audioStream->codecpar->channels;
        sample_rate = audioStream->codecpar->sample_rate;
    }

};


#endif //FFMPEG_TEST_DEVICEINPUT_H
