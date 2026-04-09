#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 1020
#define BUF_SIZE 1024

typedef struct {
    int fd;
    char id[50];
    int registered;
} Client;

void removeClient(Client *clients, int *n, int i) {
    if (i < *n - 1)
        clients[i] = clients[*n - 1];
    (*n)--;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    Client clients[MAX_CLIENTS];
    int nClients = 0;

    fd_set fdread;
    char buf[BUF_SIZE];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int maxfd = listener;

        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i].fd, &fdread);
            if (clients[i].fd > maxfd)
                maxfd = clients[i].fd;
        }

        int ret = select(maxfd + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0) break;

        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = client;
                clients[nClients].registered = 0;
                nClients++;
                send(client, "Nhap id (client_id: name)\n", 30, 0);
            } else {
                close(client);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    close(clients[i].fd);
                    removeClient(clients, &nClients, i);
                    i--;
                    continue;
                }

                buf[ret] = 0;

                if (!clients[i].registered) {
                    char *p = strstr(buf, ":");
                    if (p) {
                        *p = 0;
                        strcpy(clients[i].id, buf);
                        clients[i].registered = 1;
                        send(clients[i].fd, "OK\n", 3, 0);
                    } else {
                        send(clients[i].fd, "Sai format\n", 11, 0);
                    }
                    continue;
                }

                char msg[BUF_SIZE + 100];
                time_t now = time(NULL);
                struct tm *t = localtime(&now);

                sprintf(msg, "%04d/%02d/%02d %02d:%02d:%02d %s: %s",
                        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                        t->tm_hour, t->tm_min, t->tm_sec,
                        clients[i].id, buf);

                for (int j = 0; j < nClients; j++) {
                    if (j != i && clients[j].registered) {
                        send(clients[j].fd, msg, strlen(msg), 0);
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}