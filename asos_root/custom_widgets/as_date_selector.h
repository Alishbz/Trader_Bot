/******************************************************************************
 * (ASOS TBP) Trader Bot Project
 *
 * Author: Ali Şahbaz
 * Company: ME
 * Date: 16.06.2024
 * Mail: ali_sahbaz@outlook.com
 * Project Name: Binance Trader app
 *
 *****************************************************************************/
#ifndef AS_DATE_SELECTOR_H
#define AS_DATE_SELECTOR_H

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QDateEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QDateTime>
#include <QDialog>
#include <QGridLayout>
#include <QListView>
#include <QLineEdit>
#include <QTime>
#include <QStandardItemModel>
#include <QStandardItem>

/*
class as_date_selector : public QWidget {
    Q_OBJECT

public:
    as_date_selector(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        dateTimeEdit = new QDateTimeEdit(this);
        dateTimeEdit->setDisplayFormat("yyyy-MM-ddTHH:mm:ss"); // ISO 8601 formatı
        dateTimeEdit->setCalendarPopup(true); // Takvim popup'ı açılmasını sağlar
        layout->addWidget(dateTimeEdit);

        QPushButton *submitButton = new QPushButton("Submit", this);
        layout->addWidget(submitButton);

        dateTimeLabel = new QLabel("Selected Date and Time:", this);
        layout->addWidget(dateTimeLabel);

        connect(submitButton, &QPushButton::clicked, this, &as_date_selector::onSubmit);
    }

private slots:
    void onSubmit() {
        QDateTime dateTime = dateTimeEdit->dateTime();
        QString dateTimeString = dateTime.toString(Qt::ISODate);
        dateTimeLabel->setText("Selected Date and Time: " + dateTimeString);
    }

private:
    QDateTimeEdit *dateTimeEdit;
    QLabel *dateTimeLabel;
};
*/


class TimePopup : public QDialog {
    Q_OBJECT

public:
    TimePopup(QWidget *parent = nullptr) : QDialog(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        timeListView = new QListView(this);
        timeListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        layout->addWidget(timeListView);

        QStandardItemModel *model = new QStandardItemModel(this);
        for (int hour = 0; hour < 24; ++hour) {
            for (int minute = 0; minute < 60; minute += 5) { // 5 dakikalık aralıklarla
                QTime time(hour, minute);
                QStandardItem *item = new QStandardItem(time.toString("HH:mm"));
                model->appendRow(item);
            }
        }
        timeListView->setModel(model);

        connect(timeListView, &QListView::clicked, this, &TimePopup::onTimeSelected);
    }

    QTime getSelectedTime() const {
        return selectedTime;
    }

private slots:
    void onTimeSelected(const QModelIndex &index) {
        selectedTime = QTime::fromString(index.data().toString(), "HH:mm");
        accept();
    }

private:
    QListView *timeListView;
    QTime selectedTime;
};

class as_date_selector : public QWidget {
    Q_OBJECT

public:
    as_date_selector(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        dateEdit = new QDateEdit(this);
        dateEdit->setDisplayFormat("yyyy-MM-dd"); // ISO 8601 tarih formatı
        dateEdit->setCalendarPopup(true); // Takvim popup'ı açılmasını sağlar
        layout->addWidget(dateEdit);

        QPushButton *timeButton = new QPushButton("Select Time", this);
        layout->addWidget(timeButton);

        timeEdit = new QLineEdit(this);
        timeEdit->setReadOnly(true);
        layout->addWidget(timeEdit);

        QPushButton *submitButton = new QPushButton("Submit", this);
        layout->addWidget(submitButton);

        dateTimeLabel = new QLabel("Selected Date and Time:", this);
        layout->addWidget(dateTimeLabel);

        connect(timeButton, &QPushButton::clicked, this, &as_date_selector::onSelectTime);
        connect(submitButton, &QPushButton::clicked, this, &as_date_selector::onSubmit);
    }

    QString get_date() const {
        QDateTime dateTime(dateEdit->date(), QTime::fromString(timeEdit->text(), "HH:mm"));
        return dateTime.toString(Qt::ISODate);
    }

private slots:
    void onSelectTime() {
        TimePopup timePopup(this);
        if (timePopup.exec() == QDialog::Accepted) {
            QTime selectedTime = timePopup.getSelectedTime();
            timeEdit->setText(selectedTime.toString("HH:mm"));
        }
    }

    void onSubmit() {
        QString dateTimeString = get_date();
        dateTimeLabel->setText("Selected Date and Time: " + dateTimeString);
    }

private:
    QDateEdit *dateEdit;
    QLineEdit *timeEdit;
    QLabel *dateTimeLabel;
};


#endif
