#ifndef LOGGER_PAGE_H
#define LOGGER_PAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>

class logger_page_c : public QWidget {
    Q_OBJECT

public:
    logger_page_c(QWidget *parent = nullptr):
        QWidget(parent)
    {
        QVBoxLayout *main_layout = new QVBoxLayout(this);

        QLabel *log_label = new QLabel("LOG WATCHING", this);
        log_label->setStyleSheet("QLabel { color : red; font-size: 24px; }");
        log_label->setAlignment(Qt::AlignCenter);

        log_text_edit = new QTextEdit(this);
        log_text_edit->setReadOnly(true);

        QPushButton *clear_button = new QPushButton("Clear Log", this);
        connect(clear_button, &QPushButton::clicked, log_text_edit, &QTextEdit::clear);

        main_layout->addWidget(log_label);
        main_layout->addWidget(log_text_edit);
        main_layout->addWidget(clear_button);

        setLayout(main_layout);
    }

public slots:
    void log_print(const QString &message){
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        QString timestamped_message = "<font color=\"red\">" + timestamp + "</font>: " + message;
        log_text_edit->append(timestamped_message);
    }


private:
    QTextEdit *log_text_edit;
};


#endif // LOGGER_PAGE_H
