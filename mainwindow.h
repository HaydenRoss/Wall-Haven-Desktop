#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>

#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
#include <QPushButton>
#include <QCheckBox>
#include <QJsonDocument>
#include <QDebug>
#include <QFileDialog>

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
    void imageManagerFinished(QNetworkReply *reply);

    void SearchButtonClicked();
    void DownloadButtonClicked(QAbstractButton*);
    void SettingsButtonClicked();
    void SaveButtonClicked();
    void BackButtonClicked();
    void DownloadFolderButtonClicked();
    void PageBackButtonClicked();
    void PageForwardButtonClicked();
    void DownloadAllButtonClicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager* manager;
    QNetworkAccessManager* p_thumbnail_manager;
    QNetworkAccessManager* p_image_manager;

    int m_page_number = 1;
    QMap<int, PAGE> PAGES;

    QString isChecked(QCheckBox* cb);
    QString parseUIInput();
    void SendHTTPRequest();
};
#endif // MAINWINDOW_H
