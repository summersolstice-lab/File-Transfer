#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// 任务二新增：定义文件头结构体，用于传输元数据
typedef struct {
    char filename[256];
    long filesize;
} FileHeader;

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
            
            /// --- 情况 A：收到下载请求 ---
if (strncmp(buf, "GET", 3) == 0) {
    printf("收到下载请求，正在回传 server.c...\n");
    FILE *fp = fopen("server.c", "rb"); 
    if (fp == NULL) {
        printf("错误：找不到 server.c 文件\n");
    } else {
        // =================【新插入：计算大小并发送协议头】=================
        // 1. 计算文件大小
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // 2. 填充结构体面单
        FileHeader header;
        strcpy(header.filename, "server_downloaded.c"); // 传到 Windows 后的文件名
        header.filesize = file_size;

        // 3. 率先把这块结构体数据发给 Windows
        send(cfd, &header, sizeof(header), 0);
        
        // 4. 稍微延迟 0.1 秒（100毫秒），防止后面的大文件数据跟面单粘在同一个网络包里
        usleep(100000); 
        // ====================================================================


        // =================【新修改：确保用 1024 固定大小循环分块】=================
        long total_sent = 0;
        // 强制每次只读 1024 字节，实现加分项中的“分块传输”
        while ((len = fread(buf, 1, 1024, fp)) > 0) {
            int sent = send(cfd, buf, len, 0);
            if (sent < 0) {
                perror("发送中断");
                break;
            }
            total_sent += sent;
            printf("已分块回传: %ld/%ld 字节\n", total_sent, file_size);
        }
        // ====================================================================

        fclose(fp);
        printf("文件分块回传完毕，共发送 %ld 字节！\n", total_sent);
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
