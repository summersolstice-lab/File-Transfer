#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket> // 网络库
#include <QFile>      // 文件库

// ================== 【修改 1：把结构体放在类外面定义，让全局可见】 ==================
typedef struct {
    char filename[256];
    long filesize;
} FileHeader;
// ============================================================================

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSelect_clicked();
    void on_btnSend_clicked();
    void on_btnDownload_clicked();
    void onReadyRead();

private:
    Ui::MainWindow *ui;

    // --- 在这里登记变量，报错就会消失 ---
    QTcpSocket *tcpSocket; // 通信套接字
    QString filePath;      // 文件完整路径
    QString fileName;      // 文件名
    qint64 fileSize;       // 文件大小

    // ================== 【修改 2：把这四个控制变量安全地移到类内部（private）】 ==================
    bool isHeaderReceived;    // 标记是否收到了文件头
    long totalBytesReceived;  // 记录当前收到了多少字节
    long targetFileSize;      // 记录文件总大小
    QString currentFileName;  // 记录当前正在下载的文件名
    // ============================================================================
};

#endif // MAINWINDOW_H
