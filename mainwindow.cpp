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
            return; // 连接失败就退出，不再执行后面的 write
        }
    }

    // 3. 只有连接成功了，才发送指令
    tcpSocket->write("GET");
    ui->textEdit->append("已向服务器请求下载源码 (GET)...");
}

// 核心：处理 Linux 传回来的数据流
void MainWindow::onReadyRead() {
    // 读取目前缓冲区的所有数据
    QByteArray data = tcpSocket->readAll();

    // 如果数据长度很小且内容是"GET"，说明是指令，不保存（可选逻辑）
    // 但通常这里收到的直接就是文件流

    // 保存到桌面，起个好区分的名字
    QString savePath = "C:/Users/vgy69/Desktop/from_linux_server.c";
    QFile file(savePath);

    // 使用 Append 追加模式，确保大文件分次接收时能完整拼合
    if (file.open(QIODevice::Append)) {
        file.write(data);
        file.close();
        ui->textEdit->append(QString("收到数据块: %1 字节，已存至桌面").arg(data.size()));
    } else {
        ui->textEdit->append("错误：无法在桌面创建文件，请检查权限！");
    }
}
