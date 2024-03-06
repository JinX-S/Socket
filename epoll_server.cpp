#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
   
#define SERVER_PORT 8080 // 端口
#define LENGTH_OF_LISTEN_QUEUE 20 // 
#define BUFFER_SIZE 1024
#define FILE_NAME_MAX_SIZE 512 

struct sockaddr_in server_addr;	// 服务端
struct sockaddr_in client_addr;	// 客户端

socklen_t client_addr_length = sizeof(client_addr);       // 定义客户端的socket地址结构 

int server_socket_fd;	// 服务端socket
int new_server_socket_fd;	// 客户端socket
int opt = 1; 
int length = 0; 

char buffer[BUFFER_SIZE];	// 接收数据
char file_name[FILE_NAME_MAX_SIZE+1];  

FILE *fp;

// epoll
int n, num =0, sockfd;
ssize_t nready, efd, res;
epoll_event tep, ep[FOPEN_MAX]; //tep: epoll_ctl参数  ep[] : epoll_wait参数

// 声明并初始化一个服务器端的socket地址结构
int Start_Server_Socket() {
  	bzero(&server_addr, sizeof(server_addr)); 
  	server_addr.sin_family = AF_INET; 
  	server_addr.sin_addr.s_addr = htons(INADDR_ANY); 
  	server_addr.sin_port = htons(SERVER_PORT); 
  	return 1;
}

// socket:创建socket，若成功，返回socket描述符 
int Creat_Socket() {
  	server_socket_fd = socket(PF_INET, SOCK_STREAM, 0); 
  	if(server_socket_fd < 0) 
  	{ 
    		perror("Create socket error:"); 
    		exit(1); 
  	} 
  	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
	return 1;
}

// bind:绑定客户端的socket和客户端的socket地址结构
int Client_Server_Bind() {
  	if(-1 == (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))) 
  	{ 
    		perror("Server Bind Failed:"); 
    		exit(1); 
  	} 
	return 1;
}

// listen:监听
int Serve_Listen() {
  	if(-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE))) { 
    		perror("Listen error:"); 
    		exit(1); 
  	}
	return 1; 
   
}

// accept:接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信
int Serve_Accept_Link() {
    	new_server_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_length);     // accept函数会把连接到的客户端信息写到client_addr中 
    	if(new_server_socket_fd < 0) { 
      		perror("Server Accept Failed:"); 
    		exit(0);
    	} 
	return 1;
}

int Send_File_Name()  // 接收需要发送的文件位置及文件名
{
 	// recv函数接收数据到缓冲区buffer中 
		bzero(buffer, BUFFER_SIZE); 
		n = recv(sockfd, buffer, BUFFER_SIZE, 0);
   	// 然后从buffer(缓冲区)拷贝到file_name中 
		bzero(file_name, FILE_NAME_MAX_SIZE+1); 
		strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer)); 
		printf("The file name which you want to send:%s\n", file_name); 
	return 1;
}

// 发送文件 recv, send
int Document_Send()	{
   	// 打开文件并读取文件数据 
    	fp = fopen(file_name, "r"); 
    	if(fp == NULL) { 
      		printf("File:%s Not Found\n", file_name); 
    	} else { 
      		bzero(buffer, BUFFER_SIZE); 
		// 每读取一段数据，便将其发送给客户端，循环直到文件读完为止 
      		while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) { 
        		if(send(sockfd, buffer, length, 0) < 0) { 
          			printf("Send File:%s Failed./n", file_name); 
          			exit(0); 
        		} 
        		bzero(buffer, BUFFER_SIZE); 
      		} 
      		// 关闭文件 
      		fclose(fp); 
      		printf("File:%s send success\n", file_name); 
    	} 
	return 1;
   
}


int Link_Trans() {
    efd = epoll_create(FOPEN_MAX);  //创建epoll模型, efd指向红黑树根节点
    if (efd == -1) {
        perror("epoll_create error!");
        exit(0);
    }
	// tep 用来设置单个fd属性，ep是、epoll_wait()传出的满足监听事件的数组
    tep.events = EPOLLIN;	// 初始化lfd的监听属性
    tep.data.fd = server_socket_fd; //指定server_socket_fd的监听时间为"读"
    res = epoll_ctl(efd, EPOLL_CTL_ADD, server_socket_fd, &tep);//将server_socket_fd及对应的结构体设置到树上,efd可找到该树
    if (res == -1) {
        perror("epoll_ctl error!");
        exit(0);
    }
    while (true) {
        /*epoll为server阻塞监听事件, ep为struct epoll_event类型数组, OPEN_MAX为数组容量, -1表永久阻塞*/
        nready = epoll_wait(efd, ep, FOPEN_MAX, -1); // nready 满足事件的总个数; 实施监听
        if (nready == -1) {
            perror("epoll_wait error!");
            exit(0);
        }

        for (int i = 0; i < nready; ++i) {
            if (!(ep[i].events & EPOLLIN)) continue;//如果不是"读"事件, 继续循环
            if (ep[i].data.fd == server_socket_fd) {//server_socket_fd满足读事件，有新的客户端发起连接请求 
                client_addr_length = sizeof(client_addr);
                Serve_Accept_Link();//接受链接
                tep.events = EPOLLIN;	// 初始化cfd的监听属性
                tep.data.fd = new_server_socket_fd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, new_server_socket_fd, &tep);//加入红黑树
                if (res == -1) {
                    perror("epoll_ctl error!");
                    exit(0);
                }
            } else {    // cfd们满足读事件，有客户端写数据来
                sockfd = ep[i].data.fd;
				Send_File_Name();
                if (n == 0) { // 读到0,说明客户端关闭链接
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, nullptr); // 将该文件描述符从红黑树摘除
                    if (res == -1) {
                        perror("epoll_ctl error!");
                        exit(0);
                    }
                    close(sockfd); //关闭与该客户端的链接
                    std::cout << "client:" << sockfd << "closed connection" << std::endl;
                } else if (n < 0) { //出错
                    perror("ecv n < 0 error!");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, nullptr);//摘除节点
                    close(sockfd);
                } else { //实际读到了字节数
                    Document_Send();
					close(sockfd);
                }

            }
        }
    }
	return 1;
}


int main(void) {
	Start_Server_Socket();
	Creat_Socket();
	Client_Server_Bind();  
	Serve_Listen();   
	Link_Trans();
   	close(server_socket_fd);    // 关闭监听用的socket
  	return 0; 
}
