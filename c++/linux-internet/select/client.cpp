    /*============================================================================= 
    #     FileName: tcpcliselect.c 
    #         Desc: send data to server and receive data from server 
    #       Author: Licaibiao 
    #   LastChange: 2017-02-12  
    =============================================================================*/  
    #include<stdio.h>    
    #include<sys/types.h>    
    #include<sys/socket.h>    
    #include<unistd.h>    
    #include<stdlib.h>    
    #include<errno.h>    
    #include<arpa/inet.h>    
    #include<netinet/in.h>    
    #include<string.h>    
    #include<signal.h>  
    #define MAXLINE   1024  
    #define LISTENLEN 10  
    #define SERV_PORT 6666  
      
    int max(int a, int b)  
    {  
        return a>b ? a : b;  
    }  
      
    void str_cli(FILE *fp, int sockfd)  
    {  
        int         maxfdp1, stdineof;  
        fd_set      rset;  
        char        buf[MAXLINE];  
        int     n;  
      
        stdineof = 0;  
        FD_ZERO(&rset);  
        for ( ; ; )   
        {  
            if (stdineof == 0)  
                FD_SET(fileno(fp), &rset);  
            FD_SET(sockfd, &rset);  
            maxfdp1 = max(fileno(fp), sockfd) + 1;  
            select(maxfdp1, &rset, NULL, NULL, NULL);  
      
            if (FD_ISSET(sockfd, &rset))   
            {     
                if ( (n = read(sockfd, buf, MAXLINE)) == 0) /* socket is readable */  
                {  
                    if (stdineof == 1)  
                        return;     /* normal termination */  
                    else  
                        printf("str_cli: server terminated prematurely");  
                }  
                write(fileno(stdout), buf, n);  
            }  
      
            if (FD_ISSET(fileno(fp), &rset))  /* input is readable */  
            {    
                if ( (n = read(fileno(fp), buf, MAXLINE)) == 0)   
                {  
                    stdineof = 1;  
                    shutdown(sockfd, SHUT_WR);  /* send FIN */  
                    FD_CLR(fileno(fp), &rset);  
                    continue;  
                }  
      
                write(sockfd, buf, n);  
            }  
        }  
    }  
      
    int main(int argc, char **argv)  
    {  
        int sockfd;  
        struct sockaddr_in  servaddr;  
      
        if (argc != 2)  
        {  
            printf("usage: tcpcli <IPaddress>");  
            exit(0);  
        }  
        sockfd = socket(AF_INET, SOCK_STREAM, 0);  
      
        bzero(&servaddr, sizeof(servaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_port = htons(SERV_PORT);  
        inet_pton(AF_INET, argv[1], &servaddr.sin_addr);  
      
        connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));  
      
        str_cli(stdin, sockfd);     /* do it all */  
      
        exit(0);  
    }  
