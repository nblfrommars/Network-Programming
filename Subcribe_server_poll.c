#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_TOPICS 100

struct Client {
    int fd;
    char topics[MAX_TOPICS][50];
    int topic_count;
};

struct Client clients[MAX_CLIENTS];

struct pollfd fds[MAX_CLIENTS + 1];
int nfds = 0;

void add_topic(struct Client *c, char *topic) {
    if (c->topic_count < MAX_TOPICS) {
        strcpy(c->topics[c->topic_count++], topic);
    }
}

int is_subscribed(struct Client *c, char *topic) {
    for (int i = 0; i < c->topic_count; i++) {
        if (strcmp(c->topics[i], topic) == 0)
            return 1;
    }
    return 0;
}

void remove_topic(struct Client *c, char *topic) {
    for (int i = 0; i < c->topic_count; i++) {
        if (strcmp(c->topics[i], topic) == 0) {

            for (int j = i; j < c->topic_count - 1; j++) {
                strcpy(c->topics[j], c->topics[j + 1]);
            }

            c->topic_count--;
            return;
        }
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 10);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
    }

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    nfds = 1;

    printf("Server running on port 9000...\n");

    while (1) {
        int ret = poll(fds, nfds, -1);

        if (ret < 0) {
            perror("poll() failed");
            exit(1);
        }

        for (int i = 0; i < nfds; i++) {

            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listener) {
                    int client_fd = accept(listener, NULL, NULL);

                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;

                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].fd == -1) {
                            clients[j].fd = client_fd;
                            clients[j].topic_count = 0;
                            break;
                        }
                    }
                    printf("New client: %d\n", client_fd);
                }
                else {
                    char buf[BUFFER_SIZE];
                    int len = recv(fds[i].fd, buf, sizeof(buf)-1, 0);

                    if (len <= 0) {
                        printf("Client %d disconnected\n", fds[i].fd);
                        close(fds[i].fd);

                        fds[i] = fds[nfds - 1];
                        nfds--;

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].fd == fds[i].fd) {
                                clients[j].fd = -1;
                                break;
}
}
i--;
}
            else {
                  buf[len] = '\0';
                  printf("Received: %s\n", buf);
                  char cmd[10], topic[50], msg[500];
                  if (sscanf(buf, "%s %s", cmd, topic) >= 2 &&
                        strcmp(cmd, "SUB") == 0) {
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (clients[j].fd == fds[i].fd) {
                                    add_topic(&clients[j], topic);
                                    printf("Client %d subscribed %s\n",
                                           fds[i].fd, topic);
                                    break;
        }
        }
    }
                else if (sscanf(buf, "%s %s %[^\n]", cmd, topic, msg) >= 3 && strcmp(cmd, "PUB") == 0) {
                    char buffer[BUFFER_SIZE];
                    sprintf(buffer, "[%s]: %s\n", topic, msg);

                for (int j = 0; j < MAX_CLIENTS; j++) {

                if (clients[j].fd != -1 &&
                  clients[j].fd != fds[i].fd &&
                  is_subscribed(&clients[j], topic)) {

                  send(clients[j].fd, buffer, strlen(buffer), 0);
        }
    }
}
                else if (sscanf(buf, "%s %s", cmd, topic) >= 2 &&
                         strcmp(cmd, "UNSUB") == 0) {

                       for (int j = 0; j < MAX_CLIENTS; j++) {
                       if (clients[j].fd == fds[i].fd) {
                       remove_topic(&clients[j], topic);
                       printf("Client %d unsubscribed %s\n",fds[i].fd, topic);
                       break;
        }
    }
}
    }
    }
}
}
}

    return 0;
}