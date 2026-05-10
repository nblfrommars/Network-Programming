#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define WORKER_COUNT 4
void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void worker_process(int listener) {
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client < 0) {
            perror("accept() failed");
            continue;
        }
        printf("[PID %d] New client connected: %d\n",
               getpid(), client);

        char buf[256];
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret > 0) {
            buf[ret] = 0;
            printf("[PID %d] Received:\n%s\n",getpid(), buf);
            char *msg ="HTTP/1.1 200 OK\r\n""Content-Type: text/html\r\n"
                       "\r\n""<html>""<body>""<h1>Anyeonghaseyoooo</h1>""</body>""</html>";
            send(client, msg, strlen(msg), 0);
        }
        close(client);
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener < 0) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(listener,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0) {

        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(listener,
             (struct sockaddr*)&addr,
             sizeof(addr)) < 0) {

        perror("bind() failed");
        close(listener);
        return 1;
    }
    if (listen(listener, 10) < 0) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    printf("Server listening on port %d...\n", PORT);
    signal(SIGCHLD, signal_handler);
    for (int i = 0; i < WORKER_COUNT; i++) {

        pid_t pid = fork();

        if (pid == 0) {
            printf("Worker %d started\n", getpid());

            worker_process(listener);

            exit(0);
        }
    }
    while (1) {
        pause();
    }
    close(listener);

    return 0;
}