#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS 1024
#define BUF_SIZE 1024

typedef struct {
    int fd;
    int logged;
} Client;

int check_login(char *user, char *pass) {
    FILE *f = fopen("user.txt", "r");
    if (!f) return 0;

    char u[100], p[100];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

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
            clients[nClients].fd = client;
            clients[nClients].logged = 0;
            nClients++;

            send(client, "Nhap: user pass\n", 17, 0);
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
                buf[strcspn(buf, "\r\n")] = 0;

                if (!clients[i].logged) {
                    char user[100], pass[100];

                    if (sscanf(buf, "%s %s", user, pass) == 2) {
                        if (check_login(user, pass)) {
                            clients[i].logged = 1;
                            send(clients[i].fd, "Login success\n", 14, 0);
                        } else {
                            send(clients[i].fd, "Login failed\n", 13, 0);
                        }
                    } else {
                        send(clients[i].fd, "Nhap dung: user pass\n", 23, 0);
                    }
                    continue;
                }

                if (strlen(buf) == 0) continue;

                char cmd[BUF_SIZE + 50];
                sprintf(cmd, "%s > out.txt", buf);

                system(cmd);

                FILE *f = fopen("out.txt", "r");
                if (f) {
                    char out[BUF_SIZE];
                    int n;
                    while ((n = fread(out, 1, sizeof(out), f)) > 0) {
                        send(clients[i].fd, out, n, 0);
                    }
                    fclose(f);
                } else {
                    send(clients[i].fd, "Command error\n", 14, 0);
                }
            }
        }
    }

    close(listener);
    return 0;
}