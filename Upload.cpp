#include "Upload.h"

#define error(msg) fprintf(stderr,"ERROR %s\n",msg)

Upload::Upload(string addr, int port):server_address(addr),port(port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("opening socket");

    auto server = gethostbyname(server_address.c_str());

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
    serv_addr.sin_port =htons(port);

    if(connect(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        error("connect socket");
        exit(-1);
    }else{
        printf("connect to server: %s:%d\n",server_address.c_str(),port);
    }
}

bool Upload::SendVideo(uint8_t* stream,int length, uint64_t index){
    const char header[500] = "POST /video/%lld.ts HTTP/1.1\r\n"
                             "HOST: %s:%d\r\n"
                             "Accept: */*\r\n"
                             "User-Agent: Raspberry\r\n"
                             "Content-Type: video/mpegts\r\n"
                             "Connection: keep-alive\r\n"
                             "Content-Length: %d\r\n\r\n";


    char h[500];
    sprintf(h,header,index,server_address.c_str(),port,length);

    //write header
    int ret = send(sockfd,h,strlen(h),0);
    if(ret<0){
        error("fail write header");
        return false;
    }

    //write body
    ret = send(sockfd,stream,length,0);
    if(ret<0){
        error("fail write body");
        return false;
    }

    return true;
}

Upload::~Upload() {
    close(sockfd);
}
