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

#ifndef BINANCE_WEB_SOCKET_H
#define BINANCE_WEB_SOCKET_H

#include <QWidget>
#include <QWebSocket>
#include <QUrl>
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <QObject>
#include <QUrlQuery>
#include <QtNetwork>
#include <QCoreApplication>
#include <QTimer>

#include "../account/coin_list.h"

class binance_web_socket_c : public QWidget
{
    Q_OBJECT
public:

    explicit binance_web_socket_c(QWidget *parent = nullptr ,
                                  coin_select_e _selected_coin = coin_select_e::BTC ,
                                  const QString& _listen_key = "")
        : QWidget(parent) , selected_coin(_selected_coin), listen_key(_listen_key)
    {
        connect(&depth_web_socket, &QWebSocket::connected, this, &binance_web_socket_c::depth_web_socket_connected);
        connect(&depth_web_socket, &QWebSocket::disconnected, this, &binance_web_socket_c::depth_web_socket_disconnected);
        connect(&depth_web_socket, &QWebSocket::textMessageReceived, this, &binance_web_socket_c::depth_web_socket_receive);

        connect(&trade_web_socket, &QWebSocket::connected, this, &binance_web_socket_c::trade_web_socket_connected);
        connect(&trade_web_socket, &QWebSocket::disconnected, this, &binance_web_socket_c::trade_web_socket_disconnected);
        connect(&trade_web_socket, &QWebSocket::textMessageReceived, this, &binance_web_socket_c::trade_web_socket_receive);

        connect(&user_stream_web_socket, &QWebSocket::connected, this, &binance_web_socket_c::user_stream_web_socket_connected);
        connect(&user_stream_web_socket, &QWebSocket::disconnected, this, &binance_web_socket_c::user_stream_web_socket_disconnected);
        connect(&user_stream_web_socket, &QWebSocket::textMessageReceived, this, &binance_web_socket_c::user_stream_web_socket_receive);

        depth_socket_connection_state = false;
        trade_socket_connection_state = false;
        user_stream_web_socket_connection_state = false;

        // START
        //trade_web_socket_start(selected_coin);
        //user_stream_web_socket_start();
        //depth_web_socket_start();
    }

    /*** trade data stream interface ***********/


    bool trade_web_socket_is_connected()const{
        return trade_socket_connection_state;
    }

    coin_select_e get_selected_coin()const{
        return selected_coin;
    }

    void  trade_web_socket_start(coin_select_e _selected_coin = coin_select_e::EMPTY){
        if(_selected_coin != coin_select_e::EMPTY)
            start_coin_trade(_selected_coin);
        else
            start_coin_trade(selected_coin);
    }

    void trade_web_socket_close(){
        trade_web_socket.close();
    }


    /*** depth data stream interface ***********/


    bool depth_web_socket_is_connected()const{
        return depth_socket_connection_state;
    }

    void  depth_web_socket_start(coin_select_e _selected_coin = coin_select_e::EMPTY){
        if(depth_socket_connection_state == false){
            if(_selected_coin != coin_select_e::EMPTY){
                _start_depth_connection(coin_stream_map_get_enum_to_str(_selected_coin));
            }
            else if(selected_coin != coin_select_e::EMPTY){
                _start_depth_connection(coin_stream_map_get_enum_to_str(selected_coin));
            }
        }
    }

    void depth_web_socket_close(){
        depth_web_socket.close();
    }



    /*** user data stream interface ***********/

    void  user_stream_web_socket_start(const QString& _listen_key = "" ){

        if(!_listen_key.isEmpty()){

            listen_key = _listen_key;

            _start_user_stream_connection();
        }
        else if(!listen_key.isEmpty()){

            _start_user_stream_connection();
        }
    }

    bool  user_stream_web_socket_is_connected()const{
        return  user_stream_web_socket_connection_state;
    }

    void user_stream_web_socket_close(){

        user_stream_web_socket.close();
    }

    void set_listen_key(const QString& _listen_key){
        listen_key = _listen_key;
    }

private:

    void start_coin_trade(coin_select_e _selected_coin){

        const QString& coin = coin_stream_map_get_enum_to_str(_selected_coin);
        if(!coin.isEmpty()){
            _start_coin_trade_connection(coin);
            selected_coin = _selected_coin;
        } else {
            qDebug() << "Invalid coin selected";
        }

    }

signals:
    void log_print(const QString &message);
    void coin_price_update(double data);
    void depth_bids_ratio_update(double bids_ratio); // asks_ratio = 1 - bids_ratio , If you multiply it by 100 it becomes a percentage
    void coin_price_quantity_volume(double data);

private slots:

    /**** trade web socket - live datas ***************/

    void trade_web_socket_connected()
    {
        trade_socket_connection_state = true;

        qDebug() << "trade_web_socket connected";
    }

    void trade_web_socket_disconnected()
    {
        trade_socket_connection_state = false;

        qDebug() << "trade_web_socket disconnected";
    }




    void trade_web_socket_receive(const QString &message)
    {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        // price
        if (jsonObj.contains("p")) {
            QJsonValue pValue = jsonObj.value("p");

            if (pValue.isString()) {
                QString pStr = pValue.toString().trimmed();
                bool ok;
                double pDouble = pStr.toDouble(&ok);
                if (ok) {
                    emit coin_price_update(pDouble) ;
                } else {
                    qWarning() << "Conversion error for 'p'!";
                }
            } else {
                qWarning() << "'p' value is not a string!";
            }
        } else {
            qWarning() << "'p' value not found!";
        }

        // hacim
        if (jsonObj.contains("q")) {
            QJsonValue qValue = jsonObj.value("q");
            if (qValue.isString()) {
                QString qStr = qValue.toString().trimmed();
                bool ok;
                double qDouble = qStr.toDouble(&ok);
                if (ok) {
                    emit coin_price_quantity_volume(qDouble) ;
                } else {
                    qWarning() << "Conversion error for 'q'!";
                }
            } else {
                qWarning() << "'q' value is not a string!";
            }
        } else {
            qWarning() << "'q' value not found!";
        }
    }



    /*** user_stream *************/


    void user_stream_web_socket_connected()
    {
        user_stream_web_socket_connection_state = true;

        qDebug() << "user_stream_web_socket connected";
    }

    void user_stream_web_socket_disconnected()
    {
        selected_coin = coin_select_e::EMPTY;

        user_stream_web_socket_connection_state = false;

        qDebug() << "user_stream_web_socket disconnected";
    }

    void user_stream_web_socket_receive(const QString &message)
    {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        QString jsonString = QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
        emit log_print("USER DATA STREAM: " + jsonString);
    }



    /** depth ******************/

    void depth_web_socket_connected()
    {
        depth_socket_connection_state = true;

        qDebug() << "depth_web_socket connected";
    }

    void depth_web_socket_disconnected()
    {
        depth_socket_connection_state = false;

        qDebug() << "depth_web_socket disconnected";
    }

    void depth_web_socket_receive(const QString &message)
    {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8(), &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        QList<QPair<double, double>> bidsList;
        QList<QPair<double, double>> asksList;

        // Alış emirleri
        double bids_total_money_deposited_by_customers = 0;
        QJsonArray bidsArray = jsonObj["b"].toArray();
        for (const QJsonValue &bidValue : bidsArray) {
            QJsonArray bidArray = bidValue.toArray();
            double price = bidArray[0].toString().toDouble();
            double quantity = bidArray[1].toString().toDouble();
            bidsList.append(qMakePair(price, quantity));
            bids_total_money_deposited_by_customers+=(price*quantity);
        }

        // Satış emirleri
        double asks_total_money_deposited_by_customers = 0;
        QJsonArray asksArray = jsonObj["a"].toArray();
        for (const QJsonValue &askValue : asksArray) {
            QJsonArray askArray = askValue.toArray();
            double price = askArray[0].toString().toDouble();
            double quantity = askArray[1].toString().toDouble();
            asksList.append(qMakePair(price, quantity));
            asks_total_money_deposited_by_customers+=(price*quantity);
        }

        double bids_weight_ratio  = (bids_total_money_deposited_by_customers / (asks_total_money_deposited_by_customers + bids_total_money_deposited_by_customers));

        emit depth_bids_ratio_update(bids_weight_ratio);

        //double asks_weight_ratio  = (asks_total_money_deposited_by_customers / (asks_total_money_deposited_by_customers + bids_total_money_deposited_by_customers));

        //qDebug() << "bids_weight_ratio: " << bids_weight_ratio << " asks_weight_ratio: " << asks_weight_ratio;

        // todo signal
    }



private:

    //    wss://stream.binance.com:9443/ws/btcusdt@trade
    void _start_coin_trade_connection(const QString& coin_select){
        QString _url = "wss://stream.binance.com:9443/ws/" + coin_select + "@trade";
        QUrl url(_url);
        qDebug() << "Opening trade connection to" << _url;
        trade_web_socket.open(url);
    }

    void _start_user_stream_connection( ){
        QString _url = "wss://stream.binance.com:9443/ws/" + listen_key ;
        QUrl url(_url);
        qDebug() << "Opening user stream connection to" << _url;
        user_stream_web_socket.open(url);
    }


    void _start_depth_connection(const QString& coin_select){
        QString _url = "wss://stream.binance.com:9443/ws/" + coin_select + "@depth";
        QUrl url(_url);
        qDebug() << "Opening depth connection to" << _url;
        depth_web_socket.open(url);
    }


    QString listen_key;
    QWebSocket user_stream_web_socket;
    bool user_stream_web_socket_connection_state ;

    coin_select_e selected_coin;  // for depth_socket + trade_socket
    QWebSocket trade_web_socket;
    bool trade_socket_connection_state ;

    QWebSocket depth_web_socket;
    bool depth_socket_connection_state ;

    QNetworkAccessManager manager;

};

#endif

