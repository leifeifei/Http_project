/*
author:lff
date:2016-3-15
function:该程序通过标准socket实现简单Http服务器 
         运行该服务器可以通过浏览器访问服务器目录下的 
         Html文件和jpg图片 完成初步的Http服务器功能 
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


#define SERVER_STRING "Server: LFF_Service"  

void accept_request(int clientfd);         //处理从套接字上监听到的一个 HTTP 请求
void request_message(int clientfd,char *); //在这里可以很大一部分地体现服务器处理请求流程。
int socket_creat_bind_listen();           //初始化 httpd 服务，包括建立套接字，绑定端口，进行监听等。
void error_request(int clientfd);          //返回给客户端这是个错误请求，HTTP 状态吗 400 BAD REQUEST. 
void file_not_found(int clientfd);         //主要处理找不到请求的文件时的情况。
void unimplemented(int clientfd);          //返回给浏览器表明收到的 HTTP 请求所用的 method不被支持。
int read_request_message(char *input,char *method,char* file,char *versions,char *content_type);  //读取请求报文
void response_message(int clientfd, const char *filename,const char *content_type,int file_size); //回应报文
void execute_cgi();


//返回给浏览器表明收到的 HTTP 请求所用的 method不被支持。HTTP 状态码 501 Method Not Implemented.
void unimplemented(int clientfd)  
{  
    char buf[1024];  
  
    /* HTTP method 不被支持*/  
    sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    /*服务器信息*/  
    sprintf(buf, SERVER_STRING);  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "Content-Type: text/html\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "</TITLE></HEAD>\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "</BODY></HTML>\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
}  

//返回给客户端这是个错误请求，HTTP 状态吗 400 BAD REQUEST.
void error_request(int clientfd)  
{  
    char buf[1024];  
  
    /*回应客户端错误的 HTTP 请求 */  
    sprintf(buf, "HTTP/1.1 400 BAD REQUEST\r\n");  
    send(clientfd, buf, sizeof(buf), 0);  
    sprintf(buf, "Content-type: text/html\r\n");  
    send(clientfd, buf, sizeof(buf), 0);  
    sprintf(buf, "\r\n");  
    send(clientfd, buf, sizeof(buf), 0);  
    sprintf(buf, "<P>Your browser sent a bad request, ");  
    send(clientfd, buf, sizeof(buf), 0);  
    sprintf(buf, "such as a POST without a Content-Length.\r\n");  
    send(clientfd, buf, sizeof(buf), 0);  
}  

//主要处理找不到请求的文件时的情况 HTTP 状态吗 404 NOT FOUND.
void file_not_found(int clientfd)  
{  
    char buf[1024];  
  
    /* 404 页面 */  
    sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    /*服务器信息*/  
    sprintf(buf, SERVER_STRING);  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "Content-Type: text/html\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "your request because the resource specified\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "is unavailable or nonexistent.\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    sprintf(buf, "</BODY></HTML>\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
}  

void cannot_execute_CGI(int clientfd)
{
    char buf[1024];

    /* 回应客户端 cgi 无法执行*/
    sprintf(buf, "HTTP/1.1 500 Internal Server Error\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(clientfd, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(clientfd, buf, strlen(buf), 0);
}

//回复报文
void response_message(int clientfd, const char *file,const char *content_type,int file_size)  
{  
    char buf[1024];   
    /*正常的 HTTP header */  
    strcpy(buf, "HTTP/1.1 200 OK\r\n");  
    send(clientfd, buf, strlen(buf), 0);  
    /*服务器信息*/  
    strcpy(buf, SERVER_STRING);  
    send(clientfd, buf, strlen(buf), 0); 

    sprintf(buf, "Content-Type: %s\r\n",content_type);  
    send(clientfd, buf, strlen(buf), 0);  

    sprintf(buf, "Content-Length: %d\r\n",file_size);  
    send(clientfd, buf, strlen(buf), 0);  

    strcpy(buf, "\r\n");  
    send(clientfd, buf, strlen(buf), 0);  

    send(clientfd,file,file_size,0);
    close(clientfd);
}  

void execute_cgi(int clientfd,char *method,char *query_string,char *path)
{
    char buf[1024];
    int CGI_output[2];
    int CGI_input[2];
    pid_t pid;
    strcpy(buf, "HTTP/1.1 200 OK\r\n");  
    send(clientfd, buf, strlen(buf), 0);

    //创建管道  
    if(pipe(CGI_output) < 0)
    {
        cannot_execute_CGI(clientfd);
        return;
    }
    if(pipe(CGI_input) < 0)
    {
        cannot_execute_CGI(clientfd);
        return;
    }
    if(pid = fork() < 0)
    {
       cannot_execute_CGI(clientfd);
        return;
    }
    
    if(pid == 0)
    {
        char method_env[30];
        char query_string_env[30];

        // 把 STDOUT 重定向到 cgi_output 的写入端 
        dup2(CGI_output[1],1);

        //把 STDIN 重定向到 cgi_input 的读取端
        dup2(CGI_input[0],0);

        //关闭 cgi_input 的写入端 和 cgi_output 的读取端
        close(CGI_output[0]);
        close(CGI_input[1]);

        //设置 request_method 的环境变量
        sprintf(method_env,"REQUEST_METHOD=%s",method);
        putenv(method_env);

        //设置 query_string 的环境变量
        sprintf(query_string_env, "QUERY_STRING=%s", query_string);
        putenv(query_string_env);

        //用 execl 运行 cgi 程序
        execl(path, path, NULL);
        exit(0);
    }
    if(pid > 0)
    {
        //关闭 cgi_input 的读取端 和 cgi_output 的写入端
        close(CGI_output[1]);
        close(CGI_input[0]);
        
        //读取 cgi_output 的管道输出到客户端，该管道输入是 STDOUT
        bzero(buf,sizeof(buf));
        if(read(CGI_output[0],buf,sizeof(buf)-1) > 0)
        {
           send(clientfd,buf,strlen(buf),0);
        }

        //关闭管道
        close(CGI_input[1]);
        close(CGI_output[0]);

        //等待子进程结束
        wait(NULL);
    }

}

//分别读取请求报文的每一部分 请求类型 请求文件 版本号 及文件类型
int read_request_message(char *input,char *method,char* url,char *versions,char *content_type)
{
	char* tmp = input;                 //定义一个临时变量指针

    sscanf(tmp,"%[^ ]",method);      //读取请求类型
    printf("method:%s\n",method);
    tmp = tmp + strlen(method) + 2;

    sscanf(tmp,"%[^ ]",url);        //读取请求文件
    printf("url:%s\n",url);
    tmp = tmp + strlen(url) + 1;

    sscanf(tmp,"%[^A]",versions);    //读取版本号
    printf("versions:%s\n",versions);
    tmp = tmp + strlen(versions);

    sscanf(tmp,"%*[^ ]%*c%[^,]",content_type);   //读取文件类型
    printf("content_type:%s\n",content_type); 
}

//解析请求报文
void request_message(int clientfd,char *input)
{
	char *method = (char *)malloc(5);    //malloc(5)是开辟了5个byte。
	char *url = (char *)malloc(20);
	char *versions = (char *)malloc(20);
    char *content_type = (char *)malloc(15);
    char *query_string = NULL; 
    char *file = (char *)malloc(20);
    char *path = (char *)malloc(20);
	read_request_message(input,method,url,versions,content_type);

    if (strcmp(method, "GET") != 0)    //请求类型判断，若不是GET则，返回客户端该请求类型不支持
    {  
        unimplemented(clientfd);  
        return;  
    } 

    int i,j;
    query_string = url;
    while(*query_string != '?' && *query_string != '\r' && *query_string != '\n')
    {
        query_string++;
    }
    if(*query_string == '?')
    {
        *query_string = '\0';
        query_string++;

        //说明是CGI文件
        sprintf(path, "CGI%s", url);
        if (path[strlen(path) - 1] == '/')
        {
            strcat(path, "CGI.html");
        }
        execute_cgi(clientfd,method,query_string,path);
    }
    else
    {
        sprintf(file, "%s", url);
        struct stat st;
        int ret = stat(file,&st);           //判断请求文件是否存在，若不存在，返回客户端该文件不存在
        if(ret == -1)
        {
          file_not_found(clientfd);
          return;
       }
       int file_size = st.st_size;        //若该文件大小小于零，则返回客户端这是个错误请求
       if(file_size<0)
       {
          error_request(clientfd);
          return;
       }

      int fd = open(file,O_RDONLY);
      if(fd == -1)
      {
        perror("open:");
        return;
      }
    
      ret = read(fd,file,file_size);
      if(ret < 0)
      {
        perror("read:");
        return;
      }
      response_message(clientfd,file,content_type,file_size);
       close(fd);
   }  
  
}

////处理从套接字上监听到的一个 HTTP 请求
void accept_request(int  clientfd)
{
    int recv_bytes;  
    char buf[1024]; 
    if (( recv_bytes = recv(clientfd, buf, sizeof(buf)-1, 0)) == -1)  
    {  
        perror("recv:");  
        exit(1);  
    }
    printf("recv_bytes=%d\n", recv_bytes);
    buf[recv_bytes] = '\0';
    printf("recv=%s\n",buf);
    request_message(clientfd,buf); 
}

//初始化 http 服务 创建，绑定，监听套接字
int socket_creat_bind_listen()
{
	int curr_socket = socket(AF_INET,SOCK_STREAM,0);
    if(curr_socket == -1)
    {
    	perror("curr_socket:");
    	exit(-1);
    }

    struct sockaddr_in seraddr;        // 服务器地址结构体  
    bzero(&seraddr,sizeof(seraddr));

    //填充服务器连接信息 
    seraddr.sin_family=AF_INET;
    seraddr.sin_port=htons(6789);
    seraddr.sin_addr.s_addr=inet_addr("192.168.201.3");
	
    //绑定服务
	if(bind(curr_socket,(struct sockaddr*)&seraddr,sizeof(seraddr)) == 1)
	{
		perror("bind fail:");
		exit(-1);
	}
    
    //开始监听 
    if(listen(curr_socket,10) == -1)    //套接字 LISTENT状态
    {
    	perror("listen fail:");
    	exit(-1);
    }
    return curr_socket;
}

int main(int argc, char const *argv[])
{
	int curr_socket=socket_creat_bind_listen();     // 创建服务器端的socket
	pthread_t pthread;  
	struct sockaddr_in clientaddr;
    int addrlen;
    while(1)
    {

       int clientfd = accept(curr_socket,(struct sockaddr*)&clientaddr,&addrlen);    // 接收连接  
       if(clientfd<0)
       {
           perror("clientfd:");
           exit(-1);
       }  
       printf("IP:%s\tport:%d\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

       //派生新线程用 accept_request 函数处理新请求 
       int ret = pthread_create(&pthread,NULL,(void*)accept_request,(void*)clientfd);
       while(ret != 0)
       {
        ret = pthread_create(&pthread,NULL,(void*)accept_request,(void*)clientfd);
       }
       pthread_join(pthread,NULL);
    }
	return 0;
}