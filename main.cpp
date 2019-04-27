#include "ffmpeg.h"
#include "UserWindow.h"
#include "DeviceInput.h"
#include "FileOutput.h"
#include "Upload.h"

UserWindow  * userWindow  = nullptr;
FileOutput  * fileOutput  = nullptr;
DeviceInput * deviceInput = nullptr;

#include <time.h>
int main() {

    avdevice_register_all();

    //Open device
    deviceInput = new DeviceInput();
    deviceInput->OpenAudioDevice();
    deviceInput->OpenVideoDevice();

    puts("open device");

    //Open output
    fileOutput = new FileOutput("/Users/yuan/Desktop/test",deviceInput);
    fileOutput->OpenOutputStream();

    puts("open output");

    int w,h,s,c;
    deviceInput->GetInfo(w,h,c,s);

    // create show window
    userWindow = new UserWindow();
    userWindow->CreateWindow(w,h);

    puts("create window");

    deviceInput->SetVideoCB([](const AVFrame* frame){
        userWindow->Refresh(frame->data[0],frame->linesize[0]);
    });

    deviceInput->StartRecord();
    puts("start record");


    fileOutput->StartWriteFileLoop();


    userWindow->EventLoop();

    fileOutput->Close();
    puts("close file output");
    deviceInput->Close();
    puts("close device input");

    return 0;
}