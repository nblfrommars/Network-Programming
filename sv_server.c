#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

struct SinhVien {
    char mssv[15];
    char hoTen[50];
    char ngaySinh[15];
    float diemTB;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Use: %s <Port> <File log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *log_filename = argv[2];

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting on port %d...\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
    if (client < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    char *client_ip = inet_ntoa(client_addr.sin_addr);
    printf("Client connecting from IP: %s\n", client_ip);

    FILE *f_log = fopen(log_filename, "a");
    if (f_log == NULL) {
        perror("Cannot open file log");
        exit(EXIT_FAILURE);
    }

    struct SinhVien sv;
     char buffer[512]; 
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int ret = recv(client, buffer, sizeof(buffer) - 1, 0);
        
        if (ret <= 0) {
            printf("Client has disconnected!.\n");
            break;
        }

        buffer[ret] = '\0';
        
        buffer[strcspn(buffer, "\n\r")] = 0;

        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[26];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        printf("%s %s %s\n", client_ip, time_str, buffer);
        fprintf(f_log, "%s %s %s\n", client_ip, time_str, buffer);
        
        fflush(f_log); 
    }

    fclose(f_log);
    close(client);
    close(listener);

    return 0;
}