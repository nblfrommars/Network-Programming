#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define BUF_SIZE 1024

void signal_handler(int sig)
{
    int pid = wait(NULL);
    printf("Child process terminated: %d\n", pid);
}
int check_login(char *user, char *pass)
{
    FILE *f = fopen("user.txt", "r");
    if (f == NULL)
    {
        perror("Cannot open users.txt");
        return 0;
    }
    char u[100];
    char p[100];

    while (fscanf(f, "%s %s", u, p) != EOF)
    {
        if (strcmp(user, u) == 0 &&
            strcmp(pass, p) == 0)
        {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}
int main()
{
    int listener = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(listener,(struct sockaddr *)&addr,sizeof(addr)))
    {
        perror("bind() failed");
        close(listener);
        return 1;
    }
    if (listen(listener, 5))
    {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    printf("Server listening on port %d...\n", PORT);
    signal(SIGCHLD, signal_handler);
    while (1)
    {
        int client = accept(listener, NULL, NULL);
        if (client < 0)
        {
            perror("accept() failed");
            continue;
        }
        if (fork() == 0)
        {
            close(listener);
            char buf[BUF_SIZE];
            char user[100];
            char pass[100];
            send(client, "Username: ", 10, 0);
            int len = recv(client,user,sizeof(user) - 1,0);
            if (len <= 0)
            {
                close(client);
                exit(0);
            }
            user[len] = 0;
            user[strcspn(user, "\r\n")] = 0;
            send(client, "Password: ", 10, 0);
            len = recv(client,pass,sizeof(pass) - 1,0);
            if (len <= 0)
            {
                close(client);
                exit(0);
            }
            pass[len] = 0;
            pass[strcspn(pass, "\r\n")] = 0;
            if (!check_login(user, pass))
            {
                send(client,"Login failed!\n",14,0);
                close(client);
                exit(0);
            }
            send(client,"Login successful!\n",18,0);
            while (1)
            {
                memset(buf, 0, sizeof(buf));
                send(client, ">cmd\n ", 6, 0);
                len = recv(client,buf,sizeof(buf) - 1,0);
                if (len <= 0)
                    break;
                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;
                if (strcmp(buf, "exit") == 0)
                    break;
                char command[1200];
                sprintf(command,"%s > out.txt",buf);
                system(command);
                FILE *f = fopen("out.txt", "r");
                if (f == NULL)
                {
                    send(client,"Cannot open out.txt\n",20,0);
                    continue;
                }
                char outbuf[BUF_SIZE];
                while (fgets(outbuf,sizeof(outbuf),f))
                {
                    send(client,outbuf,strlen(outbuf),0);
                }
                fclose(f);
            }
            close(client);
            exit(0);
        }
        close(client);
    }
    close(listener);
    return 0;
}