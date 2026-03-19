#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

// Định nghĩa cấu trúc sinh viên theo yêu cầu
struct SinhVien {
    char mssv[15];
    char hoTen[50];
    char ngaySinh[15];
    float diemTB;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Sử dụng: %s <Địa chỉ IP> <Cổng>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect() failed");
        close(client);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    struct SinhVien sv;
    char buffer[256];

    while (1) {
        printf("Nhap thong tin sinh vien or exit de thoat\n");
        
        printf("MSSV: ");
        scanf("%s", sv.mssv);
        if (strcmp(sv.mssv, "exit") == 0) break;
        
        while (getchar() != '\n');

        printf("Ho ten: ");
        fgets(sv.hoTen, sizeof(sv.hoTen), stdin);
        sv.hoTen[strcspn(sv.hoTen, "\n")] = 0; 

        printf("Ngay sinh (dd/mm/yyyy): ");
        scanf("%s", sv.ngaySinh);

        printf("GPA: ");
        scanf("%f", &sv.diemTB);

        int sent = send(client, &sv, sizeof(sv), 0);
        if (sent < 0) {
            perror("send() failed");
            break;
        }
        printf("=>Sent Students Infor (%d bytes).\n\n", sent);
    }

    close(client);
    printf("Closed Connection.\n");

    return 0;
}