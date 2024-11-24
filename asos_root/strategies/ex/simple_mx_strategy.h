#ifndef SIMPLE_MX_STRATEGY_H
#define SIMPLE_MX_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

   - MA_25 yeşil kesip yukarda kapanırsa    ->   mx_2 -> period: 25 ? changable

     if (above_the_MA_line_counter_flag)
        above_the_MA_line_counter_flag++;

     if (lastPrice_.get_average_all_price() > movingAverage_)
        above_the_MA_line_counter_flag = 1

   - 8 dk içinde ikinci bir mum yine MA_25 i aşağıdan yukarı yeşil keserse

     if(above_the_MA_line_counter_flag && above_the_MA_line_counter < 8)
     else if(above_the_MA_line_counter > 8)
        above_the_MA_line_counter = 0;
        above_the_MA_line_counter_flag = 0;

   - MA_25 altına geleni direk SAT veya %1 kar görürsen sat koşulu ekle !!

***/

class simple_mx_strategy_c : public trade_strategy_interface{
public:
    simple_mx_strategy_c(std::size_t _period = 5,
                         account_interface * _account = nullptr) :
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

        double last_avr_price = last_price.get_average_all_price();

        double rsi = data_container->last_rsi;

        if(moving_avarage>0){

            switch (state)
            {
            case 0:  // wait idle step
            {
                if (last_avr_price > moving_avarage &&
                    IS_SELLED())
                {
                    above_the_MA_line_counter = 0;
                    state = 1;
                }
                break;
            }

            case 1:  // process could be start wait for it to fall
            {
                above_the_MA_line_counter++;

                if (rsi > 30 &&
                    last_avr_price < moving_avarage &&
                    above_the_MA_line_counter > min_wait_THRESOLD_candle &&
                    above_the_MA_line_counter < max_wait_candle)
                {
                    state = 2;
                    break;
                }

                if(above_the_MA_line_counter >= max_wait_candle){
                    state = 0;
                    above_the_MA_line_counter = 0;
                    break;
                }

                break;
            }

            case 2:  // check second MA pass green candle
            {
                above_the_MA_line_counter++;

                if (last_avr_price > moving_avarage+(moving_avarage*min_avarage_factor) &&
                    above_the_MA_line_counter < max_wait_candle)
                {
                    state = 3;

                    save_purchased_price = last_price;

                    above_the_MA_line_counter = 0;

                    return BUY();
                }

                if(above_the_MA_line_counter >= max_wait_candle){
                    state = 0;
                    above_the_MA_line_counter = 0;
                }

                break;
            }

            case 3:  // wait SELL
            {
                above_the_MA_line_counter++;

                if(last_avr_price > moving_avarage)
                    above_the_MA_line_counter = 0;

                if (rsi > 60 &&
                    last_avr_price < moving_avarage - (moving_avarage*min_avarage_factor) &&
                    above_the_MA_line_counter > min_wait_THRESOLD_candle &&
                    IS_BUYED())
                {
                    state = 0;

                    return SELL();
                }

                // %1 kari gördün sat !! test et
                if (last_avr_price > save_purchased_price.get_average_all_price() + (save_purchased_price.get_average_all_price()*max_percent_profit) &&
                    IS_BUYED())
                {
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

    // todo bu strategilere debug print mi eklesek ?

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
    const int min_wait_THRESOLD_candle = 3;
    const int max_wait_candle = 12;
    const double max_percent_profit = 0.008;  // %1

    const double min_avarage_factor = 0.0006;

    // ALGO params
    OHLC_s save_purchased_price ; // satın alınan fiyat
    int above_the_MA_line_counter = 0;
    int state = 0;

private:  // must be static
    std::size_t period;
    OHLC_s last_price;
    OHLC_container  *data_container;
    account_interface * account;
    bool is_buy_order_set_before;
};





#endif // SIMPLE_MX_STRATEGY_H
