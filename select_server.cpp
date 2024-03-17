#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
   
#define SERVER_PORT 8080 // 端口
#define LENGTH_OF_LISTEN_QUEUE 1024 // 监听上限
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

fd_set rdset; // 读集合
int nready = 0, fdsize = 0, rc;
int fds[1024] = {0}; // 存放需要监听的fd数组

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
  	if(server_socket_fd < 0) { 
    		perror("Create socket error:"); 
    		exit(1); 
  	} 
  	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
	return 1;
}

int Client_Server_Bind() // 绑定客户端的socket和客户端的socket地址结构
{
  	if(-1 == (bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)))) { 
    		perror("Server Bind Failed:"); 
    		exit(1); 
  	} 
	return 1;
}

int Serve_Listen() // 监听
{
  	if(-1 == (listen(server_socket_fd, LENGTH_OF_LISTEN_QUEUE))) { 
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
    	std::cout << "The file name which you want to send:" << file_name << std::endl; 
	return 1;
}

int Document_Send()	// 发送文件
{
   	// 打开文件并读取文件数据 
    	fp = fopen(file_name, "r"); 
    	if(fp == NULL) std::cout << "File:" << file_name << " Not Found\n"; 
    	else 
    	{ 
      		bzero(buffer, BUFFER_SIZE); 
		// 每读取一段数据，便将其发送给客户端，循环直到文件读完为止 
      		while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0) { 
        		if(send(new_server_socket_fd, buffer, length, 0) < 0) { 
          			std::cout << "Send File:" << file_name << " Failed./n"; 
          			exit(0); 
        		} 
        		bzero(buffer, BUFFER_SIZE); 
      		} 
      		// 关闭文件 
      		fclose(fp); 
      		std::cout << "File:" << file_name << " send success" << std::endl; 
    	} 
	return 1;
   
}

int Link_Trans()
{

	fds[fdsize++] = server_socket_fd; // 把lfd放入到fds数组的[0]位置
	while (true) {
		FD_ZERO(&rdset); // 将读集合清空
		for (int i = 0; i < fdsize; i++) FD_SET(fds[i], &rdset); // 将fds数组加入到等待队列
		if (fdsize < 0 || fdsize > 1024) {
			std::cout << "Too many clients!" << std::endl;
			exit(1);
		}
		nready = select(fds[fdsize-1] + 1, &rdset, nullptr, nullptr, nullptr);// 使用select监听
		if (nready == -1) {
			std::cout << "select error!" << strerror(errno) <<  std::endl;
			for(int i = 1; i < fdsize ; ++i) close(fds[i]);
			break;
		} else if (nready == 0) continue;; // 超时

		for (int i = 0; i < fdsize; i++) {
			if (FD_ISSET(fds[i], &rdset)) {
				if (fds[i] == server_socket_fd) { // 有新连接
					Serve_Accept_Link();
					if (fdsize >= 1024) {
						std::cout << "已经达到最大检测数1024。" << std::endl;
						continue;
					} else fds[fdsize++] = new_server_socket_fd;
				} else {
					Send_File_Name();
					if (rc == -1) {
						std::cout << "recv error!" << std::endl;
						exit(1);
					} else if (rc == 0) { // 客户端断开连接
						close(fds[i]);
						std::cout << "client:" << fds[i] << "closed connection" << std::endl;
						for(int j = i--; j < fdsize -1; ++j) fds[j] = fds[j + 1];
						fds[--fdsize] = 0;
					} else Document_Send();
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
