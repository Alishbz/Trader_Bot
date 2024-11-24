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
#ifndef TRADE_STRATEGY_INTERFACE_H
#define TRADE_STRATEGY_INTERFACE_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QPair>

#include "../generals/ohlc.h"
#include "../account/account_interface.h"


#define PERCENT_ADDER(value,percent)           (value + (value*percent))
#define PERCENT_SUBTRACTOR(value,percent)      (value - (value*percent))



struct strategy_params {

    QMap<QString, double> params;

    void add_param(const QString& name, double value) {
        params.insert(name, value);
    }

    double get_param_value(const QString& name) const {
        return params.value(name, 0.0);
    }

    void add_by_index(int index , double value)   {
        if (index >= 0 && index < params.size()) {
            QList<QString> keys = params.keys();
            QString key = keys.at(index);
            params.insert(key, value);
        }
    }

    QString get_param_name(int index) const {
        if (index >= 0 && index < params.size()) {
            QList<QString> x = params.keys();
            return x.at(index);
        } else {
            return QString();
        }
    }
};

enum class trade_strategy_order_e {
    PROCESSING,
    PROCESSING_MARK_1,  // Sky Blue, could be a peak detection signal
    PROCESSING_MARK_2,  // Dark Pit, could be a trough detection signal
    BUY,
    SELL,
    SHORT
};




enum class trade_logger_signals{
    STARTED,
    BUYED,
    SHORT,
    SELLED,
    STOPED,
    LAST_PRICE
};

// trade_logger: amaç satın alınan değer satılan değer vs gibi bilgileri kaydetmek, kar zarar hesaplamak
class trade_logger: public QObject {
    Q_OBJECT
public:
    trade_logger(QObject* parent = nullptr )  : QObject(parent) {
        commission = FAKE_COMMISSION_RATE;
    }

    void print(const QString & str){
        emit direct_print(str);
    }

    void set_signal(trade_logger_signals signal ,
                    double price = 0,
                    account_interface* _account = nullptr)
    {
        double wallet_budget = 1000;
        if(_account){
            wallet_budget = _account->get_wallet();
        }

        switch (signal) {

        case trade_logger_signals::BUYED:
        {
            short_signal_active = false;

            trades.append(qMakePair(price, 0.0));

            break;
        }

        case trade_logger_signals::SHORT:
        {
            short_signal_active = true;

            trades.append(qMakePair(price, 0.0));  // TODO calculate short

            break;
        }

        case trade_logger_signals::SELLED:
            if (!trades.isEmpty()) {
                for (auto &trade : trades) {
                    if (trade.second == 0.0) {  // Find the first unsold trade
                        if(short_signal_active == false){
                            // LONG BUY
                            trade.second = price;
                            double profit = (price - trade.first - commission*2) / trade.first * 100;
                            print(QString("LOG: Sold at %1, bought at %2, percent profit: %3").arg(price).arg(trade.first).arg(profit));
                            fake_wallet += profit;

                        }
                        else{
                            // SHORT
                            trade.second = price;
                            double profit = (trade.first - price - commission * 2) / trade.first * 100;
                            print(QString("LOG: Covered SHORT at %1, sold at %2, percent profit: %3")
                                      .arg(price).arg(trade.first).arg(profit));
                            fake_wallet += profit;
                        }
                        break;
                    }
                }
            }else {
                print("LOG: No trades to sell");
            }
            break;
        case trade_logger_signals::STARTED:
            print("LOG: Trade LOG starting");
            started_price = price;
            started_wallet = wallet_budget;
            break;
        case trade_logger_signals::LAST_PRICE:
            last_price = price;
            break;
        case trade_logger_signals::STOPED:
            double total_profit = 0.0;
            double total_commission = 0.0;
            int trade_count = 0;
            for (const auto &trade : trades) {
                if (trade.second != 0.0) {
                    total_profit += (price - trade.first - commission*2);
                    total_commission += commission * 2;
                    trade_count++;
                }
            }
           // double final_wallet = started_wallet + total_profit; // todo bug
            double potential_wallet = started_wallet * (last_price / started_price);

            //todo kaç karlı kaç zararda process oldu ?

            QString report = QString("LOG: Trading stopped\nTotal percent profit: %1\nTotal commission paid: %2\n")
                                 .arg( (wallet_budget - started_wallet)  / started_wallet * 100)
                                 .arg(total_commission);
            report += QString("LOG: trade count:  %1\n").arg(trade_count);
            report += QString("LOG: If no trades were made, wallet: %1\n").arg(potential_wallet);
           // report += QString("LOG: Final wallet:                   %1\n").arg(final_wallet);

            print(report);
            trades.clear();
            break;
        }
    }

private:
    bool short_signal_active = false;
    double commission = 0;
    double started_price = 0;
    double started_wallet = 1000;
    double fake_wallet = 1000;
    double last_price = 0;
    QVector<QPair<double, double>> trades;  // Pairs of <buy_price, sell_price>

signals:
    void direct_print(const QString &message);
};




class trade_strategy_interface {
public:
    virtual ~trade_strategy_interface() = default;

    virtual trade_strategy_order_e execute() = 0;

    virtual void feed_ohlc(const OHLC_s& ohlc_seed) = 0;

    virtual void feed_depth(double bids_ratio) = 0;

    virtual void feed_price_quantity_volume(double quantity) = 0;

    /** logger ***/

    virtual void log_print(const QString & str) {
        if(logger)
            logger->print(str);
    }

    virtual void logger_signal(trade_logger_signals signal , double value = 0, account_interface* _account = nullptr) {
        if(logger)
            logger->set_signal(signal,value,_account );
    }

    void set_logger( trade_logger* _logger){
        logger = _logger;
    }

private:
    trade_logger* logger = nullptr;
};





/*****************/


/** trader_c: USER can chose strategy independently **/
class trader_c : public QObject{
    Q_OBJECT
public:
    trader_c( QObject* parent = nullptr ) :
        QObject(parent) , strategy_(nullptr)
    {
        logger = new trade_logger(this);
        QObject::connect(logger, &trade_logger::direct_print,this, &trader_c::log_print);
    }

    void set_strategy(trade_strategy_interface* strategy) {
        strategy_ = strategy;
        if(strategy_){
            strategy_->set_logger(logger);
        }
        else if(strategy_ == nullptr){
            logger->set_signal(trade_logger_signals::STOPED,0);
        }
    }

    trade_strategy_order_e execute_strategy() {
        if (strategy_) {
            return strategy_->execute();
        }
        return trade_strategy_order_e::PROCESSING;
    }

    void async_data_update(const OHLC_s& ohlc_seed)
    {
        if (strategy_) {
            strategy_->feed_ohlc(ohlc_seed);
        }
    }

    void async_depth_update(double bids_ratio)
    {
        if (strategy_) {
            strategy_->feed_depth(bids_ratio);
        }
    }

    void async_price_quantity_volume_update(double quantity)
    {
        if (strategy_) {
            strategy_->feed_price_quantity_volume(quantity);
        }
    }

signals:
    void log_print(const QString &message);

private:
    trade_strategy_interface* strategy_;
    trade_logger *             logger;
};



#endif // TRADE_STRATEGY_INTERFACE_H
