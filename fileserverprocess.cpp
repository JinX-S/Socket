#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
   
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
pid_t pid;

char buffer[BUFFER_SIZE];	// 接收数据
char file_name[FILE_NAME_MAX_SIZE+1];  

FILE *fp;


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
    // 设置多路复用
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
    		perror("Listen error!:"); 
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
    	if(recv(new_server_socket_fd, buffer, BUFFER_SIZE, 0) < 0) 
    	{ 
      		perror("accept file error!"); 
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

void catch_child(int signum) {
    while (waitpid(0, nullptr, WNOHANG) > 0);
    return;
}

int Link_Trans()
{
	while (true) {
		Serve_Accept_Link();
		pid = fork();
		if (pid == -1) {
			perror("Create fork error");
			exit(0);
		} else if (pid == 0) {
			Send_File_Name();
			Document_Send();
			close(new_server_socket_fd);
		} else {
			struct sigaction act;
            act.sa_handler = catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            int sret =  sigaction(SIGCHLD, &act, nullptr);
            if (sret != 0) {
                std::cout << "signal error! errno=" << errno 
                    << " errmsg:" << strerror(errno) << std::endl;
                exit(0);
            }
            close(new_server_socket_fd);
            continue;
		}
	}
    // close(new_server_socket_fd); // 关闭与客户端的连接 
	return 1;
}


int main(void) 
{ 
	Start_Server_Socket();
	Creat_Socket();
	Client_Server_Bind();  
	Serve_Listen();   
	Link_Trans();
   	// close(server_socket_fd);    // 关闭监听用的socket
  	return 0; 
}