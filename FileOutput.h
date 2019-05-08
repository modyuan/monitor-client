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
#include "Upload.h"

using namespace std;


struct filePacket {
    uint64_t index;
    uint8_t *stream;
    int length;
};

class FileOutput {
private:

    AVFormatContext *oFormatCtx;

    AVCodecContext *videoCodecCtx;
    AVCodecContext *audioCodecCtx;

    AVStream *videoStream;
    AVStream *audioStream;

    atomic_uint64_t videoFrameCount;
    atomic_uint64_t audioFrameCount;
    AVPacket videoPkt;
    AVPacket audioPkt;

    uint64_t startTime;

    atomic_bool running;

    thread *writeFileLoopThread;

    mutex fileMutex;

    uint8_t *ioBuffer;
    uint8_t *fileBuffer;
    int fileBufferUsage;

    DeviceInput *deviceInput;
    Upload *upload;

    bool OpenFile();
    bool CloseFile();

public:

    void UpdateBuffer(int size);

    // path should not end with '/'
    FileOutput( DeviceInput *in, Upload *up) :
            deviceInput(in), upload(up),
            running(true) {};

    bool OpenOutputStream();

    int WriteVideoFrame(AVFrame *frame,bool forceI);

    int WriteAudioFrame(AVFrame *frame);

    void WriteAFile(int index);

    void StartWriteFileLoop();

    void Close();

};


#endif //FFMPEG_TEST_FILEOUTPUT_H
