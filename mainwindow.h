#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket> // 网络库
#include <QFile>      // 文件库

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
    // 这两个名字必须和你 UI 里的按钮 objectName 对应
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
};

#endif // MAINWINDOW_H
