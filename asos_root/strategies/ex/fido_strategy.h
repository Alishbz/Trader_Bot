#ifndef FIDO_STRATEGY_H
#define FIDO_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
  Strateji açıklaması:

  short MA , long MA yi aşağıdan yukarı keserse AL emri, tam tersi SAT
  İkinci stepde rsi göre koru , çünkü bu algoritma durağan düz piyasalarda kötü çalışıyor

   Özellikler:
 - piyasa yönü aşağı ise korur
 - piyasa yönü yukarı ise KAR alır
 - durağan düz ise komisyondan zarara sokar !!

***/


#define FIDO_RISK_MAX_VECTOR_PRICE_SIZE 1024



class fido_strategy_c : public trade_strategy_interface{
public:
    fido_strategy_c(std::size_t _short_period = 7, std::size_t _long_period = 25, account_interface * _account = nullptr) :
        short_period(_short_period),long_period(_long_period), account(_account)
    {
        data_short_container = new OHLC_container(short_period);
        data_long_container = new OHLC_container(long_period);
        is_buy_order_set_before = false;
    }

    // this execute() function is triging every second and OHLC's are 1 minute
    trade_strategy_order_e execute() override{

        qsizetype c_size = data_long_container->container.size();

        if ( c_size < long_period ) {
            return trade_strategy_order_e::PROCESSING;
        }

        double long_moving_avarage = data_long_container->last_moving_avarage;
        double short_moving_avarage = data_short_container->last_moving_avarage;
        double long_rsi  = data_long_container->last_rsi;
        double short_rsi = data_short_container->last_rsi;
        double last_avr_price = last_price.get_average_all_price();

        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:
            {
                if (IS_SELLED() &&
                    short_moving_avarage > long_moving_avarage)
                {
                    state_ohlc_counter = 0;
                    state = 1; // rsi check signal
                }
                break;
            }

            case 1:
            {
                state_ohlc_counter++;

                if(IS_SELLED() && state_ohlc_counter < 4 && short_rsi < 70) // todo 45 ? kalibre et ?
                {
                    state = 10;
                    return BUY();
                }
                else if(state_ohlc_counter>3){
                    state = 2;
                }

                break;
            }

            case 2:  // fail wait reverse cross , then START again to process
            {
                if( short_moving_avarage < long_moving_avarage ){
                    state_ohlc_counter = 0;
                    state = 0;
                }
                break;
            }

            case 10:
            {
                state_ohlc_counter++;

                if(state_ohlc_counter > 3){

                    if (IS_BUYED() &&
                        short_moving_avarage < long_moving_avarage &&
                        short_rsi > 30)
                    {
                        state = 0;
                        return SELL();
                    }
                }
                else if (last_avr_price > save_purchased_price.get_average_all_price() + (save_purchased_price.get_average_all_price()*max_percent_profit) &&
                           IS_BUYED())
                {
                    // kari gördüysende çık
                    state = 0;

                    return SELL();
                }

                break;
            }

            default:
                break;
            }
        }

        return trade_strategy_order_e::PROCESSING;
    }

    void feed_ohlc(const OHLC_s& ohlc_seed) override{

        data_short_container->add(ohlc_seed);

        data_long_container->add(ohlc_seed);

        last_price = ohlc_seed;

        prices.append(last_price.get_average_all_price());

        if(prices.size()>FIDO_RISK_MAX_VECTOR_PRICE_SIZE)
            prices.removeFirst();
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
        save_purchased_price = last_price;
        if(account)
            account->buy_order();
        return trade_strategy_order_e::BUY;
    }

    trade_strategy_order_e SELL(){
        is_buy_order_set_before = false;
        save_purchased_price.clear();
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





    QVector<double> prices;
    const double max_percent_profit = 0.01;
    const double min_avarage_factor = 0.0006;

    // ALGO params
    int state = 0;
    int state_ohlc_counter = 0;

private:  // must be static
    std::size_t short_period;
    std::size_t long_period;
    OHLC_s save_purchased_price ; // satın alınan fiyat
    OHLC_s last_price;
    OHLC_container  *data_short_container;
    OHLC_container  *data_long_container;
    account_interface * account;
    bool is_buy_order_set_before;
};





#endif
