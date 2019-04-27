//
// Created by yuansu on 2019-04-23.
//

#include "Upload.h"

#define error(msg) fprintf(stderr,"ERROR %s\n",msg)

Upload::Upload(string addr, int port):server_address(addr),port(port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("opening socket");

    sockaddr_in serv_addr;

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    uint32_t t = inet_addr(server_address.c_str());
    if(t==-1){
        error("fail to parse ip address");
        exit(-1);
    }
    serv_addr.sin_addr.s_addr = t;
    serv_addr.sin_port =htons(port);

    if(connect(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))== -1){
        error("connect socket");
        exit(-1);
    }

}

bool Upload::SendVideo(string filename,uint64_t index) {
    const char header[500] = "POST /video/%lld.ts HTTP/1.1\r\n"
                             "Accept: */*\r\n"
                             "User-Agent: Raspberry\n"
                             "Connection: Keep-Alive\r\n"
                             "Content-Length: %d\r\n\r\n";

    char h[500];int ret;

    int filesize = -1;
    struct stat statbuff;
    if(stat(filename.c_str(), &statbuff) < 0){
        return false;
    }else{
        filesize = statbuff.st_size;
    }

    FILE *fp = fopen(filename.c_str(),"rb");
    if(!fp) return false;

    sprintf(h,header,index,filesize);

    if(write(sockfd,h,strlen(h))<0)
        return false;

    auto fbuf = (uint8_t*)malloc(filesize);
    fread(fbuf,1,filesize,fp);

    if(write(sockfd,fbuf,filesize)<0)
        return false;

    return true;
}

Upload::~Upload() {
    close(sockfd);
}
