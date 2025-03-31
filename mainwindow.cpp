#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QDebug>
#include <QFileDialog>

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

    //Attempt to load config otherwise use defaults
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

    //Create NetworkManagers that catch the responses from various get requests
    manager = new QNetworkAccessManager();
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(managerFinished(QNetworkReply*)));
    p_thumbnail_manager = new QNetworkAccessManager();
    QObject::connect(p_thumbnail_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(thumbnailManagerFinished(QNetworkReply*)));
    p_image_manager = new QNetworkAccessManager();
    QObject::connect(p_image_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(imageManagerFinished(QNetworkReply*)));

    //Button Mappings (simple = lambda function)

    QObject::connect(ui->DownloadButtons, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(DownloadButtonClicked(QAbstractButton*)));
    QObject::connect(ui->SaveButton, &QPushButton::clicked, this, &MainWindow::SaveButtonClicked);

    // Search with configuration
    QObject::connect(ui->SearchButton, &QPushButton::clicked, this, [=](){SendHTTPRequest();});
    // Go back to main screen
    QObject::connect(ui->BackButton, &QPushButton::clicked, this, [=](){ui->Tabs->setCurrentIndex(0);});
    // Go to settings screen
    QObject::connect(ui->SettingsButton, &QPushButton::clicked, this, [=](){ui->Tabs->setCurrentIndex(1);});
    // Move forward back page
    QObject::connect(ui->PageBackButton, &QPushButton::clicked, this, [=](){ if(m_page_number > 1) { m_page_number--; ui->PageNumberLabel->setText(QString::number(m_page_number)); SendHTTPRequest();}});
    // Move forward one page
    QObject::connect(ui->PageForwardButton, &QPushButton::clicked, this, [=](){ m_page_number++; ui->PageNumberLabel->setText(QString::number(m_page_number)); SendHTTPRequest(); });
    // Set download folder
    QObject::connect(ui->DownloadFolderButton, &QPushButton::clicked, this, [=](){ui->DownloadFolderInput->setText(QUrl(QFileDialog::getExistingDirectory(this, tr(""), QDir::homePath())).toString() + "/");});
    // Download all images on current page
    QObject::connect(ui->DownloadAllButton, &QPushButton::clicked, this, [=]()
    {
        for(int i = 0; i < 24; i++){
            QFile image(ui->DownloadFolderInput->text() + PAGES[m_page_number].json["data"][i]["path"].toString().split("/").last());
            if(!image.exists()) p_image_manager->get(QNetworkRequest(QUrl::fromUserInput(PAGES[m_page_number].json["data"][i]["path"].toString())));
            else qDebug() << "File Already Exists";
        }
    });

    //Do initial request for the first page
    SendHTTPRequest();
}
MainWindow::~MainWindow(){
    delete ui;
}
void MainWindow::SendHTTPRequest(){
    QString request = "https://wallhaven.cc/api/v1/search?";

    QString api_token = ui->APITokenInput->text();
    if(api_token != "") request += "&apikey=" + api_token;
    request += "&categories=" + isChecked(ui->ImageTypesGeneralCheckBox) + isChecked(ui->ImageTypesAnimeCheckBox) + isChecked(ui->ImageTypesPeopleCheckBox);
    request += "&purity=" + isChecked(ui->PuritySFWCheckBox) + isChecked(ui->PuritySketchyCheckBox) + isChecked(ui->PurityNSFWCheckBox);
    request += "&sorting=" + ui->SortingComboBox->currentText();
    request += "&order=" + ui->OrderComboBox->currentText();
    request += "&ai_art_filter=" + isChecked(ui->AIArtCheckBox);
    QString resolution = ui->ResolutionInput->text(); if(resolution != "") request += "&atleast=" + resolution;
    QString ratio = ui->RatioInput->text(); if(ratio != "") request += "&ratios=" + ratio;
    request += "&page=" + QString::number(m_page_number);

    if(PAGES[m_page_number].url == request){
        for(int i = 0; i < 24; i++){
            ui->DownloadButtons->buttons().at(i)->setIcon(PAGES[m_page_number].icon[i]);
            ui->DownloadButtons->buttons().at(i)->setToolTip(PAGES[m_page_number].json["data"][i]["path"].toString());
        }
        return;
    }

    QNetworkReply* reply = manager->get(QNetworkRequest(QUrl::fromUserInput(request)));
    m_data_storage[reply] = m_page_number;
}
void MainWindow::managerFinished(QNetworkReply *reply){
    if(reply->error()){ qDebug() << reply->errorString(); return; }
    QString answer = reply->readAll();

    int page_number = m_data_storage[reply];
    m_data_storage.remove(reply);

    PAGES[page_number].url = reply->url();
    PAGES[page_number].json = QJsonDocument::fromJson(answer.toUtf8());

    for(int i = 0; i < 24; i++){
        QNetworkReply* rep = p_thumbnail_manager->get(QNetworkRequest(QUrl::fromUserInput(PAGES[page_number].json["data"][i]["thumbs"]["small"].toString())));
        m_data_storage[rep] = page_number;
    }
}
void MainWindow::thumbnailManagerFinished(QNetworkReply *reply){
    if(reply->error()){ qDebug() << reply->errorString(); return; }
    auto data = reply->readAll();

    int page_number = m_data_storage[reply];

    for(int i = 0; i < 24; i++){
        if(QUrl::fromUserInput(PAGES[page_number].json["data"][i]["thumbs"]["small"].toString()) == reply->url()){
            QPixmap p;
            p.loadFromData(data);
            PAGES[page_number].icon[i] = QIcon(p);

            if(page_number == m_page_number){
                ui->DownloadButtons->buttons().at(i)->setIcon(PAGES[m_page_number].icon[i]);
                ui->DownloadButtons->buttons().at(i)->setToolTip(PAGES[m_page_number].json["data"][i]["path"].toString());
            }
        }
    }
}
void MainWindow::DownloadButtonClicked(QAbstractButton* button){
    QString request = button->toolTip();

    QFile image(ui->DownloadFolderInput->text() + request.split("/").last());
    if(image.exists()){
        qDebug() << "File Already Exists";
        return;
    }

    p_image_manager->get(QNetworkRequest(QUrl::fromUserInput(request)));
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
                     +ui->DownloadFolderInput->text()+"\", \"sorting\": \""
                     +ui->SortingComboBox->currentText()+"\", \"order\": \""
                     +ui->OrderComboBox->currentText()+"\"}";

    QJsonDocument config_json = QJsonDocument::fromJson(output.toUtf8());

    QFile config_file(QDir::homePath() + "/.config/wallhaven/config.json");
    if(config_file.open(QIODevice::WriteOnly)){
        config_file.write(config_json.toJson());
        config_file.close();
    }

    ui->Tabs->setCurrentIndex(0);
}
