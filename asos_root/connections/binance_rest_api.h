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

/**********

Notes:

Responce:
[
    .. json arrays ..
    {
        .. json ..
    },
    {
        .. json ..
    }
    // ....
]


Responce:
{
    .. json ..
}

******************************************

"timeInForce" : "GTC" //  Sipariş, kullanıcı tarafından iptal edilene kadar geçerli kalır. Sipariş, işlenene kadar aktif olur ve otomatik olarak iptal edilmez.
"timeInForce" : "IOC" // Sipariş hemen işleme alınmalıdır. Eğer siparişin tamamı hemen gerçekleştirilemezse, mevcut kısmı işleme alınır ve geri kalan kısmı iptal edilir. Yani, siparişin tamamlanmasını beklemeden, mümkün olan kısmı işleme alınır. Kullanım: Hızlı işlem yapmak ve hemen piyasa fiyatında bir miktar alım veya satım gerçekleştirmek istiyorsanız bu seçeneği kullanabilirsiniz.

***********/

#ifndef BINANCE_REST_API_H
#define BINANCE_REST_API_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QMessageAuthenticationCode>

#include "../generals/ohlc.h"
#include "../account/coin_list.h"
#include "rest_api_structs.h"


// USER Config, todo add from GUI
#define USER_API_KEY      "5AQcSAAAAIVXVEBjWIWeWqs6fAaeXXXXHWgiLZB5t5MLvznTTTTTeRYBcX4dZ2ViUW"  // SET YOUR BINANCE API KEY
#define USER_API_SECRET   "bdmRQMAAAAndXhTmlfO9q2wABJS8XXXXVOJbeDFUUEWMRSHTTTTTTXm7EmU66UWQzM"  // SET YOUR BINANCE SECRET KEY


class binance_rest_api_c : public QWidget   {
    Q_OBJECT
public:
    binance_rest_api_c(QWidget* parent = nullptr,
                       coin_select_e _selected_coin = coin_select_e::BTC)
        : QWidget(parent),
        api_key(USER_API_KEY),
        api_secret(USER_API_SECRET)
    {
        finded_listen_key = "";
        selected_coin = _selected_coin;
        is_account_connection_success = false;
        network_manager = new QNetworkAccessManager(this);

        connect(network_manager, &QNetworkAccessManager::finished,
                this, &binance_rest_api_c::xon_network_reply);

        // @first job is get the userdata stream
        get_user_data_stream( );  // add button for this? todo
    }

    void change_api_key(const QString& dt )
    {
        api_key = dt;
    }

    void change_api_secret(const QString& dt )
    {
        api_secret = dt;
    }

    QString get_finded_listen_key( )
    {
        return finded_listen_key;
    }

    void select_coin(coin_select_e s)
    {
        selected_coin = s;
    }

    void get_account(qint64 recv_window = 5000) {
        _get_account(recv_window);
    }

    bool is_get_account_success(){
        return is_account_connection_success;
    }


    void get_my_trades(coin_select_e coin ,
                       int limit,
                       long fromId,
                       qint64 recv_window = 5000)
    {
        _get_my_trades(coin ,limit,fromId, recv_window);
    }

    void get_open_orders(coin_select_e coin ,
                         qint64 recv_window = 5000)
    {
        _get_open_orders(coin , recv_window);
    }

    void send_order(coin_select_e coin ,
                    side_select_e side,
                    type_select_e type,
                    double quantity,
                    double price,
                    double stopPrice = 0,
                    const QString& order_id = "",
                    qint64 recv_window = 5000)
    {
        _post_order(coin,side,type,quantity,price,stopPrice,order_id,recv_window);
    }

    void get_order(coin_select_e coin ,
                   const QString& order_id,
                   qint64 recv_window = 5000)
    {
        _get_order(coin,order_id,recv_window);
    }

    void close_order(coin_select_e coin ,
                     const QString& order_id,
                     qint64 recv_window = 5000)
    {
        _delete_order(coin,order_id,recv_window);
    }


    void get_klines(coin_select_e coin ,
                    kline_select_e select_kline,
                    qint64 start_time,  // you can use -> convert_string_to_timestamp
                    qint64 end_time,    // you can use -> convert_string_to_timestamp
                    int limit = 0 /*** limitmax 1000 ***/  ){
        _get_klines(coin,select_kline,start_time,end_time,limit);
    }

    void get_user_data_stream(){
        _post_user_data_stream(  );
    }

    /** you must call every 50 minute , it use current listenkey **/
    void allive_user_data_stream(){
        _put_user_data_stream();
    }

    /**
    ex: convert_string_to_timestamp("2021-02-20T18:10:52") = 1613833852000
    ***/
    qint64 convert_string_to_timestamp(const QString& isoDateString) {
        QDateTime dateTime = QDateTime::fromString(isoDateString, Qt::ISODate);
        return dateTime.toMSecsSinceEpoch();
    }


    /**
    ex: convert_timestamp_to_string(1613833852141) = "2021-02-20T18:10:52"
    ***/
    QString convert_timestamp_to_string(qint64 timestamp) {
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
        return dateTime.toString(Qt::ISODate);
    }



signals:

    void log_print(const QString &message);

    void time_received(qint64 server_time);

    void user_data_stream_finded(const QString& listen_key);

    void kline_coin_price_update(const OHLC_s& data , qint64 timestamp, double price_quantity_volume);

private:


    /*
     _parse_get_account ex:
        {
          "makerCommission": 15,
          "takerCommission": 15,
          "buyerCommission": 0,
          "sellerCommission": 0,
          "commissionRates": {
            "maker": "0.00150000",
            "taker": "0.00150000",
            "buyer": "0.00000000",
            "seller": "0.00000000"
          },
          "canTrade": true,
          "canWithdraw": true,
          "canDeposit": true,
          "brokered": false,
          "requireSelfTradePrevention": false,
          "preventSor": false,
          "updateTime": 123456789,
          "accountType": "SPOT",
          "balances": [
            {
              "asset": "BTC",
              "free": "4723846.89208129",
              "locked": "0.00000000"
            },
            {
              "asset": "LTC",
              "free": "4763368.68006011",
              "locked": "0.00000000"
            }
          ],
          "permissions": [
            "SPOT"
          ],
          "uid": 354937868
        }
    **/
    void _parse_get_account(const QJsonObject & json_object){



        QJsonArray balances  = json_object["balances"].toArray();

        double balance = 0.0;

        for (const QJsonValue& value : balances) {
            QJsonObject balance_object = value.toObject();
            if (balance_object["asset"].toString() == coin_map_get_enum_to_str(selected_coin)) {
                balance = balance_object["free"].toString().toDouble();
                break;
            }
        }

        emit log_print("R_API: wallet: " + coin_map_get_enum_to_str(selected_coin) + " : " + QString::number(balance));
    }


    /***
     _parse_my_trades ex:
          {
            "symbol": "BNBBTC",
            "id": 28457,
            "orderId": 100234,
            "orderListId": -1,
            "price": "4.00000100",
            "qty": "12.00000000",
            "quoteQty": "48.000012",
            "commission": "10.10000000",
            "commissionAsset": "BNB",
            "time": 1499865549590,
            "isBuyer": true,
            "isMaker": false,
            "isBestMatch": true
          }
     */
    void _parse_my_trades(const QJsonObject & json_object){

        QString symbol = json_object["symbol"].toString();
        QString price = json_object["price"].toString();
        qint64 time = json_object["time"].toVariant().toLongLong();
        /*
        int id = json_object["id"].toInt();
        int order_id = json_object["orderId"].toInt();
        int order_list_id = json_object["orderListId"].toInt();
        QString qty = json_object["qty"].toString();
        QString quote_qty = json_object["quoteQty"].toString();
        QString commission = json_object["commission"].toString();
        QString commission_asset = json_object["commissionAsset"].toString();
        bool is_buyer = json_object["isBuyer"].toBool();
        bool is_maker = json_object["isMaker"].toBool();
        bool is_best_match = json_object["isBestMatch"].toBool();
        */
        emit log_print("R_API my trades: " + symbol + " , time: " + convert_timestamp_to_string(time) + " , price: " + price);
    }

    /**
    _parse_open_orders ex:
        {
            "symbol": "BTCUSDT",
            "origClientOrderId": "E6APeyTJvkMvLMYMqu1KQ4",
            "orderId": 11,
            "orderListId": -1,
            "clientOrderId": "pXLV6Hz6mprAcVYpVMTGgx",
            "transactTime": 1684804350068,
            "price": "0.089853",
            "origQty": "0.178622",
            "executedQty": "0.000000",
            "cummulativeQuoteQty": "0.000000",
            "status": "CANCELED",
            "timeInForce": "GTC",
            "type": "LIMIT",
            "side": "BUY",
            "selfTradePreventionMode": "NONE"
        }
    **/
    void _parse_open_orders(const QJsonObject & json_object){

        QString symbol = json_object["symbol"].toString();
        QString price = json_object["price"].toString();
        qint64 time = json_object["transactTime"].toVariant().toLongLong();

        emit log_print("R_API open orders: " + symbol + " , time: " + convert_timestamp_to_string(time) + " , price: " + price);
    }


    /**
    {"listenKey":"GAy7DB7YGRmfA9bXNZJZkmxK1kWOw2zXsefxxKsnXIO95gboecJNA8VsI56O"}
    **/
    void _parse_listen_key(const QJsonObject & json_object){

        if (json_object.contains("listenKey")) {
            QJsonValue listenKeyValue = json_object.value("listenKey");

            if (listenKeyValue.isString()) {

                finded_listen_key = listenKeyValue.toString();

                emit user_data_stream_finded(finded_listen_key);
                emit log_print("R_API found listen key: " + finded_listen_key);

            }
            else {

                qWarning() << "'listenKey' value is not a string!";
            }
        } else {

            qWarning() << "'listenKey' not found in JSON object!";
        }
    }



    /**
        [
                1499040000000,      // Kline open time
                "0.01634790",       // Open price
                "0.80000000",       // High price
                "0.01575800",       // Low price
                "0.01577100",       // Close price
                "148976.11427815",  // Volume
                1499644799999,      // Kline Close time
                "2434.19055334",    // Quote asset volume
                308,                // Number of trades
                "1756.87402397",    // Taker buy base asset volume
                "28.46694368",      // Taker buy quote asset volume
                "0"                 // Unused field, ignore.
        ],
    **/
    void _kline_json_parser(const QJsonArray & kline_array ){

        OHLC_s data;
        // test print
        //emit log_print("kline receive: ");
        //QJsonDocument doc(kline_array);
        //emit log_print(doc.toJson(QJsonDocument::Compact));

        QString highPriceStr = kline_array[2].toString();
        QString lowPriceStr = kline_array[3].toString();
        QString openPriceStr = kline_array[1].toString();
        QString closePriceStr = kline_array[4].toString();
        qint64 klineOpenTime = kline_array[0].toVariant().toLongLong();
        QString volume = kline_array[5].toString();
        /*qint64 klineCloseTime = kline_array[6].toVariant().toLongLong();
        QString quoteAssetVolume = kline_array[7].toString();
        int numberOfTrades = kline_array[8].toInt();
        QString takerBuyBaseAssetVolume = kline_array[9].toString();
        QString takerBuyQuoteAssetVolume = kline_array[10].toString();
        QString unusedField = kline_array[11].toString(); // Unused field, ignore
        */

        bool ok;
        data.high = highPriceStr.toDouble(&ok);
        if (!ok) { data.high  = 0.0; }
        data.low = lowPriceStr.toDouble(&ok);
        if (!ok) { data.low = 0.0; }

        data.open = openPriceStr.toDouble(&ok);
        if (!ok) { data.open = 0.0; }
        data.close = closePriceStr.toDouble(&ok);
        if (!ok) { data.close = 0.0; }

        double price_quantity_volume = 0;
        price_quantity_volume = volume.toDouble(&ok);
        if (!ok) { price_quantity_volume = 0.0; }

        emit kline_coin_price_update(data , klineOpenTime , price_quantity_volume);
    }

    void _json_parser(const QJsonObject & json_object ){

        if (json_object.contains("balances") &&
            json_object["balances"].isArray()) {

            _parse_get_account(json_object);

            is_account_connection_success = true;
        }
        else if (json_object.contains("commission") &&
                 json_object.contains("isMaker")  &&
                 json_object.contains("isBuyer")  ) {

            _parse_my_trades(json_object);

        }
        else if (json_object.contains("origClientOrderId") &&
                 json_object.contains("clientOrderId")  &&
                 json_object.contains("transactTime")  ) {

            _parse_open_orders(json_object);

        }
        else if (json_object.contains("listenKey")   ) {
            _parse_listen_key(json_object);
        }
        else{

            emit log_print("R_API: unknown JSON ");

            QJsonDocument doc(json_object);

            emit log_print(doc.toJson(QJsonDocument::Compact));
        }
    }




private slots:


    void xon_network_reply(QNetworkReply* reply){

        if (reply->url().path() == "/api/v3/time") {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray response_data = reply->readAll();
                QJsonDocument json_doc = QJsonDocument::fromJson(response_data);
                QJsonObject json_object = json_doc.object();
                qint64 server_time = json_object.value("serverTime").toVariant().toLongLong();
                emit time_received(server_time);
            } else {
                emit log_print("R_API TIME REQ:  Error fetching server time:" + reply->errorString());
            }
        }
        else
        {
            if (reply->error() == QNetworkReply::NoError) {

                QByteArray response_data = reply->readAll();

                QJsonDocument json_doc = QJsonDocument::fromJson(response_data);

                if (json_doc.isObject()) {

                    QJsonObject json_object = json_doc.object();

                    _json_parser(json_object);   // @super Receive JSON Parser
                }
                else  if (json_doc.isArray()) {

                    QJsonArray jsonArray = json_doc.array();

                    for (const QJsonValue &value : jsonArray) {

                        if (value.isObject()) {

                            QJsonObject _json_object = value.toObject();

                            _json_parser(_json_object);   // @super Receive JSON Parser
                        }
                        else if (value.isArray()) {
                            QJsonArray inner_array = value.toArray();

                            // TODO emin ol bir tek klinelar mı buraya düşer?

                            _kline_json_parser(inner_array);   // @super KLINE Receive JSON Parser
                        }
                    }
                }
                else{
                    emit log_print("R_API: unknown responce" );
                }
            }
            else
            {
                emit log_print("R_API: Error fetching balance: "  + reply->errorString());
                QByteArray response = reply->readAll();
                emit log_print("Error Response: "  + QString(response));
            }

        }
        reply->deleteLater();
    }




private:

    void _get_account(qint64 recv_window = 5000) {

        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }
#ifdef FUTURES_MODE_ON
        QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
        QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, recv_window](qint64 server_time)
                {
#ifdef FUTURES_MODE_ON
                    QUrl url("https://fapi.binance.com/fapi/v3/account");
#else
                    QUrl url("https://api.binance.com/api/v3/account");
#endif

                    qint64 timestamp = server_time;
                    QUrlQuery query;
                    query.addQueryItem("timestamp", QString::number(timestamp));

                    if (recv_window > 0) {
                        query.addQueryItem("recvWindow", QString::number(recv_window));
                    }

                    QString query_string = query.toString(QUrl::FullyEncoded);
                    QString signature = create_signature(query_string);
                    query.addQueryItem("signature", signature);
                    url.setQuery(query);

                    QNetworkRequest request(url);
                    request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

                    network_manager->get(request);

                });

        network_manager->get(time_request);

    }




    void _get_my_trades(coin_select_e coin ,
                        int limit,
                        long fromId,
                        qint64 recv_window = 5000)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

#ifdef FUTURES_MODE_ON
         QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
         QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, coin,limit,fromId , recv_window](qint64 server_time) {

#ifdef FUTURES_MODE_ON
            QUrl url("https://fapi.binance.com/fapi/v3/myTrades");
#else
          QUrl url("https://api.binance.com/api/v3/myTrades");
#endif

            qint64 timestamp = server_time;
            QUrlQuery query;

            query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

            if (limit > 0) {
                query.addQueryItem("limit", QString::number(limit));
            }

            if (fromId > 0) {
                query.addQueryItem("fromId", QString::number(fromId));
            }

            if (recv_window > 0) {
                query.addQueryItem("recvWindow", QString::number(recv_window));
            }

            query.addQueryItem("timestamp", QString::number(timestamp));

            QString query_string = query.toString(QUrl::FullyEncoded);
            QString signature = create_signature(query_string);
            query.addQueryItem("signature", signature);
            url.setQuery(query);

            QNetworkRequest request(url);
            request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

            network_manager->get(request);
        });

        network_manager->get(time_request);

    }




    void _get_open_orders(coin_select_e coin ,
                          qint64 recv_window = 5000)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

#ifdef FUTURES_MODE_ON
        QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
        QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, coin,  recv_window](qint64 server_time) {

#ifdef FUTURES_MODE_ON
            QUrl url("https://fapi.binance.com/fapi/v3/openOrders");
#else
            QUrl url("https://api.binance.com/api/v3/openOrders");
#endif

            qint64 timestamp = server_time;
            QUrlQuery query;

            query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

            if (recv_window > 0) {
                query.addQueryItem("recvWindow", QString::number(recv_window));
            }

            query.addQueryItem("timestamp", QString::number(timestamp));

            QString query_string = query.toString(QUrl::FullyEncoded);
            QString signature = create_signature(query_string);
            query.addQueryItem("signature", signature);
            url.setQuery(query);

            QNetworkRequest request(url);
            request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

            network_manager->get(request);
        });

        network_manager->get(time_request);

    }






    void _get_klines(coin_select_e coin ,
                     kline_select_e select_kline,
                     qint64 start_time,  // you can use -> convert_string_to_timestamp
                     qint64 end_time,    // you can use -> convert_string_to_timestamp
                     int limit = 500 /*** limitmax 1000 ***/)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

#ifdef FUTURES_MODE_ON
        QUrl url("https://fapi.binance.com/fapi/v3/klines");
#else
        QUrl url("https://api.binance.com/api/v3/klines");
#endif


        QUrlQuery query;

        query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

        query.addQueryItem("interval", kline_select_to_string(select_kline));


        if ( start_time > 0 && end_time > 0 ) {
            query.addQueryItem("startTime", QString::number(start_time) );
            query.addQueryItem("endTime", QString::number(end_time) );

        } else if ( limit > 0 ) {
            int _limit = limit;
            if (limit > 1000)
                _limit = 1000;
            query.addQueryItem("limit", QString::number(_limit) );
        }

        url.setQuery(query);

        QNetworkRequest request(url);
        request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

        network_manager->get(request);
    }


    /**
        "timeInForce" : "GTC"
        or
        "timeInForce" : "IOC"

        const char* newClientOrderId -> you can set your order id number for trace
    **/
    void _post_order(coin_select_e coin ,
                     side_select_e side,
                     type_select_e type,
                     double quantity,
                     double price,
                     double stopPrice,
                     const QString& order_id,
                     qint64 recv_window = 5000)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

#ifdef FUTURES_MODE_ON
        QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
        QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, coin, side, type, quantity ,price, stopPrice,order_id, recv_window](qint64 server_time) {

#ifdef FUTURES_MODE_ON
            QUrl url("https://fapi.binance.com/fapi/v3/order");
#else
            QUrl url("https://api.binance.com/api/v3/order");
#endif

            qint64 timestamp = server_time;
            QUrlQuery query{0};

            query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

            query.addQueryItem("side", side_select_to_string(side) );

            query.addQueryItem("type", type_select_to_string(type) );

            query.addQueryItem("timeInForce", "GTC");  // "GTC" or  "IOC" -> FASTER  ?

            query.addQueryItem("quantity",  QString::number(quantity));

            query.addQueryItem("price",  QString::number(price));

            query.addQueryItem("timestamp", QString::number(timestamp));

            if(!order_id.isEmpty()){
                // A unique id among open orders. Automatically generated if not sent.
                // Orders with the same newClientOrderID can be accepted only when the
                // previous one is filled, otherwise the order will be rejected.
                query.addQueryItem("newClientOrderId",  order_id);
            }

            if(stopPrice > 0){
                query.addQueryItem("stopPrice",  QString::number(stopPrice));
            }

            if (recv_window > 0) {
                query.addQueryItem("recvWindow", QString::number(recv_window));
            }

            QString query_string = query.toString(QUrl::FullyEncoded);
            QString signature = create_signature(query_string);
            query.addQueryItem("signature", signature);

            //url.setQuery(query);  // ATTANTION -> For POST you can not add query to URL ATTANTION!!!!

            QNetworkRequest request(url);
            request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

            QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();

            network_manager->post(request, postData);
        });

        network_manager->get(time_request);
    }



    void _get_order(coin_select_e coin ,
                    const QString& order_id,
                    qint64 recv_window = 5000)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

#ifdef FUTURES_MODE_ON
        QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
        QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, coin, order_id, recv_window](qint64 server_time) {

#ifdef FUTURES_MODE_ON
            QUrl url("https://fapi.binance.com/fapi/v3/order");
#else
            QUrl url("https://api.binance.com/api/v3/order");
#endif

            qint64 timestamp = server_time;
            QUrlQuery query;

            query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

            if(!order_id.isEmpty()){
                query.addQueryItem("origClientOrderId",  order_id);
            }

            if (recv_window > 0) {
                query.addQueryItem("recvWindow", QString::number(recv_window));
            }

            query.addQueryItem("timestamp", QString::number(timestamp));

            QString query_string = query.toString(QUrl::FullyEncoded);
            QString signature = create_signature(query_string);
            query.addQueryItem("signature", signature);
            url.setQuery(query);

            QNetworkRequest request(url);
            request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

            network_manager->get(request);
        });

        network_manager->get(time_request);

    }




    void _delete_order(coin_select_e coin ,
                       const QString& order_id,
                       qint64 recv_window = 5000)
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }


#ifdef FUTURES_MODE_ON
        QUrl time_url("https://fapi.binance.com/fapi/v3/time");
#else
        QUrl time_url("https://api.binance.com/api/v3/time");
#endif

        QNetworkRequest time_request(time_url);

        disconnect(this, &binance_rest_api_c::time_received, this, nullptr);

        connect(this, &binance_rest_api_c::time_received, [this, coin, order_id, recv_window](qint64 server_time) {

#ifdef FUTURES_MODE_ON
            QUrl url("https://fapi.binance.com/fapi/v3/order");
#else
            QUrl url("https://api.binance.com/api/v3/order");
#endif

            qint64 timestamp = server_time;
            QUrlQuery query;

            query.addQueryItem("symbol", coin_api_map_get_enum_to_str(coin) );

            if(!order_id.isEmpty()){
                query.addQueryItem("origClientOrderId",  order_id);
            }

            if (recv_window > 0) {
                query.addQueryItem("recvWindow", QString::number(recv_window));
            }

            query.addQueryItem("timestamp", QString::number(timestamp));

            QString query_string = query.toString(QUrl::FullyEncoded);
            QString signature = create_signature(query_string);
            query.addQueryItem("signature", signature);
            url.setQuery(query);

            QNetworkRequest request(url);
            request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

            network_manager->deleteResource(request);
        });

        network_manager->get(time_request);
    }

    // TODO close_user_data_stream() , EKLE

    /** you must call every 30 min. **/
    void _put_user_data_stream(  )  // todo , test edilmeli, yeni listen key üretmez var olanın süresini uzatır, responce olarak boş döner yada hata üretir
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }

        if(finded_listen_key.isEmpty()){
            emit log_print("R_API _put_user_data_stream: finded_listen_key EMPTY");
            return;
        }

#ifdef FUTURES_MODE_ON
        QUrl url("https://fapi.binance.com/fapi/v3/userDataStream");
#else
        QUrl url("https://api.binance.com/api/v3/userDataStream");
#endif

        QUrlQuery query{0};

        query.addQueryItem("listenKey",  finded_listen_key);

        url.setQuery(query.query());  // ?

        QNetworkRequest request(url);
        request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

        network_manager->put(request, QByteArray());
    }


    void _post_user_data_stream(  )
    {
        if (api_key.isEmpty() || api_secret.isEmpty()) {
            emit log_print("R_API:  API Key and Secret Key have not been set.");
            return;
        }


#ifdef FUTURES_MODE_ON
        QUrl url("https://fapi.binance.com/fapi/v3/userDataStream");
#else
        QUrl url("https://api.binance.com/api/v3/userDataStream");
#endif

        QUrlQuery query{0};

        QNetworkRequest request(url);
        request.setRawHeader("X-MBX-APIKEY", api_key.toUtf8());

        QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();

        network_manager->post(request, postData);
    }



    QString create_signature(const QString& params) {
        QByteArray secret = api_secret.toUtf8();
        QByteArray hash = QMessageAuthenticationCode::hash(params.toUtf8(), secret, QCryptographicHash::Sha256);
        return QString(hash.toHex());
    }


private:
    QNetworkAccessManager* network_manager;
    QString finded_listen_key;
    QString api_key;
    QString api_secret;
    coin_select_e selected_coin;
    bool is_account_connection_success ;
};


#endif
