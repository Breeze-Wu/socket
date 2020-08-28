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

int sfd ,cfd;
char pth_flag = 1;
char sock_status = 0;
pthread_t pth;

typedef struct{
	unsigned int sockid;
	unsigned char link_status;
} Link_Parament;

Link_Parament Clinet[8];

void read_sock()
{
	#if 0
	//add select() or epoll()
	char recv_buf[100];
	int len;
	while(pth_flag)
	{
		memset(recv_buf,0,sizeof(recv_buf));
		if((len = read(cfd,(void *)recv_buf,100)) < 0)
		{
			close(sfd);
			close(cfd);
			printf("read err\r\n");
			break;
		}
		else if(len == 0)
			continue;
		else
			printf("len:%d string:%s\r\n",len,recv_buf);
	}
	#endif
	fd_set r_fd;
	struct timeval timeout;
	int ret,len;
	char recv_buf[100];
     while(pth_flag)
     {
         FD_ZERO(&r_fd);
         FD_SET(cfd,&r_fd);
         memset(&timeout,0,sizeof(struct timeval));
         memset(recv_buf,0,100);
 
         timeout.tv_sec = 2;
         timeout.tv_usec = 0;
 
         ret = select(cfd+1,&r_fd,NULL,NULL,&timeout);
 
         if(ret < 0)
         {
             sock_status = 0;
             printf("select err:%d\r\n",errno);
             break;
         }
         else if(ret == 0)
             continue;
         else
            if(FD_ISSET(cfd,&r_fd))
      		{
                 if((len = read(cfd,(void *)recv_buf,100)) < 0)
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
	//int sfd ,cfd;
	struct sockaddr arr;
	struct sockaddr_in addr;

	int port = atoi(argv[1]);

	memset(Client,0,sizeof(Link_Parament));

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
	fd_set r_fd;
	struct timeval timeout;
	int ret;
	unsigned char i;

	while(1)
	{
		FD_ZERO(&r_fd);
		FD_SET(sfd,&r_fd);

		timeout.tv_sec = 2;
		timeout.tv_usec = 10000;	//10ms
		ret = select(sfd+1,&r_fd,NULL,NULL,&timeout);

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
				//pthread_t pth;
				//int pth_flag = 1;

				cfd = accept(sfd,(struct sockaddr *)&src_ip,&len);
				if(cfd < 0)
				{
					printf("accept err:%d\r\n",errno);
					//close(sfd);
					continue;
				}

				for(i=0;i<8:i++)
				{
					if(Client[i].sockid == 0)
					{
						Client[i].sockid = cfd;
						client[i].status = 1;		
					}
				}
				printf("src_ip:success!\r\n");
				sock_status = 1;
				pthread_create(&pth,NULL,read_sock,NULL);
				break;
			}
		}
	}
	char buff[100];
	int len ;
	while(1)
	{
		if(sock_status == 0)
		{
			pth_flag = 0;
			sleep(1);
			pthread_join(pth,NULL);
			close(sfd);
			close(cfd);
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
			//pth_flag = 0;
			//sleep(1);
			//pthread_join(pth,NULL);
			//close(sfd);
			//close(cfd);
			printf("exit!\r\n");
			sock_status = 0;
			//return 0;
		}

		if((len = send(cfd,(void *)buff,strlen(buff),0)) < 0)		
		{
			//pth_flag = 0;
			//sleep(1);	//delay 1s
			//pthread_join(pth,NULL);
			//close(sfd);
			//close(cfd);
			printf("send err:%d\r\n",errno);
			sock_status = 0;
			//break;
		}
	}
	return 0;
}
