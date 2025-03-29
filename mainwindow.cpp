#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFile>

#include <QJsonObject>

#ifndef PAGE_S
#define PAGE_S
struct PAGE{
    QUrl url;
    QJsonDocument json;
    QIcon icon[24];
};
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QFile config(QDir::homePath()+"/.config/wallhaven/config.json");
    if(config.open(QIODevice::ReadOnly)){
        auto contents = config.readAll();
        QJsonDocument config_json = QJsonDocument::fromJson(contents);

        ui->ImageTypesGeneralCheckBox->setChecked(config_json["image_types"]["general"].toInt());
        ui->ImageTypesAnimeCheckBox->setChecked(config_json["image_types"]["anime"].toInt());
        ui->ImageTypesPeopleCheckBox->setChecked(config_json["image_types"]["people"].toInt());
        ui->PuritySFWCheckBox->setChecked(config_json["purity"]["sfw"].toInt());
        ui->PuritySketchyCheckBox->setChecked(config_json["purity"]["sketchy"].toInt());
        ui->PurityNSFWCheckBox->setChecked(config_json["purity"]["nsfw"].toInt());
        ui->AIArtCheckBox->setChecked(config_json["ai_art"].toInt());
        ui->ResolutionInput->setText(config_json["resolution"].toString());
        ui->RatioInput->setText(config_json["ratios"].toString());
        ui->APITokenInput->setText(config_json["token"].toString());
        ui->DownloadFolderInput->setText(config_json["download_folder"].toString());
        config.close();
    }

    ui->PageNumberLabel->setText(QString::number(m_page_number));

    manager = new QNetworkAccessManager();
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));

    p_thumbnail_manager = new QNetworkAccessManager();
    QObject::connect(p_thumbnail_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(thumbnailManagerFinished(QNetworkReply*)));

    p_image_manager = new QNetworkAccessManager();
    QObject::connect(p_image_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(imageManagerFinished(QNetworkReply*)));


    QObject::connect(ui->DownloadButtons, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(DownloadButtonClicked(QAbstractButton*)));

    QObject::connect(ui->SearchButton, &QPushButton::clicked, this, &MainWindow::SearchButtonClicked);
    QObject::connect(ui->SettingsButton, &QPushButton::clicked, this, &MainWindow::SettingsButtonClicked);
    QObject::connect(ui->SaveButton, &QPushButton::clicked, this, &MainWindow::SaveButtonClicked);
    QObject::connect(ui->BackButton, &QPushButton::clicked, this, &MainWindow::BackButtonClicked);
    QObject::connect(ui->DownloadFolderButton, &QPushButton::clicked, this, &MainWindow::DownloadFolderButtonClicked);
    QObject::connect(ui->PageBackButton, &QPushButton::clicked, this, &MainWindow::PageBackButtonClicked);
    QObject::connect(ui->PageForwardButton, &QPushButton::clicked, this, &MainWindow::PageForwardButtonClicked);
    QObject::connect(ui->DownloadAllButton, &QPushButton::clicked, this, &MainWindow::DownloadAllButtonClicked);

    SendHTTPRequest();
}

void MainWindow::DownloadAllButtonClicked(){
    for(int i = 0; i < 24; i++){
        p_image_manager->get(QNetworkRequest(QUrl::fromUserInput(PAGES[m_page_number].json["data"][i]["path"].toString())));
    }
}

void MainWindow::DownloadButtonClicked(QAbstractButton* button){
    QString request = button->text();
    qDebug() << button->text();
    p_image_manager->get(QNetworkRequest(QUrl::fromUserInput(request)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SettingsButtonClicked()
{
    ui->Tabs->setCurrentIndex(1);
}

void MainWindow::SendHTTPRequest(){    
    QString request = parseUIInput();

    if(PAGES[m_page_number].url == request){
        for(int i = 0; i < 24; i++){
            //if(PAGES[m_page_number].icon[i].isNull()){
            //    manager->get(QNetworkRequest(QUrl::fromUserInput(request)));
            //    return;
            //}
            ui->DownloadButtons->buttons().at(i)->setIcon(PAGES[m_page_number].icon[i]);
            ui->DownloadButtons->buttons().at(i)->setText(PAGES[m_page_number].json["data"][i]["path"].toString());
        }
        return;
    }
    manager->get(QNetworkRequest(QUrl::fromUserInput(request)));
}

QString MainWindow::isChecked(QCheckBox* cb){
    if(cb->isChecked()) return "1";
    return "0";
}

QString MainWindow::parseUIInput(){
    QString request = "https://wallhaven.cc/api/v1/search?";

    QString api_token = ui->APITokenInput->text();
    if(api_token != "") request += "&apikey=" + api_token;

    request += "&categories=" + isChecked(ui->ImageTypesGeneralCheckBox) + isChecked(ui->ImageTypesAnimeCheckBox) + isChecked(ui->ImageTypesPeopleCheckBox);
    request += "&purity=" + isChecked(ui->PuritySFWCheckBox) + isChecked(ui->PuritySketchyCheckBox) + isChecked(ui->PurityNSFWCheckBox);
    request += "&sorting=views";
    request += "&order=desc";
    request += "&ai_art_filter=" + isChecked(ui->AIArtCheckBox);

    QString resolution = ui->ResolutionInput->text();
    if(resolution != "") request += "&atleast=" + resolution;

    QString ratio = ui->RatioInput->text();
    if(ratio != "") request += "&ratios=" + ratio;

    request += "&page=" + QString::number(m_page_number);

    qDebug() << request;

    return request;
}

void MainWindow::managerFinished(QNetworkReply *reply){
    if(reply->error()){
        qDebug() << reply->errorString();
        return;
    }
    QString answer = reply->readAll();

    PAGES[m_page_number].url = reply->url();
    PAGES[m_page_number].json = QJsonDocument::fromJson(answer.toUtf8());

    for(int i = 0; i < 24; i++){
        p_thumbnail_manager->get(QNetworkRequest(QUrl::fromUserInput(PAGES[m_page_number].json["data"][i]["thumbs"]["small"].toString())));
    }
}

void MainWindow::thumbnailManagerFinished(QNetworkReply *reply){
    if(reply->error()){
        qDebug() << reply->errorString();
        return;
    }

    auto data = reply->readAll();

    for(int i = 0; i < 24; i++){
        if(QUrl::fromUserInput(PAGES[m_page_number].json["data"][i]["thumbs"]["small"].toString()) == reply->url()){
            QPixmap p;
            p.loadFromData(data);
            PAGES[m_page_number].icon[i] = QIcon(p);
            ui->DownloadButtons->buttons().at(i)->setIcon(PAGES[m_page_number].icon[i]);
            ui->DownloadButtons->buttons().at(i)->setText(PAGES[m_page_number].json["data"][i]["path"].toString());
        }
    }
}

void MainWindow::imageManagerFinished(QNetworkReply *reply){
    if(reply->error()){
        qDebug() << reply->errorString();
        return;
    }

    auto data = reply->readAll();

    QPixmap image;
    image.loadFromData(data);
    QString output = ui->DownloadFolderInput->text() + reply->url().toString().split("/").last();
    qDebug() << image.save(output, nullptr, 100);
}

void MainWindow::SaveButtonClicked()
{
    QDir config_dir;
    config_dir.mkpath(QDir::homePath() + "/.config/wallhaven");
    qDebug() << QDir::homePath() + "/.config/wallhaven";

    QString output = "{\"image_types\":{\"general\": "
                     +isChecked(ui->ImageTypesGeneralCheckBox)+",\"anime\": "
                     +isChecked(ui->ImageTypesAnimeCheckBox)+",\"people\": "
                     +isChecked(ui->ImageTypesPeopleCheckBox)+" }, \"purity\": { \"sfw\": "
                     +isChecked(ui->PuritySFWCheckBox)+", \"sketchy\": "
                     +isChecked(ui->PuritySketchyCheckBox)+", \"nsfw\": "
                     +isChecked(ui->PurityNSFWCheckBox)+" }, \"ai_art\": "
                     +isChecked(ui->AIArtCheckBox)+", \"resolution\": \""
                     +ui->ResolutionInput->text()+"\", \"ratios\": \""
                     +ui->RatioInput->text()+"\", \"token\": \""
                     +ui->APITokenInput->text()+"\", \"download_folder\": \""
                     +ui->DownloadFolderInput->text()+"\"}";

    QJsonDocument config_json = QJsonDocument::fromJson(output.toUtf8());

    QFile config_file(QDir::homePath() + "/.config/wallhaven/config.json");
    if(config_file.open(QIODevice::WriteOnly)){
        config_file.write(config_json.toJson());
        config_file.close();
    }

    ui->Tabs->setCurrentIndex(0);
}


void MainWindow::BackButtonClicked()
{
    ui->Tabs->setCurrentIndex(0);
}


void MainWindow::DownloadFolderButtonClicked()
{
    ui->DownloadFolderInput->setText(QUrl(QFileDialog::getExistingDirectory(this, tr(""), QDir::homePath())).toString() + "/");
}


void MainWindow::SearchButtonClicked()
{
    SendHTTPRequest();
}


void MainWindow::PageBackButtonClicked()
{
    if(m_page_number > 1) {
        m_page_number--;
        ui->PageNumberLabel->setText(QString::number(m_page_number));
        SendHTTPRequest();
    }
}


void MainWindow::PageForwardButtonClicked()
{
    m_page_number++;
    ui->PageNumberLabel->setText(QString::number(m_page_number));
    SendHTTPRequest();
}
