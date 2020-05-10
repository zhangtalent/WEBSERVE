#include <iostream>
using namespace std;
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_PORT 12000
#define BUFFER_SIZE 1024

struct client_thread{
	int clifd;
	int retval;
	struct sockaddr_in c_addr;
};

//读取字节流
int ReadStream(int fd,char* path);
//发送字节流
void WriteStream(int fd,char *c);
//获取文件大小
int file_size(char *path);
//暂无
char* getPath(char* data);

//线程池
pthread_t tid[1024];
//线程参数
struct client_thread cts[1024];
int cur_i = 0;

//处理链接
void* process_client(void* arg){
	//接收参数
	struct client_thread *carg = (struct client_thread*)arg;
	char addr[1024];
	int clientfd = carg->clifd;
	//网路字节转化
	inet_ntop(AF_INET,&(carg->c_addr).sin_addr.s_addr,addr,1024);
	//cout << "client addr:" << addr << " port:" << ntohs(carg->c_addr.sin_port) << endl;
	//循环建立链接
	while(1){
		//web浏览器接收需要文件命
		char *pp = (char *)malloc(1024*sizeof(char));
		cout << pp << endl;
		bzero(pp,sizeof(pp));
		//返回读取的字节数
		int len = ReadStream(clientfd,pp);
		//是否存在
		int rok = access(pp,R_OK);
		
		//如果读取为0或出错，那么结束这个链接
		if(len <= 0){
			close(clientfd);
			cout << "disconnect socket" << endl;
                        carg->retval = 0;
			pthread_exit(0);
			break;
                }
		
                	
                char *path = pp;
                //不存在放回404
		if(rok!=0){
			//cout << "not found" << endl;
			path = (char*)"404.html";
			if(pp[0]=='\0')path = (char*)"index.htm";
		}
		//文件的大小
		int lens = file_size(path)+1;
		//计算长度得位数
		int slen = 1;
                while(lens/10>0){
	                lens /= 10;
	                slen++;
	        }

		//获取大小
		int size = file_size(path);
	        
		//Response头部
		char *header = (char *)"HTTP/1.1 200 OK\r\ncontent-type: \r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length:\r\n\r\n";
		//类型
		char *typec = (char*)"text/html;";

		//后缀名
		char tt[1024];
		// 检查后缀
		int off = 0;
		int dotpos = 0;
		while(path[off]!='\0'){
			if(path[off]=='.')dotpos=off;
			off++;
		}
		cout << "path:::" << path << endl;
		//后缀长度
		int wid = 0;
		int pps = 0;
		off = dotpos;
		memset(tt,'\0',sizeof(tt));
		while(path[off]!='\0'){
			if(path[off]!='.'){
				tt[wid]=path[off];
				wid++;			
			}
			off++;
		}
		cout << wid << " " << tt << endl;
			
		if(strcmp(tt,"gif")==0||strcmp(tt,"jpg")==0||strcmp(tt,"png")==0){
			//cout << "picture" << endl;
			typec = (char*)"image/*;";
		}

		//先输出头部信息
		char html[strlen(typec)+strlen(header)+slen+1];
		snprintf(html,sizeof(html),"HTTP/1.1 200 OK\r\ncontent-type: %s\r\nDate: Mon, 27 Jul 2009 12:28:53 GMT\r\nServer: Apache/2.2.14 (Win32)\r\nLast-Modified: Wed, 22 Jul 2009 19:15:56 GMT\r\nContent-Length:%lu\r\n\r\n",typec,size);
		WriteStream(clientfd,html);
		
		
		//输出文件内容
		int ffd = open(path,O_RDONLY);
		int offset = 0;
		char datas[1024];
	
		while((offset = read(ffd,datas,1024))){
			write(clientfd,datas,offset);
			//cout << datas << endl;
			if(offset<1024){
				break;
			}
		}

	}
}


int main(){
	int serverfd,clientfd;
	struct sockaddr_in server_addr,client_addr;
	socklen_t server_len,client_len;
	int ret;

	//配置bind结构体，服务器地址，本地
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	//cout << "port" << endl;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//绑定宽口
	//cout << "pton" << endl;
	serverfd = socket(AF_INET,SOCK_STREAM,0);
	if(serverfd == -1){
		cout << "socket create fail" << endl;
		exit(1);
	}

	server_len = sizeof(server_addr);
	
	ret = bind(serverfd,(struct sockaddr*)&server_addr,server_len);
	if(ret == -1){
		cout << "bind fail:" << errno  << endl;
	}
	
	//监听
	ret = listen(serverfd,100);
	
	//循环accept
	while(1){
		client_len = sizeof(client_addr);
		bzero(&client_addr,sizeof(client_addr));
		clientfd = accept(serverfd,(struct sockaddr*)&client_addr,&client_len);
		

		//多线程连接
		while(cts[cur_i].retval == 1){
			cur_i = (cur_i+1)%1024;
		}
		
		cts[cur_i].clifd = clientfd;
		cts[cur_i].retval = 1;
		cts[cur_i].c_addr = client_addr;
		
		pthread_create(&tid[cur_i],NULL,process_client,(void*)&cts[cur_i]);
		//线程分离
		pthread_detach(tid[cur_i]);
		
	}
	return 0;
}
int ReadStream(int fd,char* path){
	
	int lenOfData = 0;
	int readlen = 0;
	char c[BUFFER_SIZE];
	int first = 0;
	memset(path,'\0',sizeof(path));
	cout << "before:" << path << endl;
	while((readlen = read(fd,c,BUFFER_SIZE))){
		lenOfData += readlen;
		if(first == 0){
			int i=0;
			int offset = 0;
			int no=0;
			while(no<2){
				if(no==1&&c[i]!=' '&&c[i]!='/'){
					path[offset] = c[i];
					offset++;
				}
				if(c[i]==' '){
					no++;
				}
				i++;
			}
			first = 1;
		}
		if(readlen<1024)break;
	}
	cout << "later:" << path << endl;
	return lenOfData;
}
void WriteStream(int fd,char *data){
	int lenOfData = strlen(data);
	int writelen = 0;
	int alreadylen = 0;
	int leavelen = lenOfData;
	while(lenOfData>alreadylen){
		int wlen = leavelen>=1024?1024:leavelen;
		writelen = write(fd,data,wlen);
		leavelen -= wlen;
		//cout << "write :" << writelen  << endl;
		alreadylen += writelen;		
	}
	//cout << "write over" << endl;
		
}

int file_size(char *path){
	struct stat sbuf;
	stat(path,&sbuf);
	int size = sbuf.st_size;
	return size;
}

char* getPath(char* data){
	cout << "ddd" << data << endl;
	char c[1024];
	int i=0;
	int offset = 0;
	int no=0;
	while(no<2){
		if(no==1&&data[i]!=' '){
			c[offset] = data[i];
		        offset++;	
		}
		if(data[i]==' '){
			no++;
		}
		i++;	
	}
	cout << c << "ddd" << endl;
	return c;
}
