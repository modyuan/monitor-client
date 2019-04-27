#include "ffmpeg.h"
#include "UserWindow.h"
#include "DeviceInput.h"
#include "FileOutput.h"
#include "Upload.h"

UserWindow  * userWindow  = nullptr;
FileOutput  * fileOutput  = nullptr;
DeviceInput * deviceInput = nullptr;
Upload      * upload      = nullptr;

int main() {

    avdevice_register_all();

    //Open device
    deviceInput = new DeviceInput();
    deviceInput->OpenAudioDevice();
    deviceInput->OpenVideoDevice();

    puts("open device");

    upload = new Upload("127.0.0.1",8000);

    //Open output
    fileOutput = new FileOutput(deviceInput,upload);
    fileOutput->OpenOutputStream();

    puts("open output");

    int w,h,s,c;
    deviceInput->GetInfo(w,h,c,s);

    // create show window
    //userWindow = new UserWindow();
    //userWindow->CreateWindow(w,h);

    puts("create window");

    deviceInput->SetVideoCB([](const AVFrame* frame){
        //userWindow->Refresh(frame->data[0],frame->linesize[0]);
    });

    deviceInput->StartRecord();
    puts("start record");


    fileOutput->StartWriteFileLoop();


    this_thread::sleep_for(20s);
    //userWindow->EventLoop();

    fileOutput->Close();
    puts("close file output");
    deviceInput->Close();
    puts("close device input");

    return 0;
}