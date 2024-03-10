#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>
   
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

// select参数
int ret, maxfd = 0, rc;
fd_set rset, allset; // 定义读集合，备份集合allset

int Start_Server_Socket()  // 声明并初始化一个服务器端的socket地址结构 
{
  	bzero(&server_addr, sizeof(server_addr)); 
  	server_addr.sin_family = AF_INET; 
  	server_addr.sin_addr.s_addr = htons(INADDR_ANY); 
  	server_addr.sin_port = htons(SERVER_PORT); 
  	return 1;
}

int Creat_Socket()  // 创建socket，若成功，返回socket描述符 
{
  	server_socket_fd = socket(PF_INET, SOCK_STREAM, 0); 
  	if(server_socket_fd < 0) 
  	{ 
    		perror("Create socket error:"); 
    		exit(1); 
  	} 
  	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
	return 1;
}

int Client_Server_Bind() // 绑定客户端的socket和客户端的socket地址结构
{
  	if(-1 == (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))) 
  	{ 
    		perror("Server Bind Failed:"); 
    		exit(1); 
  	} 
	return 1;
}

int Serve_Listen() // 监听
{
  	if(-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE))) 
  	{ 
    		perror("Listen error:"); 
    		exit(1); 
  	}
	return 1; 
   
}
int Serve_Accept_Link()// 接受连接请求，返回一个新的socket(描述符)，这个新socket用于同连接的客户端通信
{
    	new_server_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_length);     // accept函数会把连接到的客户端信息写到client_addr中 
    	if(new_server_socket_fd < 0) 
    	{ 
      		perror("Server Accept Failed:"); 
    		exit(0);
    	} 
	return 1;
}

int Send_File_Name()  // 接收需要发送的文件位置及文件名
{
 	// recv函数接收数据到缓冲区buffer中 
    	bzero(buffer, BUFFER_SIZE); 
    	if((rc = recv(new_server_socket_fd, buffer, BUFFER_SIZE, 0)) < 0) { 
      		perror("Accept file error"); 
      		exit(0); 
    	} 
   	// 然后从buffer(缓冲区)拷贝到file_name中 
    	bzero(file_name, FILE_NAME_MAX_SIZE+1); 
    	strncpy(file_name, buffer, strlen(buffer)>FILE_NAME_MAX_SIZE?FILE_NAME_MAX_SIZE:strlen(buffer)); 
    	printf("The file name which you want to send:%s\n", file_name); 
	return 1;
}

int Document_Send()	// 发送文件
{
   	// 打开文件并读取文件数据 
    	fp = fopen(file_name, "r"); 
    	if(fp == NULL) 
    	{ 
      		printf("File:%s Not Found\n", file_name); 
    	} 
    	else 
    	{ 
      		bzero(buffer, BUFFER_SIZE); 
		// 每读取一段数据，便将其发送给客户端，循环直到文件读完为止 
      		while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) 
      		{ 
        		if(send(new_server_socket_fd, buffer, length, 0) < 0) 
        		{ 
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

int Link_Trans()
{
    int opt = 1;
    setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    maxfd = server_socket_fd;   // 最大文件描述符
    FD_ZERO(&allset); // 清空监听集合
    FD_SET(server_socket_fd, &allset);  // 将待监听fd添加到监听集合中
    while (true) {
        rset = allset;  // 备份
        ret = select(maxfd+1, &rset, nullptr, nullptr, nullptr);  // 使用select监听
        if (ret < 0) {
            std::cout << "select error. errno=" << errno 
                << "errmsg:" << strerror(errno) << std::endl;
            exit(0);
        }
        if (FD_ISSET(server_socket_fd,&rset)) { // listenfd满足监听的读事件
            // clie_addr_len = sizeof(clie_addr);
            // new_server_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_length);// 建立链接-不会阻塞
            Serve_Accept_Link();
            FD_SET(new_server_socket_fd, &allset); // 将新产生的fd添加到监听集合中，监听数据的读事件
            if (maxfd < new_server_socket_fd) maxfd = new_server_socket_fd; // 修改maxfd
            if (ret == 1) continue;         // 说明select只返回一个，并且是listenfd
        }

        for (int i = server_socket_fd; i <= maxfd; ++i) { // 处理满足读事件的fd
            if (FD_ISSET(i,&rset)) {    // 找到满足读事件的fd
                // rc = recv(new_server_socket_fd, buffer, sizeof(buffer), 0);
                Send_File_Name();
                if (rc == 0) {  // 检测到客户端已关闭连接
                    close(i);
                    FD_CLR(i, &allset);
                    continue;
                } else if (rc == -1) {  // 将关闭的fd移除出监听集合
                    std::cout << "recv error. errno=" << errno 
                        << " errmsg:" << strerror(errno) << std::endl;
                    continue;
                }
                Document_Send();
            }
        }
    }
	return 1;
}


int main(void) 
{ 
	Start_Server_Socket();
	Creat_Socket();
	Client_Server_Bind();  
	Serve_Listen();   
	Link_Trans();
   	close(server_socket_fd);    // 关闭监听用的socket
  	return 0; 
}
