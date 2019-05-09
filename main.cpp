#include "ffmpeg.h"
#include "UserWindow.h"
#include "DeviceInput.h"
#include "FileOutput.h"
#include "Upload.h"

UserWindow  * userWindow  = nullptr;
FileOutput  * fileOutput  = nullptr;
DeviceInput * deviceInput = nullptr;
Upload      * upload      = nullptr;

int main(int argc,char **argv) {
    if(argc == 1){
        printf("Usage:\n\t%s server-ip [port]\n",argv[0]);
        exit(-1);
    }

    int port = 80;
    if(argc == 3)  port = atoi(argv[2]);
    printf("Ready to link server: %s:%d\n",argv[1],port);

    avdevice_register_all();

    //Open device
    deviceInput = new DeviceInput();
    deviceInput->OpenAudioDevice();
    deviceInput->OpenVideoDevice();

    puts("open device");



    upload = new Upload(argv[1],8000);

    //Open output
    fileOutput = new FileOutput(deviceInput,upload);
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


    //this_thread::sleep_for(20s);
    userWindow->EventLoop();

    fileOutput->Close();
    puts("close file output");
    deviceInput->Close();
    puts("close device input");

    return 0;
}