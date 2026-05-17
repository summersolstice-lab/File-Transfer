#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化 TCP 套接字
    tcpSocket = new QTcpSocket(this);
    // 禁用代理，确保局域网连接通畅
    tcpSocket->setProxy(QNetworkProxy::NoProxy);

    // 2. 连接状态反馈
    connect(tcpSocket, &QTcpSocket::connected, [=](){
        ui->textEdit->append("--- 成功连接到服务器 ---");
    });

    // 3. 断开连接反馈
    connect(tcpSocket, &QTcpSocket::disconnected, [=](){
        ui->textEdit->append("--- 已断开连接 ---");
    });

    // 4. 关键：手动连接信号，处理 Linux 回传的数据
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ------------------ 【功能1：发送文件给 Linux】 ------------------

void MainWindow::on_btnSelect_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "选择文件", "C:/");
    if(!path.isEmpty()) {
        fileName.clear();
        QFileInfo info(path);
        fileName = info.fileName();
        fileSize = info.size();
        filePath = path;

        ui->label->setText(fileName);
        ui->textEdit->append(QString("已就绪: %1 (%2 bytes)").arg(fileName).arg(fileSize));
    }
}

void MainWindow::on_btnSend_clicked()
{
    if(filePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择一个文件！");
        return;
    }

    // --- 请确保这里的 IP 和端口与虚拟机一致 ---
    QString ip = "192.168.172.128";
    quint16 port = 9999;

    if(tcpSocket->state() != QAbstractSocket::ConnectedState) {
        ui->textEdit->append("正在连接服务器...");
        tcpSocket->abort();
        tcpSocket->connectToHost(ip, port);

        if(!tcpSocket->waitForConnected(5000)) {
            ui->textEdit->append("错误: 连接超时! 原因: " + tcpSocket->errorString());
            return;
        }
    }

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)) {
        ui->textEdit->append("错误：无法打开文件！");
        return;
    }

    ui->textEdit->append("正在发送文件头信息...");
    // 发送包头：文件名##大小
    QString head = QString("%1##%2").arg(fileName).arg(fileSize);
    tcpSocket->write(head.toUtf8());
    tcpSocket->flush();
    QThread::msleep(50); // 稍微停顿，防止粘包

    ui->textEdit->append("正在传输数据内容...");
    qint64 sentSize = 0;
    while(!file.atEnd()) {
        QByteArray line = file.read(1024 * 64);
        qint64 len = tcpSocket->write(line);
        sentSize += len;
    }

    if(sentSize == fileSize) {
        ui->textEdit->append("恭喜：文件发送完毕！");
    }
    file.close();
}

// ------------------ 【功能2：从 Linux 下载文件】 ------------------

// 点击“下载”按钮
void MainWindow::on_btnDownload_clicked() {
    // 每次点击下载前，重置接收状态计数器，防止上一次的数据干扰
    isHeaderReceived = false;
    totalBytesReceived = 0;
    targetFileSize = 0;

    // 1. 准备连接参数（确保 IP 和端口与虚拟机一致）
    QString ip = "192.168.172.128";
    quint16 port = 9999;

    // 2. 如果当前没有连接，则发起连接请求
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        ui->textEdit->append("正在建立下载连接...");
        tcpSocket->abort(); // 清除之前的状态
        tcpSocket->connectToHost(ip, port);

        // 等待 3 秒尝试连接
        if (!tcpSocket->waitForConnected(3000)) {
            ui->textEdit->append("连接失败！原因: " + tcpSocket->errorString());
            return;
        }
    }

    // 3. 只有连接成功了，才发送指令
    tcpSocket->write("GET");
    ui->textEdit->append("已向服务器请求下载源码 (GET)...");
}

// ================== 【位置 2：完全重写替换后的 onReadyRead 函数】 ==================
void MainWindow::onReadyRead() {
    // 状态 A：如果还没有收到文件头，优先解析文件头
    if (!isHeaderReceived) {
        // 如果缓冲区的数据大小，连一个 FileHeader 结构体的大小(264字节)都不够，就退出继续等
        if (tcpSocket->bytesAvailable() < sizeof(FileHeader)) {
            return;
        }

        // 数据够了，把文件头结构体整体读出来
        FileHeader header;
        tcpSocket->read((char*)&header, sizeof(FileHeader));

        // 从结构体里提取出虚拟机传过来的真实文件名和文件总大小
        currentFileName = QString::fromUtf8(header.filename);
        targetFileSize = header.filesize;
        isHeaderReceived = true; // 状态切换：表示文件头已成功解析出来了
        totalBytesReceived = 0;  // 重置接收计数器

        ui->textEdit->append(QString("【协议解析】即将下载文件: %1, 总大小: %2 字节").arg(currentFileName).arg(targetFileSize));

        // 自动初始化你在界面上拖出来的 progressBar 进度条
        ui->progressBar->setRange(0, 100);
        ui->progressBar->setValue(0);

        // 如果服务端刚发完头，缓冲区里还有后续的数据块，不要跳出，直接往下走去读数据
        if (tcpSocket->bytesAvailable() == 0) {
            return;
        }
    }

    // 状态 B：文件头已经解析过，后面来的全是 1024 字节的文件分块流
    if (tcpSocket->bytesAvailable() > 0) {
        QByteArray data = tcpSocket->readAll();

        // 依然保存到桌面，但是文件名直接使用从 Linux 传过来的真实名字 currentFileName
        QString savePath = QString("C:/Users/vgy69/Desktop/%1").arg(currentFileName);
        QFile file(savePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Append)) { // 用追加模式安全写入
            file.write(data);
            file.close();

            totalBytesReceived += data.size(); // 累加收到的字节数

            // 计算当前百分比，并实时刷新你的 UI 进度条
            int progress = static_cast<int>((totalBytesReceived * 100) / targetFileSize);
            ui->progressBar->setValue(progress);

            ui->textEdit->append(QString("已分块接收: %1/%2 字节 (%3%)")
                                     .arg(totalBytesReceived).arg(targetFileSize).arg(progress));

            // 如果收到的总字节数达到了目标文件的大小，说明整个大文件下载完毕
            if (totalBytesReceived >= targetFileSize) {
                ui->textEdit->append("★ 恭喜：大文件分块下载并组装完毕！");
                isHeaderReceived = false; // 功成身退，重置状态为下一次下载做准备
                tcpSocket->disconnectFromHost(); // 优雅断开连接
            }
        } else {
            ui->textEdit->append("错误：无法在桌面创建文件，请检查权限！");
        }
    }
}
// ============================================================================
