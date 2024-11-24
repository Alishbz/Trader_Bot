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
#ifndef LINE_GRAPH_H
#define LINE_GRAPH_H

#include <QWidget>
#include <QtCharts>
#include <QRandomGenerator>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QVBoxLayout>

enum class line_name_select_e{
    LINE_1,LINE_2,LINE_3,MAX_SIZE
};

class line_graph_c : public QWidget
{
    Q_OBJECT
public:
    explicit line_graph_c(QWidget *parent = nullptr,
                          const QString & _chart_name = "asos_line",
                          const std::array<QString, static_cast<size_t>(line_name_select_e::MAX_SIZE)>& line_series_names_arr = {},
                          int _candle_view_max_show_datas = 30 ,
                          double _candle_view_thresold = 2.00 )
        : QWidget(parent),
        candle_view_max_show_datas(_candle_view_max_show_datas),
        candle_view_thresold(_candle_view_thresold),
        chart_view(new QChartView(&chart, this))
    {
        chart.setTitle(_chart_name);
        chart.setAnimationOptions(QChart::SeriesAnimations);

        axisX = new QDateTimeAxis(this);
        axisX->setFormat("hh:mm:ss");
        axisX->setTitleText("Time");
        chart.addAxis(axisX, Qt::AlignBottom);

        axisY = new QValueAxis(this);
        axisY->setTitleText("Price");
        chart.addAxis(axisY, Qt::AlignLeft);

        chart.setBackgroundBrush(QBrush(Qt::lightGray));

        chart_view->setRenderHint(QPainter::Antialiasing);

        for(int i = 0 ; i < (size_t)line_name_select_e::MAX_SIZE; i++)
        {
            line_series[i] = new QLineSeries(&chart);
            switch (i ) {
            case 0:
                line_series[i]->setColor(Qt::yellow);
                break;
            case 1:
                line_series[i]->setColor(Qt::darkMagenta);
                break;
            case 2:
                line_series[i]->setColor(Qt::blue);
                break;
            default:
                line_series[i]->setColor(Qt::darkGray);
                break;
            }
            if(line_series_names_arr[i].isEmpty())
                line_series[i]->setName(QString("Line %1").arg(i + 1));
            else
                line_series[i]->setName(line_series_names_arr[i]);
            chart.addSeries(line_series[i]);
            line_series[i]->attachAxis(axisX);
            line_series[i]->attachAxis(axisY);
        }

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(chart_view);
        setLayout(layout);

    }

    void clear_chart(){
        for(int i = 0 ; i < (size_t)line_name_select_e::MAX_SIZE; i++)
        {
            if (line_series[i]) {
                line_series[i]->clear();
            }
        }
    }

public slots:

    void line_update(line_name_select_e  selected_line , double data){

        QLineSeries * series = line_series[(int)selected_line];

        if(series){

            QDateTime timestamp = QDateTime::currentDateTime();

            series->append( timestamp.toMSecsSinceEpoch() , (qreal)data);

            QDateTimeAxis *axisX = static_cast<QDateTimeAxis *>(chart.axes(Qt::Horizontal).at(0));
            axisX->setMin(timestamp.addSecs(-candle_view_max_show_datas));
            axisX->setMax(timestamp);

            double currentMin = axisY->min();
            double currentMax = axisY->max();
             /*
            if (currentMin < (0.1)  || currentMin > data || currentMax < data) {
                axisY->setMin(data);
            }
            */

            if (data < currentMin ) {
                axisY->setMin(data - candle_view_thresold);
            }

            if (data > currentMax ) {
                axisY->setMax(data + candle_view_thresold);
            }

            // Ensure the Y-axis min is not greater than the max
            if (axisY->min() > axisY->max()) {
                axisY->setMin(axisY->max() - candle_view_thresold);
            }

            chart_view->chart()->zoomReset();
            chart_view->chart()->update();
        }
    }

private:
    QChart chart;
    QValueAxis *axisY;
    QDateTimeAxis *axisX;
    QChartView *chart_view;
    QLineSeries *line_series[(size_t)line_name_select_e::MAX_SIZE];
    const int candle_view_max_show_datas ;
    const double candle_view_thresold;
};


#endif // LINE_GRAPH_H
