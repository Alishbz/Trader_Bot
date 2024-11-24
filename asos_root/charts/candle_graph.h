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
#ifndef CANDLE_GRAPH_H
#define CANDLE_GRAPH_H


#include <QWidget>
#include <QtCharts>
#include <QRandomGenerator>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QVBoxLayout>
#include <QDateTime>
#include <QCoreApplication>
#include <QTimer>

#include "../generals/ohlc.h"
#include "../custom_widgets/as_candle_chart.h"


#define MIN_TIMESTEMP (-3600*4)



// @line selection
enum class candle_line_select_e{
    LINE_1, LINE_2, LINE_3, MAX_SIZE
};


class candle_graph_c : public QWidget {
    Q_OBJECT
public:
    candle_graph_c(QWidget *_parent = nullptr ,
                   const QString & _candle_name = "asos_candle",
                   const std::array<QString, static_cast<size_t>(candle_line_select_e::MAX_SIZE)>& line_series_names_arr = {},
                   int _candle_view_max_show_datas = 30 ,
                   double _candle_view_thresold = 1.00) :
        QWidget(_parent),
        candle_view_max_show_datas(_candle_view_max_show_datas),
        candle_view_thresold(_candle_view_thresold),
        candle_series(new QCandlestickSeries(this)),
        layout(new QVBoxLayout(this)),
        timer(nullptr)
    {
        graph_update_count = 0;
        graph_update_signal = false;
        chart.setTitle(_candle_name);
        chart.setAnimationOptions(QChart::SeriesAnimations);

        candle_series->setName("Candle Prices");
        candle_series->setIncreasingColor(QColor(Qt::green));
        candle_series->setDecreasingColor(QColor(Qt::red));

        chart.addSeries(candle_series);

        chart.setBackgroundBrush(QBrush(Qt::lightGray));

        axisX = new QDateTimeAxis(this);
        axisX->setFormat("hh:mm:ss");
        axisX->setTitleText("Time");
        chart.addAxis(axisX, Qt::AlignBottom);
        candle_series->attachAxis(axisX);

        axisY = new QValueAxis(this);
        axisY->setTitleText("Price");
        chart.addAxis(axisY, Qt::AlignLeft);
        candle_series->attachAxis(axisY);

        chart_view = new as_candle_chart(&chart, this);
        chart_view->setRenderHint(QPainter::Antialiasing);
        //chart_view->setRubberBand(QChartView::RectangleRubberBand);
        chart_view->setDragMode(QGraphicsView::ScrollHandDrag);
        chart_view->setMouseTracking(true);

        layout->addWidget(chart_view);
        setLayout(layout);

        for(int i = 0 ; i < (size_t)candle_line_select_e::LINE_3+1; i++)
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

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &candle_graph_c::graph_updater_timer_callback);
        timer->start(250);
    }

    void graph_updater_timer_callback(){
        graph_update_count++;
        if(graph_update_signal && graph_update_count > 2){
            graph_update_count = 0;
            chart_view->chart()->zoomReset();
            chart_view->chart()->update();
            graph_update_signal = false;
        }
    }

    void clear_chart(){
        for(int i = 0 ; i < (size_t)candle_line_select_e::LINE_3+1; i++)
        {
            if (line_series[i]) {
                line_series[i]->clear();
            }
        }

        if(candle_series)
            candle_series->clear();

        axisY->setMin(0);
        axisY->setMax(0);

        chart_view->clear_datas();
    }

    void point_chart(const QString &text, const QColor &color, const QPointF &point = QPointF()) {
         chart_view->add_point_with_annotation(text,color,point); // todo
    }

public slots:

    void OHLC_update(const OHLC_s& val, qint64 _timestamp = 0){
        double open = val.open;
        double high = val.high;
        double low = val.low;
        double close = val.close;
        _OHLC_update(open,high,low,close , _timestamp);
    }

    void line_update(candle_line_select_e  selected_line , double data, qint64 _timestamp = 0){

        QLineSeries * series = line_series[(int)selected_line];

        if(series){

            QDateTime timestamp{};

            if(_timestamp == 0){
                timestamp = QDateTime::currentDateTime();
            }
            else{
                timestamp = QDateTime::fromMSecsSinceEpoch(_timestamp);  // @ref: convert_timestamp_to_string
            }

            series->append( timestamp.toMSecsSinceEpoch() , (qreal)data);

            int size_line = series->count();
            if (size_line > candle_view_max_show_datas) {
                series->remove(0);  // todo buda performans düşürebilir
            }

            axisX->setMin(timestamp.addSecs(MIN_TIMESTEMP));
            axisX->setMax(timestamp);

            // new feature, line cold be zoom in chart view
            {

                double currentMin = axisY->min();
                double currentMax = axisY->max();

                if (data < currentMin ) {
                    axisY->setMin(data - candle_view_thresold);
                }

                if (data > currentMax ) {
                    axisY->setMax(data + candle_view_thresold);
                }

                if (axisY->min() > axisY->max()) {
                    axisY->setMin(axisY->max() - candle_view_thresold);
                }
            }

            graph_update_signal = true;
            graph_update_count = 0;

            // performansı düşürüyor
            //chart_view->chart()->zoomReset();
            //chart_view->chart()->update();
        }
    }

private:

    void _OHLC_update(double open,double high , double low , double close , qint64 _timestamp = 0) {

        QDateTime timestamp{};

        if(_timestamp == 0){
            timestamp = QDateTime::currentDateTime();
        }
        else{
            timestamp = QDateTime::fromMSecsSinceEpoch(_timestamp);  // @ref: convert_timestamp_to_string
        }

        QCandlestickSet *set = new QCandlestickSet(timestamp.toMSecsSinceEpoch());
        set->setOpen(open);
        set->setHigh(high);
        set->setLow(low);
        set->setClose(close);
        candle_series->append(set);

        int size_candle = candle_series->count() ;
        if (size_candle> candle_view_max_show_datas) {
            QList<QCandlestickSet *> list_x = candle_series->sets();
            QCandlestickSet *oldestSet = list_x.first();
            if(oldestSet){
                candle_series->remove(oldestSet); // todo buda performans düşürebilir
            }
        }

        //QDateTimeAxis *_axisX = static_cast<QDateTimeAxis *>(chart.axes(Qt::Horizontal).at(0));
        axisX->setMin(timestamp.addSecs(MIN_TIMESTEMP));//axisX->setMin(timestamp.addSecs(-candle_view_max_show_datas));
        axisX->setMax(timestamp);

        double currentMin = axisY->min();
        double currentMax = axisY->max();

        if (currentMin < (0+candle_view_thresold)  || currentMin > low) {
            axisY->setMin(low - candle_view_thresold);
        } else {
            axisY->setMin(std::min(currentMin, low - candle_view_thresold));
        }

        if (currentMax < (0+candle_view_thresold) || currentMax < high) {
            axisY->setMax(high + candle_view_thresold);
        } else {
            axisY->setMax(std::max(currentMax, high + candle_view_thresold));
        }

        graph_update_signal = true;
        graph_update_count = 0;

        // performansı düşürüyor
        // chart_view->chart()->zoomReset();
        // chart_view->chart()->update();
    }

private:

    bool graph_update_signal;
    int graph_update_count;

    QChart chart;
    as_candle_chart *chart_view;
    QCandlestickSeries *candle_series;
    QVBoxLayout *layout;
    QTimer *timer;
    QValueAxis *axisY;
    QDateTimeAxis *axisX;
    QLineSeries *line_series[(size_t)candle_line_select_e::MAX_SIZE];

    const int candle_view_max_show_datas ;
    const double candle_view_thresold;
};




#endif // CANDLE_GRAPH_H
