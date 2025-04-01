#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QDebug>
#include <QFileDialog>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    LoadConfig();

    QObject::connect(ui->SaveButton, &QPushButton::clicked, this, &MainWindow::SaveButtonClicked);
    QObject::connect(ui->SearchButton, &QPushButton::clicked, this, [=](){GetPage(m_page_number);});
    QObject::connect(ui->BackButton, &QPushButton::clicked, this, [=](){ui->Tabs->setCurrentIndex(0);});
    QObject::connect(ui->SettingsButton, &QPushButton::clicked, this, [=](){ui->Tabs->setCurrentIndex(1);});
    QObject::connect(ui->PageBackButton, &QPushButton::clicked, this, [=](){ if(m_page_number > 1) { m_page_number--; GetPage(m_page_number);}});
    QObject::connect(ui->PageForwardButton, &QPushButton::clicked, this, [=](){ m_page_number++; GetPage(m_page_number); });
    QObject::connect(ui->DownloadFolderButton, &QPushButton::clicked, this, [=](){ui->DownloadFolderInput->setText(QUrl(QFileDialog::getExistingDirectory(this, tr(""), QDir::homePath())).toString() + "/");});
    QObject::connect(ui->DownloadButtons, &QButtonGroup::buttonClicked, this, [=](QAbstractButton*button){ DownloadImage(QUrl::fromUserInput(button->toolTip()), ui->DownloadFolderInput->text()); });
    QObject::connect(ui->DownloadAllButton, &QPushButton::clicked, this, [=](){ for(int i = 0; i < 24; i++) DownloadImage(QUrl::fromUserInput(m_pages[m_page_number].json["data"][i]["path"].toString()), ui->DownloadFolderInput->text()); });

    QObject::connect(ui->SearchInput, &QLineEdit::returnPressed, this, [=](){ GetPage(m_page_number); });
    QObject::connect(ui->RatioInput, &QLineEdit::returnPressed, this, [=](){ GetPage(m_page_number); });
    QObject::connect(ui->ResolutionInput, &QLineEdit::returnPressed, this, [=](){ GetPage(m_page_number); });
    QObject::connect(ui->PageNumberInput, &QLineEdit::returnPressed, this, [=](){ m_page_number = ui->PageNumberInput->text().toInt(); GetPage(m_page_number); });

    m_default_icon = QIcon(":/resources/no-icon.png");

    GetPage(m_page_number);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::LoadConfig() {
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
        ui->ExactCheckBox->setChecked(config_json["exact_resolution"].toInt());
        ui->PagePrefetchAmountInput->setText(config_json["page_prefetch_amount"].toString());
        config.close();
    }
}

QString MainWindow::GetRequestFromUI(int page_number){
    QString request = "https://wallhaven.cc/api/v1/search?";

    QString api_token = ui->APITokenInput->text();
    if(api_token != "") request += "&apikey=" + api_token;
    QString q = ui->SearchInput->text(); if(q != "") request += "&q=" + q;
    request += "&categories=" + isChecked(ui->ImageTypesGeneralCheckBox) + isChecked(ui->ImageTypesAnimeCheckBox) + isChecked(ui->ImageTypesPeopleCheckBox);
    request += "&purity=" + isChecked(ui->PuritySFWCheckBox) + isChecked(ui->PuritySketchyCheckBox) + isChecked(ui->PurityNSFWCheckBox);
    request += "&sorting=" + ui->SortingComboBox->currentText();
    request += "&order=" + ui->OrderComboBox->currentText();
    request += "&ai_art_filter=" + isChecked(ui->AIArtCheckBox);

    QString resolution = ui->ResolutionInput->text();
    if(resolution != ""){
        if(ui->ExactCheckBox->isChecked()){
            request += "&resolution=" + resolution;
        }else{
            request += "&atleast=" + resolution;
        }
    }

    QString ratio = ui->RatioInput->text(); if(ratio != "") request += "&ratios=" + ratio;
    request += "&page=" + QString::number(page_number);

    return request;
}

void MainWindow::GetPage(int page_number, int retry_count) {
    if(retry_count >= m_max_retry_count) return;

    ui->PageNumberInput->setText(QString::number(m_page_number));

    QString request = GetRequestFromUI(page_number);

    if(m_pages[page_number].url == request && retry_count == 0){
        if(page_number == m_page_number){
            for(int i = 0; i < 24; i++){
                ui->DownloadButtons->buttons().at(i)->setIcon(m_pages[page_number].icon[i]);
                ui->DownloadButtons->buttons().at(i)->setToolTip(m_pages[page_number].json["data"][i]["path"].toString());
            }
        }
    }else{
        qDebug() << "Fetched" << request;
        m_pages[page_number].url = QUrl::fromUserInput(request);

        m_threadpool.start([=](){
            QNetworkAccessManager manager;
            QNetworkReply* reply = manager.get(QNetworkRequest(QNetworkRequest(QUrl::fromUserInput(request))));

            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            if(reply->error() && retry_count < 5){
                QThread::sleep(5);
                qDebug() << reply->error() << retry_count << reply->url();
                GetPage(page_number, retry_count + 1);
                return;
            }

            QString answer = reply->readAll();

            m_pages[page_number].url = reply->url();
            m_pages[page_number].json = QJsonDocument::fromJson(answer.toUtf8());
            m_last_page_number = m_pages[page_number].json["meta"]["last_page"].toInt();

            ui->LastPageNumberLabel->setText(QString::number(m_last_page_number));

            if(m_page_number > m_last_page_number){
                return;
            }

            for(int i = 0; i < 24; i++){
                m_pages[page_number].icon[i] = DownloadThumbnail(QUrl::fromUserInput(m_pages[page_number].json["data"][i]["thumbs"]["small"].toString()), &manager, page_number);
                if(page_number == m_page_number){
                    ui->DownloadButtons->buttons().at(i)->setIcon(m_pages[page_number].icon[i]);
                    ui->DownloadButtons->buttons().at(i)->setToolTip(m_pages[page_number].json["data"][i]["path"].toString());
                }
            }
        });
    }

    if(m_page_number + ui->PagePrefetchAmountInput->text().toInt() > page_number){
        GetPage(page_number + 1);
    }
}

QIcon MainWindow::DownloadThumbnail(QUrl url, QNetworkAccessManager* manager, int page_number, int retry_count){
    if(retry_count > 5) return m_default_icon;

    QNetworkReply* reply = manager->get(QNetworkRequest(url));

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if(reply->error()){
        QThread::sleep(2);
        qDebug() << reply->error() << retry_count << reply->url();
        return DownloadThumbnail(url, manager, retry_count + 1);
    }

    auto data = reply->readAll();

    QPixmap p;
    p.loadFromData(data);
    return QIcon(p);
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
                     +ui->OrderComboBox->currentText()+"\", \"exact_resolution\": \""
                     +isChecked(ui->ExactCheckBox)+"\", \"page_prefetch_amount\": \""
                     +ui->PagePrefetchAmountInput->text()+"\"}";

    QJsonDocument config_json = QJsonDocument::fromJson(output.toUtf8());

    QFile config_file(QDir::homePath() + "/.config/wallhaven/config.json");
    if(config_file.open(QIODevice::WriteOnly)){
        config_file.write(config_json.toJson());
        config_file.close();
    }

    ui->Tabs->setCurrentIndex(0);
}

void MainWindow::DownloadImage(QUrl url, QString destination, int retry_count) {
    QString output_file = destination + url.toString().split("/").last();
    QFile image_file(output_file);
    if(image_file.exists()) return;

    m_threadpool.start([=](){
            QNetworkAccessManager manager;
            QNetworkReply* reply = manager.get(QNetworkRequest(url));

            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            if(reply->error() && retry_count < 5){
                QThread::sleep(5);
                qDebug() << reply->error() << retry_count << reply->url();
                DownloadImage(url, destination, retry_count + 1);
                return;
            }

            auto data = reply->readAll();
            QPixmap image;
            image.loadFromData(data);
            if(image.save(output_file, nullptr, 100)) qDebug() << "File successfully saved at:" << output_file;
    });
}
