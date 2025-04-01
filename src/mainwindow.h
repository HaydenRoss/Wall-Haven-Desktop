#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QCheckBox>
#include <QJsonDocument>
#include <QThreadPool>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

#ifndef PAGE_S
#define PAGE_S
struct PAGE{
    QUrl url;
    QJsonDocument json;
    QIcon icon[24];
};
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void SaveButtonClicked();

private:
    Ui::MainWindow *ui;
    QThreadPool m_threadpool;

    void LoadConfig();
    void GetPage(int, int retry_count = 0);
    QString GetRequestFromUI(int);
    QIcon DownloadThumbnail(QUrl url, QNetworkAccessManager* manager, int page_number, int retry_count = 0);
    void DownloadImage(QUrl url, QString destination, int retry_count = 0);
    inline QString isChecked(QCheckBox* cb){if(cb->isChecked()) return "1"; return "0";}

    int m_page_number = 1;
    int m_last_page_number = 1;
    int m_max_retry_count = 5;

    QIcon m_default_icon;
    QMap<int, PAGE> m_pages;

};
#endif // MAINWINDOW_H
