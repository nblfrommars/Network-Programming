#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PORT 8080
#define BUF_SIZE 1024

void signal_handler(int sig)
{
    wait(NULL);
}

void get_time_string(char *format, char *result)
{
    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(result,100,"%d/%m/%Y",t);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(result,100,"%d/%m/%y",t);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(result,100,"%m/%d/%Y",t);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(result,100,"%m/%d/%y",t);
    }
    else
    {
        strcpy(result,
               "Invalid format");
    }
}

int main()
{
    int listener = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (listener < 0)
    {
        perror("socket()");
        return 1;
    }
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listener,
             (struct sockaddr *)&addr,sizeof(addr)) < 0)
    {
        perror("bind()");
        return 1;
    }
    if (listen(listener, 5) < 0)
    {
        perror("listen()");
        return 1;
    }
    printf("Server listening on port %d...\n",PORT);
    signal(SIGCHLD, signal_handler);
    while (1)
    {
        int client = accept(listener,NULL,NULL);
        if (client < 0)
        {
            perror("accept()");
            continue;
        }
        if (fork() == 0)
        {
            close(listener);
            char buf[BUF_SIZE];
            while (1)
            {
                memset(buf, 0, sizeof(buf));
                int len = recv(client,buf,sizeof(buf) - 1,0);

                if (len <= 0)
                    break;
                buf[len] = 0;

                buf[strcspn(buf, "\r\n")] = 0;

                if (strncmp(buf,"GET_TIME ",9) != 0)
                {
                    char *msg = "Invalid command\n";

                    send(client, msg,strlen(msg),0);
                    continue;
                }
                char *format = buf + 9;

                char result[100];

                get_time_string(format,result);

                strcat(result, "\n");

                send(client,result,strlen(result),0);
            }
            close(client);
            exit(0);
        }
        close(client);
    }

    close(listener);

    return 0;
}