嵌入式文件传输系统
这是一个基于 C 语言服务端与 Qt 客户端实现的嵌入式双向文件传输系统。本项目主要展示了在 Linux 环境下，通过 Socket 编程实现 Windows 与 Ubuntu 虚拟机之间的文件下载逻辑。

目录结构说明
- `server.c`: Linux 服务端核心代码，负责监听连接及发送文件流。
- `main.cpp` / `mainwindow.cpp`: Qt 客户端主逻辑，实现 UI 交互与文件接收。
- `mainwindow.ui`: 客户端界面设计文件。
- `CMakeLists.txt`: 项目编译配置文件。

核心功能
1. **双向连接**：实现宿主机（Windows）与虚拟机（Ubuntu）的稳定 TCP 连接。
2. **下载逻辑**：服务端读取本地文件并分片传输，客户端实时接收并保存至本地。
3. **状态反馈**：客户端界面集成传输进度显示（基于 Qt 信号槽机制）。

 编译与运行
### 服务端 (Ubuntu)
```bash
gcc server.c -o server
./server
