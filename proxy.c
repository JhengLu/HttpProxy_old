#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "proxy_parse.h"
#include <sys/wait.h>


// IPv4 AF_INET sockets:
/*
struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET, AF_INET6
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};
*/

int main(void)
{
    
    int port;
    scanf("%d", &port);
    printf("port:%d\n",port);
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //参数：TCP/IP – PF_INET:IPv4;SOCK_STREAM:面向连接可靠方式,比如TCP;对于TCP参数可指定为IPPROTO_TCP,对于UDP可以用IPPROTO_UDP,使用0则根据前两个参数使用默认的协议
    // socket handle is socketFD
    if (-1 == SocketFD)
    {
        perror("can not create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(port); // set the port number  //htons(), htonl():'host'to'network'short/long'
    stSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);// bind to any local address

    if (-1 == bind(SocketFD, (const struct sockaddr*)&stSockAddr, sizeof(struct sockaddr_in)))
        //bind() associates the socket with its local address 
        //that's why server side binds, so that clients can use that address to connect to server.
    {
        perror("error bind failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(SocketFD, 100))// indicate how many connection requests can be queued
    {
        perror("error listen failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    int process_num = 0;
    int status = 0;
    for (;;)
    {      
        socklen_t size = sizeof(struct sockaddr_in);
        int ConnectFD = accept(SocketFD, (struct sockaddr*)&stSockAddr, &size);//accept来建立连接
            
        
        pid = fork();
        printf("processilove:%d\n",process_num);
        printf("iloveu\n");
        
        if (pid>0){
            sleep(1);
            //waitpid(-1,&status,WNOHANG|WUNTRACED|WCONTINUED);
            waitpid(-1, NULL, 0);
            if(WIFEXITED(status)){
                printf("status = %d\n",WEXITSTATUS(status));
            }
        }
        else if(pid == 0){
            close(SocketFD);//子进程让监听socket的计数减1, 并非直接关闭监听socket
            process_num++;
            printf("process:%d\n",process_num);
            
            if (0 > ConnectFD)
            {
                perror("error accept failed");
                close(SocketFD);
                exit(EXIT_FAILURE);
            }
            //接受命令
            char* buf = NULL;
            char recvline[1024];
            buf = recvline;
            ssize_t t = read(ConnectFD,buf,1024); // returns the number of bytes actually read and placed in buf
            buf[t] = '\0';
            char str1[] = "\r\n";
            strcat(buf,str1);
            printf("%s\n",buf);
            
            struct ParsedRequest* req = ParsedRequest_create();
            if (ParsedRequest_parse(req, buf, sizeof(recvline)) < 0) {
                printf("parse failed\n");
                return -1;
            }
            if(req->port!=NULL)
            printf("Port:%s\n",req->port);
            printf("Method:%s\n", req->method);
            printf("Host:%s\n", req->host);
            
            
            /* perform read write operations ... */
            printf("successful connection\n");

            //给远端服务器发送请求
            int socket_desc;
            struct sockaddr_in server;
            socket_desc = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_desc == -1) {
                printf("Could not create socket");
            }
            char ip[20] = { 0 };
            char* hostname = req->host;
            struct hostent* hp;
            if ((hp = gethostbyname(hostname)) == NULL) {
                return 1;
            }
            strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr_list[0]));
            server.sin_addr.s_addr = inet_addr(ip);
            server.sin_family = AF_INET;
            server.sin_port = htons(80);
            if(connect(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) { // connect!
                printf("Cannot connect\n");
                return -1;
            } 

            char message [] = "GET / HTTP/1.0\r\nHost:";
            strcat(message,hostname);
            strcat(message,"\r\n");
            /*char header[1000];
            ParsedRequest_unparse_headers(req,header,1000);
            printf("header:");
            printf("%s\n",header);*/
            //strcat(message,header);
            strcat(message,"\r\n");

            if (send(socket_desc, message, strlen(message), 0) < 0) {
                puts("Send failed");
                return 1;
            }
            puts("Data Send\n");
            int size_recv, total_size = 0;
            char chunk[512];
            while (1) {
                memset(chunk, 0, 512); //clear the variable
                size_recv = recv(socket_desc, chunk, 512, 0);
                if (size_recv == -1)
                {
                    printf("recv error/n");

                }
                else
                {
                    total_size += size_recv;
                    printf("%s", chunk);
                }
            write(ConnectFD, chunk, 512);
            
            }
            close(ConnectFD);
            return 0;
        }


    close(SocketFD);
    return 0;
    }
}
