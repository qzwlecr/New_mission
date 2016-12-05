#include "networking.h"
#include "databases.h"
redisServer* NetworkingInit()
{
    redisServer *s;
    s=(redisServer*)malloc(sizeof(redisServer));
    if((s->listen_fd=socket(PF_INET,SOCK_STREAM,0))<0)\
    {
        free(s);
        return NULL;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(SERVER_DEFAULT_PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(s->listen_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        free(s);
        return NULL;
    }
    if(listen(s->listen_fd,SOMAXCONN)<0)
    {
        free(s);
        return NULL;
    }
    FD_ZERO(&s->all_set);
    FD_SET(s->listen_fd,&s->all_set);
   return s;
}

void NetworkingListenAndAccept(redisServer *s)
{
    static fd_set temp=s->all_set;
    static sockaddr_in peer_addr;
    int connected;
    socklen_t length=sizeof(peer_addr);
    s->ready=select(s->max_fd+1,&temp,NULL,NULL,NULL);
    if(s->ready==0)
        return;
    if(FD_ISSET(s->listen_fd,&temp))
    {
        connected=accept(s->listen_fd,(struct sockaddr*)&peer_addr,&length);
        if(connected==-1)
            return;
        for(int i=0;i<CLIENTS_MAX_NUMBER;++i)
        {
            if(s->clients[i]<0)
            {
                s->clients[i]=connected;
                if(i>s->max_ready)
                    s->max_ready=i;
                break;
            }
        }
        FD_SET(connected,&s->all_set);
        if(--s->ready<=0)
            return;
        for(int i=0;i<=s->max_ready;++i)
        {
            connected=s->clients[i];
            if(connected<0)
                continue;
            if(FD_ISSET(connected,&temp))
                NetworkingRead(connected,s);
        }
    }
    return;
}

void NetworkingRead(int fd,redisServer *s)
{
    static char ibuf[1024*16];
    int size;
    if((size=recv(fd,ibuf,sizeof(ibuf),0))==-1)
    {
        return;
    }
    static char word[1024];
    sscanf(ibuf,"%s",word);
    if(word[0]=='s'||word[0]=='S')
    {
        sscanf(ibuf,"%s",word);
        string s1=word;
        sscanf(ibuf,"%s",word);
        string s2=word;
        CommandSet(&s->data,s1,s2);
    }
    return;
}
