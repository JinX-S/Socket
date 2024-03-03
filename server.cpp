#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <thread>

using namespace std;

void *fun(int cfd, FILE *fp) {
    
    // 接收客户端数据
    char buffer[1024] = { 0 }; //缓冲区
	int nCount;
	while ((nCount = fread(buffer, 1, 1024, fp)) > 0) {//以1KB为单位传输
		send(cfd, buffer, nCount, 0);
	}
 
	shutdown(cfd, SHUT_WR); //文件读取完毕，断开输出流，向客户端发送FIN包
	recv(cfd, buffer, 1024, 0); //阻塞，等待客户端接收完毕
 
	fclose(fp);


}





int main() {
    //先检查文件是否存在
	string filename = "/home/hgs/文档/socket1/send.zip"; //文件名
	FILE *fp = fopen(filename.c_str(), "rb"); //以二进制方式打开文件
	if (fp == NULL) {
		printf("Cannot open file, press any key to exit!\n");
		system("pause");
		exit(0);
	}

    // 创建socket
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int clsockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    socklen_t clsock_add_len;
    if (sockfd < 0) {
        cout << "create socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    } else cout << "create socket success!" << endl;

    // 绑定socket
    string ip = "127.0.0.1";
    int port = 8080;
    sockaddr_in sockaddr, clit_addr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
//    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(port);
    if (::bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        cout << "bind socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    } else cout << "bind socket success! ip:" << ip.c_str() << " port:" << port << endl;

    // 监听socket
    if (::listen(sockfd, 1024) < 0) {
        cout << "listen socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    } else cout << "socket listing..." << endl;


    // 接受客户端连接
    clsock_add_len = sizeof(clit_addr);
    int connfd = ::accept(sockfd, (struct sockaddr*)&clit_addr, &clsock_add_len);
    char client_ip[2014] = {0};
    cout << "client ip:" <<
        inet_ntop(AF_INET, &clit_addr.sin_addr.s_addr, client_ip, sizeof(client_ip)) <<
        "port:" << ntohs(clit_addr.sin_port)
        << endl;

    if (connfd < 0) {
        cout << "accept socket error: errno: " << errno << " errmsg: " << strerror(errno) << endl;
        return 1;
    }
    int size = 1024*1024*1024;
    int ret_flag;
    // 写缓冲区
    ret_flag = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
    // 读缓冲区
    ret_flag = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));

    thread prid(fun, connfd, fp);
    prid.detach();

    // 关闭socket
    ::close(sockfd);
    return 0;

}
