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
#ifndef AS_PROGRESSBAR_H
#define AS_PROGRESSBAR_H

#include <QWidget>
#include <QProgressBar>
#include <QPainter>
#include <QColor>

class as_progressbar : public QProgressBar
{
    Q_OBJECT

public:
    explicit as_progressbar(QWidget *parent = nullptr , QColor _leftColor = Qt::green, QColor _rightColor = Qt::red )   :
        QProgressBar(parent),
        leftColor(_leftColor),
        rightColor(_rightColor)
    {
        setTextVisible(true);
    }

    void setValue(int value)
    {
        QProgressBar::setValue(value);
        update(); // for paintEvent
    }

protected:

    // todo setValue yi overload et , ilgili setValue ile beraber progressbar üzerindeki textbax da değeri de güncellensin

    void paintEvent(QPaintEvent *event) override
    {
        QProgressBar::paintEvent(event);

        QPainter painter(this);
        QRect rect = this->rect();

        double progressRatio = value() / static_cast<double>(maximum());
        QColor fillColor = leftColor;
        QColor backgroundColor = rightColor;

        QRect filledRect = rect;
        filledRect.setWidth(rect.width() * progressRatio);

        painter.fillRect(filledRect, fillColor);

        painter.fillRect(rect.adjusted(filledRect.width(), 0, 0, 0), backgroundColor);

        painter.setPen(Qt::black); // Metin rengi
        painter.setFont(QFont("Arial", 10)); // Metin fontu ve boyutu
        painter.drawText(rect, Qt::AlignCenter, QString::number(value()));
    }

private:
    QColor leftColor;
    QColor rightColor;
};


#endif
