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
#ifndef AS_CANDLE_CHART_H
#define AS_CANDLE_CHART_H


#include <QWidget>
#include <QtCharts>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>


class as_candle_chart : public QChartView {
public:
    as_candle_chart(QChart *chart, QWidget *parent = nullptr)
        : QChartView(chart, parent), last_mouse_pos{} {
        is_dragging=false;
    }

    void clear_datas(){
        for (auto series : scatter_series_vector_) {
            chart()->removeSeries(series);
            delete series;
        }
        scatter_series_vector_.clear();

        for (auto annotation : annotation_vector_) {
            chart()->scene()->removeItem(annotation);
            delete annotation;
        }
        annotation_vector_.clear();
    }

    // todo, grafik shift edilerek veri yazılırken nasıl silicez en son BUT SELL notunu ?

    void add_point_with_annotation(const QString &text, const QColor &color, const QPointF &point = QPointF()) {
        QPointF actual_point;

        if (point.isNull()) {
            // Grafikteki son veri noktasını bul
            auto series_list = chart()->series();
            if (!series_list.isEmpty()) {
                auto series = series_list.first();  // İlk seri kullanılabilir, buna göre değiştirebilirsiniz
                if (auto scatter_series = dynamic_cast<QScatterSeries *>(series)) {
                    if (!scatter_series->points().isEmpty()) {
                        QList<QPointF> tt =  scatter_series->points();
                        actual_point = tt.last();
                       // actual_point = scatter_series->points().last();  // En son noktayı al
                    } else {
                        actual_point = QPointF(0, 0);
                        return;
                    }
                } else if (auto candlestick_series = dynamic_cast<QCandlestickSeries *>(series)) {
                    if (!candlestick_series->sets().isEmpty()) {
                        QList<QCandlestickSet *> tt =  candlestick_series->sets();
                        auto set = tt.last();
                        //auto set = candlestick_series->sets().last();
                        actual_point = QPointF(set->timestamp(), (set->high() + set->low()) / 2);  // Ortada bir yer
                    } else {
                        actual_point = QPointF(0, 0);
                        return;
                    }
                }
            } else {
                actual_point = QPointF(0, 0);
                return;
            }
        } else {
            actual_point = point;
        }

        // Scatter serisini oluştur ve özelleştir
        QScatterSeries *scatter_series = new QScatterSeries(this);
        scatter_series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        scatter_series->setMarkerSize(10.0);
        scatter_series->setColor(color);
        scatter_series->append(actual_point);

        // Annotation (yazı) oluştur
        QGraphicsSimpleTextItem *annotation = new QGraphicsSimpleTextItem(text);
        annotation->setBrush(color);
        // Not: 'chart()->mapToPosition' fonksiyonu ile 'actual_point' konumunu grafikteki konumuna dönüştür
        annotation->setPos(chart()->mapToPosition(actual_point) + QPointF(0, 10)); // 10 piksel aşağıda göster

        // Scatter serisini grafiğe ekle
        chart()->addSeries(scatter_series);
        scatter_series_vector_.append(scatter_series);

        // Scatter serisinin eksenlere bağlı olduğundan emin ol
        for (auto axis : chart()->axes(Qt::Horizontal)) {
            scatter_series->attachAxis(axis);
        }
        for (auto axis : chart()->axes(Qt::Vertical)) {
            scatter_series->attachAxis(axis);
        }

        // Annotation'ı grafiğin sahnesine ekle
        chart()->scene()->addItem(annotation);
        annotation_vector_.append(annotation);
    }
    /*
    void add_point_with_annotation(const QString &text, const QColor &color, const QPointF &point = QPointF()) {  // todo optimize et
        QPointF actualPoint;
        if (point.isNull()) {
            // Grafikteki son veri noktasını bul
            auto seriesList = chart()->series();
            if (!seriesList.isEmpty()) {
                auto series = seriesList.first();  // İlk seri kullanılabilir, buna göre değiştirebilirsiniz
                if (auto scatterSeries = dynamic_cast<QScatterSeries *>(series)) {
                    if (!scatterSeries->points().isEmpty()) {
                        QList<QPointF> tt =  scatterSeries->points();
                        actualPoint = tt.last();  // En son noktayı al
                    } else {
                        // Veri serisi boşsa, varsayılan bir konum kullan
                        actualPoint = QPointF(0, 0);
                        return;
                    }
                } else if (auto candlestickSeries = dynamic_cast<QCandlestickSeries *>(series)) {
                    if (!candlestickSeries->sets().isEmpty()) {
                        QList<QCandlestickSet *> tt =  candlestickSeries->sets();
                        auto set = tt.last();
                        actualPoint = QPointF(set->timestamp(), (set->high() + set->low()) / 2);  // Ortada bir yer
                    } else {
                        // Veri serisi boşsa, varsayılan bir konum kullan
                        actualPoint = QPointF(0, 0);
                        return;
                    }
                }
            } else {
                // Seri listesi boşsa, varsayılan bir konum kullan
                actualPoint = QPointF(0, 0);
                return;
            }
        } else {
            actualPoint = point;
        }

        // Scatter serisini oluştur ve özelleştir
        QScatterSeries *scatterSeries = new QScatterSeries(this);// todo bunu vectorde tut sonra sil
        scatterSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
        scatterSeries->setMarkerSize(10.0);
        scatterSeries->setColor(color);
        scatterSeries->append(actualPoint);  // Noktayı seriye ekle

        // Annotation (yazı) oluştur
        QGraphicsSimpleTextItem *annotation = new QGraphicsSimpleTextItem(text);// todo bunu vectorde tut sonra sil
        annotation->setBrush(color);
        // Not: 'chart()->mapToPosition' fonksiyonu ile 'actualPoint' konumunu grafikteki konumuna dönüştür
        annotation->setPos(chart()->mapToPosition(actualPoint) + QPointF(0, 10)); // 10 piksel aşağıda göster

        // Scatter serisini grafiğe ekle
        chart()->addSeries(scatterSeries);

        // Scatter serisinin eksenlere bağlı olduğundan emin ol
        for (auto axis : chart()->axes(Qt::Horizontal)) {
            scatterSeries->attachAxis(axis);
        }
        for (auto axis : chart()->axes(Qt::Vertical)) {
            scatterSeries->attachAxis(axis);
        }

        // Annotation'ı grafiğin sahnesine ekle
        chart()->scene()->addItem(annotation);
    }*/


protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->angleDelta().y() > 0){
            chart()->zoomIn();
            rescale_candlestick_chart();
        }
        else{
            chart()->zoomOut();
            rescale_candlestick_chart();
        }
        QChartView::wheelEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            is_dragging = true;
            last_mouse_pos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        QChartView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (is_dragging) {
            const qreal moveFactor = 0.5;
            QPointF delta = event->pos() - last_mouse_pos;
            chart()->scroll(-delta.x() * moveFactor, delta.y() * moveFactor);
            last_mouse_pos = event->pos();
        }
        QChartView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            is_dragging = false;
            setCursor(Qt::ArrowCursor);
        }
        QChartView::mouseReleaseEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
            event->acceptProposedAction();
        }
        QChartView::dragEnterEvent(event);
    }

    void dropEvent(QDropEvent *event) override {
        if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        }
        QChartView::dropEvent(event);
    }
private:

    void rescale_candlestick_chart() {
        QChart *chart = this->chart();
        QCandlestickSeries *candlestickSeries = nullptr;

        // Candlestick serisini bulun
        for (QAbstractSeries *series : chart->series()) {
            if (auto cs = qobject_cast<QCandlestickSeries *>(series)) {
                candlestickSeries = cs;
                break;
            }
        }

        if (!candlestickSeries) {
            return; // Candlestick serisi yoksa çık
        }

        // En büyük ve en küçük değerleri bul
        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxY = std::numeric_limits<qreal>::min();

        for (QCandlestickSet *set : candlestickSeries->sets()) {
            minY = std::min(minY, set->low());
            maxY = std::max(maxY, set->high());
        }

        // Y eksenini yeniden ölçeklendir
        QList<QAbstractAxis *> axes = chart->axes(Qt::Vertical);
        for (QAbstractAxis *axis : axes) {
            if (auto valueAxis = qobject_cast<QValueAxis *>(axis)) {
                valueAxis->setMin(minY);
                valueAxis->setMax(maxY);
            }
        }
    }

    bool is_dragging=false;
    QPoint last_mouse_pos;

    QVector<QScatterSeries *> scatter_series_vector_;
    QVector<QGraphicsSimpleTextItem *> annotation_vector_;
};


#endif
