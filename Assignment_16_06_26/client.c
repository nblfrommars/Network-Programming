/*
 * chat_client.c - Chat Client theo giao thức bài tập
 * Biên dịch: gcc -o chat_client chat_client.c -lpthread
 * Chạy:      ./chat_client [host] [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define BUF_SIZE  4096
#define MSG_SIZE  4096

static int  sock_fd   = -1;
static int  running   = 1;

/* ───── Định dạng tin nhắn từ server ───── */
static void print_server_msg(const char *line)
{
    char cmd[32]        = {0};
    char rest[MSG_SIZE] = {0};

    /* Tách cmd và phần còn lại */
    const char *sp = strchr(line, ' ');
    if (sp) {
        int clen = (int)(sp - line);
        if (clen >= (int)sizeof(cmd)) clen = sizeof(cmd) - 1;
        strncpy(cmd, line, clen);
        strncpy(rest, sp + 1, sizeof(rest) - 1);
    } else {
        strncpy(cmd, line, sizeof(cmd) - 1);
    }

    /* Response codes */
    if (strcmp(cmd, "100") == 0) {
        printf("\r\033[K  [OK]\n> ");
        fflush(stdout); return;
    }
    if (strcmp(cmd, "200") == 0) {
        printf("\r\033[K  [LOI] NICKNAME DA DUOC SU DUNG\n> ");
        fflush(stdout); return;
    }
    if (strcmp(cmd, "201") == 0) {
        printf("\r\033[K  [LOI] NICKNAME KHONG HOP LE\n> ");
        fflush(stdout); return;
    }
    if (strcmp(cmd, "202") == 0) {
        printf("\r\033[K  [LOI] NICKNAME KHONG TON TAI\n> ");
        fflush(stdout); return;
    }
    if (strcmp(cmd, "203") == 0) {
        printf("\r\033[K  [LOI] KHONG CO QUYEN THUC HIEN\n> ");
        fflush(stdout); return;
    }
    if (strcmp(cmd, "999") == 0) {
        printf("\r\033[K  [LOI] LOI KHONG XAC DINH\n> ");
        fflush(stdout); return;
    }

    /* Broadcast từ server */
    if (strcmp(cmd, "JOIN") == 0) {
        printf("\r\033[K  *** %s da tham gia phong ***\n> ", rest);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "QUIT") == 0) {
        printf("\r\033[K  *** %s da roi phong ***\n> ", rest);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "MSG") == 0) {
        /* MSG <nick> <message> */
        char nick[64]   = {0};
        char msg[MSG_SIZE] = {0};
        char *s = strchr(rest, ' ');
        if (s) {
            strncpy(nick, rest, (int)(s - rest) < 63 ? (int)(s - rest) : 63);
            strncpy(msg, s + 1, sizeof(msg) - 1);
        } else {
            strncpy(nick, rest, 63);
        }
        printf("\r\033[K  [%s]: %s\n> ", nick, msg);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "PMSG") == 0) {
        char nick[64]   = {0};
        char msg[MSG_SIZE] = {0};
        char *s = strchr(rest, ' ');
        if (s) {
            strncpy(nick, rest, (int)(s - rest) < 63 ? (int)(s - rest) : 63);
            strncpy(msg, s + 1, sizeof(msg) - 1);
        }
        printf("\r\033[K  [PM tu %s]: %s\n> ", nick, msg);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "OP") == 0) {
        printf("\r\033[K  *** %s la chu phong moi ***\n> ", rest);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "KICK") == 0) {
        /* KICK <kicked_nick> <op_nick> */
        char kicked[64] = {0};
        char op[64]     = {0};
        char *s = strchr(rest, ' ');
        if (s) {
            strncpy(kicked, rest, (int)(s - rest) < 63 ? (int)(s - rest) : 63);
            strncpy(op, s + 1, 63);
        }
        printf("\r\033[K  *** %s bi kick boi %s ***\n> ", kicked, op);
        fflush(stdout); return;
    }
    if (strcmp(cmd, "TOPIC") == 0) {
        /* TOPIC <op_nick> <topic> */
        char op[64]    = {0};
        char topic[MSG_SIZE] = {0};
        char *s = strchr(rest, ' ');
        if (s) {
            strncpy(op, rest, (int)(s - rest) < 63 ? (int)(s - rest) : 63);
            strncpy(topic, s + 1, sizeof(topic) - 1);
        }
        printf("\r\033[K  *** Chu de moi: \"%s\" (dat boi %s) ***\n> ", topic, op);
        fflush(stdout); return;
    }

    /* Mặc định */
    printf("\r\033[K  [SERVER] %s\n> ", line);
    fflush(stdout);
}

/* ───── Thread nhận tin nhắn ───── */
static void *recv_thread(void *arg)
{
    (void)arg;
    char buf[BUF_SIZE * 2];
    int  buf_len = 0;

    while (running) {
        int n = recv(sock_fd, buf + buf_len, sizeof(buf) - buf_len - 1, 0);
        if (n <= 0) {
            if (running) printf("\n[Mat ket noi voi server]\n");
            running = 0;
            break;
        }
        buf_len += n;
        buf[buf_len] = '\0';

        char *start = buf;
        char *nl;
        while ((nl = strchr(start, '\n')) != NULL) {
            *nl = '\0';
            int len = (int)(nl - start);
            if (len > 0 && start[len-1] == '\r') start[--len] = '\0';

            if (start[0]) print_server_msg(start);
            start = nl + 1;
        }

        buf_len = (int)(buf + buf_len - start);
        if (buf_len > 0) memmove(buf, start, buf_len);
        else buf_len = 0;
    }
    return NULL;
}

/* ───── main ───── */

static void print_help(void)
{
    printf("\nCac lenh:\n");
    printf("  JOIN <nickname>           - Tham gia phong chat\n");
    printf("  MSG <tin nhan>            - Gui tin cho ca phong\n");
    printf("  PMSG <nick> <tin nhan>    - Gui tin rieng\n");
    printf("  OP <nick>                 - Chuyen quyen chu phong (chi OP)\n");
    printf("  KICK <nick>               - Duoi nguoi dung (chi OP)\n");
    printf("  TOPIC <chu de>            - Dat chu de phong (chi OP)\n");
    printf("  QUIT                      - Thoat\n");
    printf("  /help                     - Hien thi tro giup\n\n");
}

int main(int argc, char *argv[])
{
    char host[128] = "127.0.0.1";
    int  port      = 9000;

    if (argc >= 2) strncpy(host, argv[1], sizeof(host) - 1);
    if (argc >= 3) port = atoi(argv[2]);

    /* Kết nối tới server */
    struct hostent *he = gethostbyname(host);
    if (!he) { fprintf(stderr, "Khong the phan giai host: %s\n", host); return 1; }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port   = htons(port);
    memcpy(&serv.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock_fd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("connect"); close(sock_fd); return 1;
    }

    printf("==============================================\n");
    printf("        CHAT CLIENT - Da ket noi %s:%d\n", host, port);
    printf("==============================================\n");
    print_help();

    /* Khởi động thread nhận */
    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);
    pthread_detach(tid);

    /* Vòng lặp nhập lệnh */
    char line[MSG_SIZE];
    while (running) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        /* Loại bỏ \n cuối */
        int len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (len == 0) continue;

        /* Lệnh nội bộ */
        if (strcmp(line, "/help") == 0 || strcmp(line, "/h") == 0) {
            print_help();
            continue;
        }

        /* Gửi lệnh tới server */
        char out[MSG_SIZE + 2];
        snprintf(out, sizeof(out), "%s\n", line);
        send(sock_fd, out, strlen(out), 0);

        /* Check QUIT */
        char tmp[16] = {0};
        strncpy(tmp, line, 4);
        for (int i = 0; tmp[i]; i++) tmp[i] = toupper((unsigned char)tmp[i]);
        if (strcmp(tmp, "QUIT") == 0) break;
    }

    running = 0;
    close(sock_fd);
    printf("[Da ngat ket noi]\n");
    return 0;
}