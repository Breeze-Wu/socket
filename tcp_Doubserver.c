#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "sys/select.h"
#include "sys/time.h"
#include "sys/socket.h"
#include "sys/types.h"
#include "netdb.h"
#include "unistd.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "errno.h"
#include "netinet/tcp.h"

#include "pthread.h"

static int sfd ,cfd;
static int max_fd=-1;
static fd_set r_fd;

char pth_flag = 1;
static char sock_status = 1;
pthread_t pth;
pthread_mutex_t mutex_test;

typedef struct{
	unsigned int sockid;
	unsigned char link_status;
} Link_Parament;

Link_Parament Client[8];

void read_sock(Link_Parament *cfd)
{
	//add select() or epoll()
	char recv_buf[100];
	int len = -1;
	memset(recv_buf,0,sizeof(recv_buf));
	
	if((len = read(cfd->sockid,(void *)recv_buf,100)) <= 0)
	{
		pthread_mutex_lock(&mutex_test);	
		cfd->link_status = 0;
		printf("sockid:%d read err\r\n",cfd->sockid);
		FD_CLR(cfd->sockid,&r_fd);
		close(cfd->sockid);
		cfd->sockid = 0;
		pthread_mutex_unlock(&mutex_test);
		return;
	}
	else
		printf("sockid:%d len:%d string:%s\r\n",cfd->sockid,len,recv_buf);
}

void write_sock(Link_Parament *p)
{
	char buff[100],j;
	int len = -1 ;
	while(1)
	{
		if(sock_status == 0)
		{

		//	pthread_mutex_lock(&mutex_test);
			pth_flag = 0;
			sleep(1);
			pthread_join(pth,NULL);
			for(j=0;j<8;j++)
			{
				if(p[j].sockid != 0)
				{
					FD_CLR(p[j].sockid,&r_fd);
					close(p[j].sockid);
					
				}
			}
			memset(p,0,8*sizeof(Link_Parament));		
		//	pthread_mutex_unlock(&mutex_test);
			close(sfd);
			break;
		}

		printf("please input :\r\n");
		memset(buff,0,100);
		fflush(stdin);	//clear buff of input invalid
		//scanf("%s",buff);
		//scanf("%*[^\n]%*c");	//invalid
		fgets(buff,100,stdin);

		if(buff[0]  == '\n')
		{
			printf("exit!\r\n");
			sock_status = 0;
		}
		//printf("addr:%p,p[0].sockid:%d,p[0].link_status:%d\r\n",p,p[0].sockid,p[0].link_status);
		for(j=0;j<8;j++)
			if(p[j].sockid !=0 && p[j].link_status == 1)
				if((len = send(p[j].sockid,(void *)buff,strlen(buff),0)) < 0)		
				{
					pthread_mutex_lock(&mutex_test);
					printf("send err:%d\r\n",errno);
					p[j].link_status = 0;
					FD_CLR(p[j].sockid,&r_fd);
					close(p[j].sockid);
					p[j].sockid = 0;
					pthread_mutex_unlock(&mutex_test);
				}
	}
}

void selec_sock()
{
	struct timeval timeout;
	int ret;
	unsigned char i;
	
	if(max_fd < sfd)
		max_fd = sfd;

	while(pth_flag)
	{
		FD_ZERO(&r_fd);
		FD_SET(sfd,&r_fd);
	
		for(i=0;i<8;i++)
			if(Client[i].sockid != 0 && Client[i].link_status == 1)
				FD_SET(Client[i].sockid,&r_fd);
		
		timeout.tv_sec = 2;			//2s
		timeout.tv_usec = 10000;	//10ms
		ret = select(max_fd+1,&r_fd,NULL,NULL,&timeout);
		//printf("client[0]:%p,client[0].sockid:%d,client[0].link_status:%d,sfd:%d,max_fd:%d\r\n",&Client[0],Client[0].sockid,Client[0].link_status,sfd,max_fd);
		if(ret < 0)
		{
			printf("select err:%d\r\n",errno);
			break;
		}

		else if(ret == 0)
			continue;	//timeout

		else
		{
			if(FD_ISSET(sfd,&r_fd))
			{
				struct sockaddr_in src_ip;
				int len = sizeof(src_ip);

				cfd = accept(sfd,(struct sockaddr *)&src_ip,&len);
				if(cfd < 0)
				{
					printf("accept err:%d\r\n",errno);
					continue;
				}

				for(i=0;i<8;i++)
				{
					if(Client[i].sockid == 0)
					{
						pthread_mutex_lock(&mutex_test);
						Client[i].sockid = cfd;
						Client[i].link_status = 1;
						if(max_fd < cfd)
							max_fd = cfd;
						FD_SET(cfd,&r_fd);
						cfd = 0;
						pthread_mutex_unlock(&mutex_test);
					}
				}
				printf("src_ip:success!\r\n");
			}
			//printf("clinet[0].sockid:%d,client[0].status:%d\r\n",Client[0].sockid,Client[0].link_status);
			//break;
			for(i=0;i<8;i++)
				if(Client[i].sockid != 0 && FD_ISSET(Client[i].sockid,&r_fd))
				{
					//printf("clinet[%d],addr:%p\r\n",i,&Client[i]);
					read_sock(&Client[i]);
				}
		}
	}
}
int main(int argc,char *argv[])
{
	//int sfd ,cfd;
	struct sockaddr arr;
	struct sockaddr_in addr;

	int port = atoi(argv[1]);

	memset(Client,0,8*sizeof(Link_Parament));
	//printf("addr:%p\r\n",Client);
	sfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if(sfd < 0 )
	{
		printf("socket err:%d\r\n",sfd);
		return 0;
	}

	memset(&addr,0,sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(bind(sfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)) == -1)
	{
		printf("bind err:%d\r\n",errno);
		close(sfd);
		return 0;
	}

	if(listen(sfd,128) == -1)
	{
		printf("listen err:%d\r\n",errno);
		close(sfd);
		return 0;
	}
	printf("listen success! port:%d\r\n",port);
	
	//printf("addr:%p,sfd:%d,max_fd:%d\r\n",Client,sfd,max_fd);
	//pthread_t pid1;
	//pthread_mutex_t mutex_test;
	pthread_mutex_init(&mutex_test,NULL);

	pthread_create(&pth,NULL,selec_sock,NULL);
	//pthread_detach(pth);
	//selec_sock();

	write_sock(Client);

	return 0;
}
