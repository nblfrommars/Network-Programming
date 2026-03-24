#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

int main(int argc, char* argv[]){
    if(argc < 3){
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    int client  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1){
        perror("socket() failed!");
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    
    if(connect(client, (struct sockaddr*)&addr,sizeof(addr)) == -1){
        perror("connect() failed!");
        exit(1);
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("Current folder: %s\n",cwd);
    else {
        perror("getcwd() failed!");
        exit(1);
    }

    DIR *dir = opendir(".");
    if (dir == NULL){
        perror("Cannot open directory!");
        return 1;
    }

    struct dirent *entry;
    struct stat fileStat;
    char *all_files = (char*)malloc(65536);
    if(!all_files){
    perror("malloc failed");
    return 1;
}
    all_files[0] = '\0';     

    while ((entry = readdir(dir)) != NULL){
        if (stat(entry->d_name, &fileStat) == 0){
            if(S_ISREG(fileStat.st_mode)){
                char buf[256];
                snprintf(buf, sizeof(buf), "%-30s %ld bytes\n", entry->d_name, (long)fileStat.st_size);
                strncat(all_files, buf, 65536 - strlen(all_files) - 1);
            }
        }
    }
    send(client, all_files, strlen(all_files), 0);
    free(all_files);
    closedir(dir);
    close(client);
    return 0;
}