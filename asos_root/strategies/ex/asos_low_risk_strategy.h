#ifndef ASOS_LOW_RISK_STRATEGY_H
#define ASOS_LOW_RISK_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 * Strateji açıklaması:
 * Bu strateji, hareketli ortalama (MA) ve Göreceli Güç Endeksi (RSI) kullanarak alım ve satım sinyalleri üretir.
 *
 * - Eğer RSI 30'un altına düşerse ve son fiyat hareketli ortalamanın altındaysa alım yapar.
 * - Eğer RSI 70'in üzerine çıkarsa ve son fiyat hareketli ortalamanın üzerindeyse satım yapar.
 * - Diğer durumlarda işlem yapmaz (PROCESSING).
 */

class asos_low_risk_strategy_c : public trade_strategy_interface{
public:
    asos_low_risk_strategy_c(std::size_t _period = 5, account_interface * _account = nullptr) :
        period(_period), account(_account)
    {
        data_container = new OHLC_container(period);
        is_buy_order_set_before = false;
    }

    trade_strategy_order_e execute() override{

        qsizetype c_size = data_container->container.size();

        if ( c_size < period ) {
            return trade_strategy_order_e::PROCESSING;
        }

        double moving_avarage = data_container->last_moving_avarage;

        double rsi = data_container->last_rsi;

        double last_avr_price = last_price.get_average_all_price();

        if(IS_SELLED() &&
            rsi > 30 &&
            rsi < 40 &&
            last_avr_price > moving_avarage)
        {
            return BUY();
        }
        else if( IS_BUYED() &&
                 rsi > 60 &&
                 last_avr_price < moving_avarage )
        {
            return SELL();
        }

        return trade_strategy_order_e::PROCESSING;
    }

    void feed_ohlc(const OHLC_s& ohlc_seed) override{

        data_container->add(ohlc_seed);

        last_price = ohlc_seed;
    }

    void feed_depth(double bids_ratio) override{

        // todo add depth to list in a period and look in 1 minute chnages radio ?
    }

    void feed_price_quantity_volume(double quantity) override{

        // todo hacmi al
    }

private:

    trade_strategy_order_e BUY(){
        is_buy_order_set_before = true;
        if(account)
            account->buy_order();
        return trade_strategy_order_e::BUY;
    }

    trade_strategy_order_e SELL(){
        is_buy_order_set_before = false;
        if(account)
            account->sell_order();
        return trade_strategy_order_e::SELL;
    }


    bool IS_BUYED(){
        return is_buy_order_set_before;
    }

    bool IS_SELLED(){
        return !is_buy_order_set_before;
    }


    // ALGO SET configs


    // ALGO params


private:  // must be static
    std::size_t period;
    OHLC_s last_price;
    OHLC_container  *data_container;
    account_interface * account;
    bool is_buy_order_set_before;
};






#endif
