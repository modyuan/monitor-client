//
// Created by yuansu on 2019-04-23.
//

#ifndef FFMPEG_TEST_UPLOAD_H
#define FFMPEG_TEST_UPLOAD_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace std;


int ipstoi(const char* ip,uint32_t *out);

class Upload {
private:
    string server_address;
    int port;
    int sockfd;

    sockaddr_in serv_addr;
public:
    Upload(string addr, int port);
    bool SendVideo(uint8_t* stream,int length, uint64_t index);
    ~Upload();
};


#endif //FFMPEG_TEST_UPLOAD_H
