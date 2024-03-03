#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
using namespace std;


int main() {
    //先输入文件名，看文件是否能创建成功
	string filename = "rec.zip"; //文件名

	FILE *fp = fopen(filename.c_str(), "wb"); //以二进制方式打开（创建）文件
	if (fp == NULL) {
		printf("Cannot open file, press any key to exit!\n");
		system("pause");
		exit(0);
    }
    
    // 创建socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        cout << "create socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    } else cout << "create socket success!" << endl;

    // 连接服务端
    string ip = "127.0.0.1";
    int port = 8080;
    sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    sockaddr.sin_port = htons(port);
//    if (::bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
//        cout << "bind socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
//        return 1;
//    } else cout << "bind socket success!" << endl;
    if (::connect(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        cout << "connect socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    }
    
    int size = 1024*1024*1024;
    int ret_flag;
    // 写缓冲区
    ret_flag = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));

    // 读缓冲区
    ret_flag = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));

    
    // 向服务端发送数据
    char buffer[1024] = { 0 }; //文件缓冲区
	int nCount;
	while ((nCount = recv(sockfd, buffer, 1024, 0)) > 0) {
		fwrite(buffer, nCount, 1, fp);
	}
	puts("File transfer success!");
 
	//文件接收完毕后直接关闭套接字，无需调用shutdown()
	fclose(fp);

    // 关闭socket
    ::close(sockfd);
    return 0;
}
