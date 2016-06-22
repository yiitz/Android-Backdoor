#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netdb.h>

int download(const char *remote,const char *local);
const char* update_script = "./xrhupdate.sh";
const char* version_file = "./xrhversion";
int main(int argc, char *argv[])
{
	int r;
	int version = 1;
	char buf[64];
	FILE *fp;
	int childpid;
	if(3 == argc)
	{
		//download file
		return download(argv[1],argv[2]);
	}else
	{
		//check update
		printf("check update begin.\n");
		while(1)
		{
			if((fp=fopen(version_file,"r"))!=NULL){
				fscanf(fp,"%d",&version);
				fclose(fp);
			}
			printf("current version:%d.\n",version);
			sprintf(buf,"files/%d/update.sh",version+1);
			r = download(buf,update_script);
			if(0 == r){
				printf("new version:%d download ok.\n",version+1);
				
				/*
				if (fork() == 0){
					//child process
					if(execl("/system/bin/sh",update_script,NULL)<0)
					{
						perror("error on execl.\n");
						exit(0);
					}
				}else{
					//parent process  
					wait(&childpid);  
				}
				*/
				chmod(update_script,S_IRUSR|S_IWUSR|S_IXUSR);
				r = system(update_script);
				if(0 != r)
				{
					sleep(60*3);
				}else if((fp=fopen(version_file,"w"))!=NULL){
					fprintf(fp,"%d",version+1);
					fclose(fp);
				}
			}else{
				sleep(60*60*4);
			}
		}
	}
	return 0;
}

int download(const char *remote,const char *local)
{
	int    sockfd;
    char    recv_buf[4096];
    struct sockaddr_in    servaddr;
	struct hostent *nlp_host;
	uint32_t size,receive_size;
	FILE *localfp;
	int len;
	int result = 0;
	
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    printf("create socket error: %s(errno: %d).\n", strerror(errno),errno);
    return -1;
    }
	if ((nlp_host=gethostbyname("kuanglizhong.com"))==NULL){
		printf("resolve error!\n");
	result = -1;
        goto download_end;
	}
    memset(&servaddr, 0, sizeof(servaddr));
	memcpy(&servaddr.sin_addr, nlp_host->h_addr_list[0], nlp_host->h_length);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5030);

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
    printf("connect error: %s(errno: %d).\n",strerror(errno),errno);
	result = -1;
        goto download_end;
    }
	
	strcpy(recv_buf,remote);
    strcat(recv_buf,":");
	if( send(sockfd, recv_buf, strlen(recv_buf), 0) < 0)
    {
    printf("send msg error: %s(errno: %d).\n", strerror(errno), errno);
	result = -1;
        goto download_end;
    }
	
	len = recv(sockfd, recv_buf, 4, 0);
	if(len != 4){
        printf("receive size error, len:%d.\n",len);
	result = -1;
        goto download_end;
	}
	
	size = *((uint32_t*)recv_buf);
	
	if(1234567 == size){
        printf("error,file not found.\n");
	result = -2;
        goto download_end;
	}
    printf("file size %d.\n", size);
	
	if((localfp=fopen(local,"wb"))==NULL)
    {
        printf("cannot open file.\n");
	result = -1;
        goto download_end;
    }
	receive_size = 0;
	while(1){
		len = recv(sockfd, recv_buf, sizeof(recv_buf), 0);
		if(len < 0){
			break;
		}
		receive_size += len;
		if(1 != fwrite(recv_buf,len,1,localfp)){
        printf("write file error.\n");
	result = -1;
	fclose(localfp);
        goto download_end;
		}
		if(receive_size == size){
			break;
		}
	}
	
	fclose(localfp);
    printf("download ok,receive size: %d.\n", receive_size);
	if(size != receive_size){
        printf("receive error,please delete file.\n");
	result = -1;
	}
download_end:
    close(sockfd);
	return result;
}