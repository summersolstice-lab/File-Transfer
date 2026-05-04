#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    // 1. 创建套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999); // 端口必须和 Windows 端 9999 一致
    addr.sin_addr.s_addr = INADDR_ANY;

    // 解决端口占用问题
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return -1;
    }
    
    listen(lfd, 128);
    printf("服务器已启动，正在 9999 端口等待 Windows 连接...\n");

    while (1) {
        int cfd = accept(lfd, NULL, NULL);
        printf("Windows 已连接！\n");

        char buf[1024];
        // 接收 Windows 的第一条消息（可能是指令，也可能是文件名）
        int len = recv(cfd, buf, sizeof(buf) - 1, 0);
        if (len > 0) {
            buf[len] = '\0';
            
            // --- 情况 A：收到下载请求 ---
            if (strncmp(buf, "GET", 3) == 0) {
                printf("收到下载请求，正在回传 server.c...\n");
                FILE *fp = fopen("server.c", "rb"); // 确保目录下有这个文件
                if (fp == NULL) {
                    printf("错误：找不到 server.c 文件\n");
                } else {
                    while ((len = fread(buf, 1, sizeof(buf), fp)) > 0) {
                        send(cfd, buf, len, 0);
                    }
                    fclose(fp);
                    printf("文件回传完毕！\n");
                }
            } 
            // --- 情况 B：收到上传请求 ---
            else {
                printf("收到上传信息: %s，开始接收文件...\n", buf);
                FILE* fp = fopen("received_file", "wb");
                while ((len = read(cfd, buf, sizeof(buf))) > 0) {
                    fwrite(buf, 1, len, fp);
                }
                fclose(fp);
                printf("文件接收完毕，已保存为 received_file\n");
            }
        }
        close(cfd);
        printf("等待下一次连接...\n");
    }

    close(lfd);
    return 0;
}
