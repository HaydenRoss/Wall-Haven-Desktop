#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QCheckBox>
#include <QJsonDocument>

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
    void managerFinished(QNetworkReply *reply);
    void thumbnailManagerFinished(QNetworkReply *reply);
    void SaveButtonClicked();
private:
    Ui::MainWindow *ui;
    QNetworkAccessManager* manager;
    QNetworkAccessManager* p_thumbnail_manager;


    void LoadConfig();
    void SendHTTPRequest();
    inline QString isChecked(QCheckBox* cb){if(cb->isChecked()) return "1"; return "0";}
    void DownloadImage(QUrl url, QString destination, int retry_count = 0);

    int m_page_number = 1;
    QMap<int, PAGE> PAGES;
    QHash<QNetworkReply*, int> m_data_storage;
};
#endif // MAINWINDOW_H
