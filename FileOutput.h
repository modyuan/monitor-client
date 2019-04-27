//
// Created by yuansu on 2019-04-16.
//

#ifndef FFMPEG_TEST_FILEOUTPUT_H
#define FFMPEG_TEST_FILEOUTPUT_H

#include "ffmpeg.h"
#include <string>
#include <mutex>
#include <thread>
#include <ctime>
#include <atomic>
#include "DeviceInput.h"

using namespace std;


struct filePacket{
    uint64_t index;
    uint8_t* stream;
    int      length;
};

class FileOutput {
private:
    string filePath; // place to store file

    AVFormatContext *oFormatCtx;

    AVCodecContext* videoCodecCtx;
    AVCodecContext* audioCodecCtx;

    AVStream* videoStream;
    AVStream* audioStream;


    atomic_uint64_t videoFrameCount;
    atomic_uint64_t audioFrameCount;
    AVPacket videoPkt;
    AVPacket audioPkt;

    int frameSize;
    int pointSize;

    AVFrame* videoFrame;
    AVFrame* audioFrame;

    uint64_t startTime;
    queue<filePacket> outbuffer;

    atomic_bool running;

    thread* writeFileLoopThread;

    mutex fileMutex;

    DeviceInput* deviceInput;

    bool OpenFile(const string &path);
    bool CloseFile();
    //void ResetFrameCount(){videoFrameCount = audioFrameCount = 0;}


public:
    // path should not end with '/'
    FileOutput(string path,DeviceInput *in):filePath(std::move(path)),deviceInput(in),running(true){};
    bool OpenOutputStream();
    int  WriteVideoFrame(AVFrame* frame);
    int  WriteAudioFrame(AVFrame* frame);

    void WriteAFile(const string& filename, int index);

    void StartWriteFileLoop();
    void Close();

};


#endif //FFMPEG_TEST_FILEOUTPUT_H
