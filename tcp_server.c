#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Sử dụng: %s <Cổng> <File câu chào> <File lưu nội dung>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    char *greeting_file = argv[2];
    char *log_file = argv[3];

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr; 
    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(listener);
        exit(EXIT_FAILURE);
    }

    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        close(listener);
        exit(EXIT_FAILURE);
    }

    printf("Server đang đợi kết nối ở cổng %d...\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
        perror("accept() failed");
        close(listener);
        exit(EXIT_FAILURE);
    }

    FILE *f_greet = fopen(greeting_file, "r");
    if (f_greet == NULL) {
        perror("Lỗi mở file câu chào");
    } else {
        char greet_buf[256];
        while (fgets(greet_buf, sizeof(greet_buf), f_greet) != NULL) {
            send(client, greet_buf, strlen(greet_buf), 0);
        }
        fclose(f_greet);
    }

    FILE *f_log = fopen(log_file, "a");
    if (f_log == NULL) {
        perror("Lỗi mở file log");
    } else {
        char buf[256];
        int len;
        printf("Đang nhận dữ liệu và ghi vào file %s...\n", log_file);
        
        while ((len = recv(client, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[len] = '\0';
            
            fwrite(buf, 1, len, f_log);
            fflush(f_log);
            
            printf("Received: %s", buf);
        }
        fclose(f_log);
    }

    close(client);
    close(listener);
    printf("Ended connection\n");

    return 0;
}