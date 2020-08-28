#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "sys/select.h"	//select()
#include "sys/time.h"	//
#include "sys/socket.h"	//socket() bind()
#include "sys/types.h"
#include "netdb.h"	//gethostbyname()
#include "unistd.h"	//read() write()
#include "netinet/in.h"	//inet_addr()
#include "arpa/inet.h"
#include "errno.h"	//errno
#include "netinet/tcp.h"

#include "pthread.h"


int fd;
char pth_flag = 1;
char sock_status = 0;
pthread_t ptr_read;

int Network_Connect(int *fd,char *hostname,int port)
{

	struct sockaddr_in addr;
	struct hostent *host = NULL;
	char ipaddr[20];
	int opt = 1;
	struct linger m_linger;
	int socketfd;
	int rc;

	if(hostname == NULL || port == 0)
	{
		printf("network parament error\r\n");
		return -1;
	}

	host = gethostbyname(hostname);  //resolve domain
	if(host == NULL)
	{
		printf("resolve domain fail\r\n");
		return -1;
	}
	else
	{
		strcpy(ipaddr,inet_ntoa(*(struct in_addr *)host->h_addr));
		printf("first addr is %s\r\n",ipaddr);
	}

	socketfd = socket(AF_INET,SOCK_STREAM,0);

	if(socketfd < 0)
	{
		printf("socketfd error fd is %d\r\n",socketfd);
		return -1;
	}
	else
		*fd = socketfd;

	if((rc = setsockopt(socketfd,IPPROTO_TCP,TCP_NODELAY,(void *)(&opt),sizeof(int))) == -1)	//disable nagle algorithm support to send small packet
	{
		printf("setsocket TCP_NODELAY err errno is %d\r\n",errno);
	//	return -1;
	}

	m_linger.l_onoff = 1;
	m_linger.l_linger = 0;	//timeout 0
	if(setsockopt(socketfd,SOL_SOCKET,SO_LINGER,(void*)&m_linger,sizeof(m_linger)) == -1)	//close TIME_WAIT and clearn send buff
	{
		printf("setsocket linger err errno is %d\r\n",errno);
	//	return -1;
	}

	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ((struct in_addr *)host->h_addr)->s_addr;
	//addr.sin_addr.s_addr = inet_addr(ipaddr);
	#if 0
	if(inet_pton(AF_INET,ipaddr,&addr.sin_addr) == -1)
	{
		printf("transform error\r\n");
		return -1;
	}
	while(connect(socketfd,(struct sockaddr *)&addr,sizeof(addr)) == -1)
	{
		printf("connect socket fail errno is %d\r\n",errno);
		//return -1;
	}
	#endif

	rc = connect(socketfd,(struct sockaddr *)&addr,sizeof(addr));
	printf("change errno:%d\r\n",errno);
	if(rc > 0)
	{
		if(errno == EINPROGRESS)
			while(1)
			{
				fd_set wfd;
				struct timeval timeout;

				FD_ZERO(&wfd);
				FD_SET(socketfd,&wfd);

				timeout.tv_sec = 0;
				timeout.tv_usec = 5;

				rc = select(socketfd+1,NULL,&wfd,NULL,&timeout);

				switch(rc)
				{
				case -1:
					printf("conncet error:%d\r\n",errno);
					break;
				case 0:
					printf("connectout:%d\r\n",errno);
					break;
				default:
					if(FD_ISSET(socketfd,&wfd))
					{
						int err=-1,err_len=4;

						getsockopt(socketfd,SOL_SOCKET,SO_ERROR,&err,&err_len);

						if(err == 0)
						{
							printf("connetc success\r\n");
							return 0;
						}
						else
						{
							printf("getsocket err:%d\r\n",err);
							close(socketfd);
							return -1;
						}
					}
					else
					{	
						printf("FD_ISSET err");
						close(socketfd);
						return -1;
					}
					break;
				}
			}

		else
		{
			printf("connect err:%d\r\n",errno);
			close(socketfd);
			return -1;
		}
	}
	return 0;
}


void sock_read(void)
{
	fd_set r_fd;
	struct timeval timeout;
	int ret,len;
	char recv_buf[100];

	while(pth_flag)
	{
		FD_ZERO(&r_fd);
		FD_SET(fd,&r_fd);
		memset(&timeout,0,sizeof(struct timeval));
		memset(recv_buf,0,100);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		ret = select(fd+1,&r_fd,NULL,NULL,&timeout);
		
		if(ret < 0)
		{
			//close(fd);
			sock_status = 0;
			printf("select err:%d\r\n",errno);
			break;
		}
		else if(ret == 0)
			continue;
		else
			if(FD_ISSET(fd,&r_fd))
			{
				if((len = read(fd,(void *)recv_buf,100)) < 0)
				{
					//close(fd);
					sock_status = 0;
					printf("read err:%d\r\n",errno);
					break;
				}
				else
					printf("len:%d,string:%s\r\n",len,recv_buf);
			}

	}
}

int main(int argc,char *argv[])
{
	//int fd = 0;
	char *ip = NULL;
	int port = 0,send_len = 0;
	char *str = "hello world!";
//	char read_buf[1024];

//	fd = malloc(sizeof(int));
	ip = argv[1];
	port =atoi(argv[2]);
	

	if(Network_Connect(&fd,ip,port) == -1)
	{
		printf("connect error\r\n");
		return 0;
	}
	sock_status = 1;
	printf("connect success!\r\n");
	if(fd != 0)
	{
	//	send_len = send(fd,(void *)str,strlen(str),0);
		send_len = write(fd,str,strlen(str));
		if(send_len < 0)
		{
			close(fd);
			printf("write error errno is %d\r\n",errno);
			return 0;
		}
	}
	
	//pthread ptr_read;
	pthread_create(&ptr_read,NULL,sock_read,NULL);

	#if 0
	while(1)
	{
		if((len = read(fd,(void *)read_buf,1024)) < 0)
		//if((len = recv(fd,(void *)read_buf,1024,0)) < 0)
		{
			printf("read error\r\n");
			return 0;
		}
		else if(len > 0)
			printf("read is :%s\r\n",read_buf);
		else
			continue;
	}
	#endif
	
	char buff[100];

	while(1)
	{
		if(sock_status == 0)
		{
			pth_flag = 0;
			sleep(1);
			pthread_join(ptr_read,NULL);	//blocking recycle thread
			close(fd);
			return 0;
		}

		printf("please input:\r\n");
		memset(buff,0,sizeof(buff));
		fflush(stdin);

		//scanf("%s",buff);
		fgets(buff,100,stdin);

		if(buff[0] == '\n')
		{
			printf("exit!\r\n");
			sock_status = 0;
		}

		if((send_len = write(fd,buff,strlen(buff))) < 0)
	//	if((send_len = send(fd,buff,strlen(buff),0)) < 0)
		{
			printf("send err:%d\r\n",errno);
			sock_status = 0;
		}
	}
	
	return 0;
}
