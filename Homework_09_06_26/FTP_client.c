#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

#define SERVER_HOST "lebavui.io.vn" 
#define CONTROL_PORT 21 
#define BUFFER_SIZE 4096

#define MSSV "20235362"
#define NGAY_SINH "01"

/**
 * Hàm gửi một câu lệnh FTP tới Server và nhận phản hồi về.
 * @param sock: Socket của kênh điều khiển 
 * @param cmd: Chuỗi câu lệnh FTP cần gửi (Nếu NULL thì chỉ nhận phản hồi)
 * @param response: Bộ đệm dùng để lưu chuỗi phản hồi trả về từ Server
 */
void send_ftp_cmd(int sock, const char *cmd, char *response) {
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
    
    if (cmd != NULL) {
        send(sock, cmd, strlen(cmd), 0);
        printf(">> %s", cmd);
    }
    
    int bytes_received = recv(sock, buf, sizeof(buf) - 1, 0);
    if (bytes_received > 0) {
        buf[bytes_received] = '\0';
        printf("<< %s", buf);
        if (response) {
            strcpy(response, buf);
        }
    }
}

/**
 * Hàm bóc tách phản hồi của lệnh PASV để lấy IP, Port và khởi tạo kết nối kênh dữ liệu.
 * @param pasv_resp: Chuỗi phản hồi từ lệnh PASV
 * @return: Socket của kênh dữ liệu đã kết nối thành công, hoặc -1 nếu lỗi
 */
int connect_data_channel(char *pasv_resp) {
    char *start = strchr(pasv_resp, '(');
    char *end = strchr(pasv_resp, ')');
    if (!start || !end) return -1;
    
    *end = '\0';
    start++;
    
    int h1, h2, h3, h4, p1, p2;
    sscanf(start, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
    
    char ip[32];
    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    int port = p1 * 256 + p2;
    
    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in data_addr;
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &data_addr.sin_addr);
    
    if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("Fail to connect data");
        return -1;
    }
    return data_sock;
}

/**
 * Hàm đảo ngược một chuỗi ký tự
 * @param str: Con trỏ trỏ tới chuỗi ký tự
 * @param len: Chiều dài của chuỗi cần đảo ngược
 */
void reverse_string(char *str, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

int main() {
    int ctrl_sock;
    struct sockaddr_in server_addr;
    struct hostent *he;
    char cmd[256], resp[BUFFER_SIZE];
    char question_filename[128] = {0};
    char answer_filename[128] = {0};
    char file_content[256] = {0};

    if ((he = gethostbyname(SERVER_HOST)) == NULL) {
        herror("gethostbyname");
        return 1;
    }

    ctrl_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONTROL_PORT);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);

    if (connect(ctrl_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Kết nối tới Server thất bại");
        return 1;
    }
    
    send_ftp_cmd(ctrl_sock, NULL, resp);

    sprintf(cmd, "USER user_%s\r\n", MSSV);
    send_ftp_cmd(ctrl_sock, cmd, resp);

    char pass[10];
    const char *mssv_ptr = MSSV + strlen(MSSV) - 4;
    sprintf(pass, "%s%s", mssv_ptr, NGAY_SINH);
    
    sprintf(cmd, "PASS %s\r\n", pass);
    send_ftp_cmd(ctrl_sock, cmd, resp);

    sprintf(cmd, "PASV\r\n");
    send_ftp_cmd(ctrl_sock, cmd, resp);
    int data_sock = connect_data_channel(resp);

    sprintf(cmd, "LIST\r\n");
    send_ftp_cmd(ctrl_sock, cmd, resp);
    char list_buf[BUFFER_SIZE] = {0};
    int bytes_read = recv(data_sock, list_buf, sizeof(list_buf) - 1, 0);
    close(data_sock);
    send_ftp_cmd(ctrl_sock, NULL, resp);
    char *q_ptr = strstr(list_buf, "question_");
    if (q_ptr == NULL) {
        printf("No question file found!\n");
        close(ctrl_sock);
        return 1;
    }
    
    sscanf(q_ptr, "%s", question_filename);
    printf("--> Found Question File!: %s\n", question_filename);
    sprintf(cmd, "PASV\r\n");
    send_ftp_cmd(ctrl_sock, cmd, resp);
    data_sock = connect_data_channel(resp);

    sprintf(cmd, "RETR %s\r\n", question_filename);
    send_ftp_cmd(ctrl_sock, cmd, resp);

    memset(file_content, 0, sizeof(file_content));
    bytes_read = recv(data_sock, file_content, sizeof(file_content) - 1, 0);
    file_content[bytes_read] = '\0';
    close(data_sock);
    send_ftp_cmd(ctrl_sock, NULL, resp);

    printf("--> Question File Content: %s\n", file_content);

    char *suffix = strchr(question_filename, '_'); 
    sprintf(answer_filename, "answer%s", suffix);

    int content_len = strlen(file_content);
    while(content_len > 0 && (file_content[content_len-1] == '\n' || file_content[content_len-1] == '\r')) {
        file_content[content_len-1] = '\0';
        content_len--;
    }

    reverse_string(file_content, content_len);
    printf("--> Reversed content: %s\n", file_content);

    sprintf(cmd, "PASV\r\n");
    send_ftp_cmd(ctrl_sock, cmd, resp);
    data_sock = connect_data_channel(resp);

    sprintf(cmd, "STOR %s\r\n", answer_filename);
    send_ftp_cmd(ctrl_sock, cmd, resp);

    send(data_sock, file_content, strlen(file_content), 0);
    close(data_sock);
    send_ftp_cmd(ctrl_sock, NULL, resp);

    sprintf(cmd, "QUIT\r\n");
    send_ftp_cmd(ctrl_sock, cmd, resp);
    close(ctrl_sock);

    return 0;
} 