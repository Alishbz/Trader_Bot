#ifndef SIMPLE_MX_PRO_STRATEGY_H
#define SIMPLE_MX_PRO_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:
mx pro algorithm: Diplerden alım tepelerden satım

BUY - ALIM için

- step_1:   current_price < MA_100  &&
            RSI_20 < 40             &&
            RSI_100 < 50            &&
            MA_20 < MA_100          :   Dip bulundu -> goto step_2


- step_2:   current_price > MA_20 &&
            RSI_100 > 50          &&
            MA_20 > MA_100        &&
            RSI_20 > 50           :     ->  BUY   goto step_3

            x_flag == true &&
            RSI_20  < 40      : -> goto step_1 , yeni dip aramaya git

            RSI_20 > 50       : ->  x_flag = true


- step_3:   current_price < MA_100 ||
            MA_20 < MA_100         ||
            RSI_100 > 60           ||
            RSI_20  > 80           :     ->  SELL   goto step_1


***/


class simple_mx_pro_strategy_c : public trade_strategy_interface{
public:
    simple_mx_pro_strategy_c(std::size_t _short_period = 10,
                             std::size_t _long_period = 20,
                             account_interface * _account = nullptr,
                             strategy_params* _user_configs = nullptr) :
        short_period(_short_period),
        long_period(_long_period),
        account(_account) ,
        user_configs(_user_configs)
    {
        if(user_configs){
            //default values
            user_configs->add_param("min_wait_candle_count" , 2);
            user_configs->add_param("max_wait_candle_count" , 14);
            user_configs->add_param("max_percent_profit" , 0.009);
            user_configs->add_param("min_percent_profit" , 0.004);
            user_configs->add_param("min_avarage_factor" , 0.0005);
            user_configs->add_param("max_rsi" , 66);
            user_configs->add_param("min_rsi" , 45);
            user_configs->add_param("definitely_sell_rsi" , 72);
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("min_wait_candle_count" , 2);
            user_configs->add_param("max_wait_candle_count" , 14);
            user_configs->add_param("max_percent_profit" , 0.009);
            user_configs->add_param("min_percent_profit" , 0.004);
            user_configs->add_param("min_avarage_factor" , 0.0005);
            user_configs->add_param("max_rsi" , 66);
            user_configs->add_param("min_rsi" , 45);
            user_configs->add_param("definitely_sell_rsi" , 72);
        }

        data_short_container = new OHLC_container(short_period);
        data_long_container = new OHLC_container(long_period);
        is_buy_order_set_before = false;;
    }

    trade_strategy_order_e execute() override{

        qsizetype c_size = data_long_container->container.size();

        if ( c_size < long_period ) {
            if(c_size == 1 && !last_price.is_empty()){
                logger_signal(trade_logger_signals::STARTED ,
                              last_price.get_average_all_price(),
                              account);
            }
            return trade_strategy_order_e::PROCESSING;
        }

        double long_moving_avarage = data_long_container->last_moving_avarage;
        double short_moving_avarage = data_short_container->last_moving_avarage;
        double long_rsi  = data_long_container->last_rsi;
        double short_rsi = data_short_container->last_rsi;
        double last_avr_price = last_price.get_average_all_price();

        double _min_wait_candle_count = user_configs->get_param_value("min_wait_candle_count" );
        double _max_wait_candle_count = user_configs->get_param_value("max_wait_candle_count" );
        double _max_percent_profit = user_configs->get_param_value("max_percent_profit");
        double _min_percent_profit = user_configs->get_param_value("min_percent_profit");
        double _min_avarage_factor = user_configs->get_param_value("min_avarage_factor" );
        double _max_rsi = user_configs->get_param_value("max_rsi");
        double _min_rsi = user_configs->get_param_value("min_rsi" );
        double _definitely_sell_rsi = user_configs->get_param_value("definitely_sell_rsi");

        static bool x_flag = false;

        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:
            {
                if(last_avr_price < long_moving_avarage        &&
                    short_rsi < 40                             &&
                    long_rsi  < 50                             &&
                    short_moving_avarage < long_moving_avarage &&
                    IS_SELLED())
                {
                    state = 1;
                    ohlc_counter = 0;
                    x_flag = false;
                }
                break;
            }

            case 1:
            {
                if (last_avr_price > short_moving_avarage &&
                    long_rsi > 50          &&
                    short_moving_avarage > long_moving_avarage        &&
                    short_rsi > 50 )
                {
                    state = 2;
                    return BUY();
                }


                if(x_flag == true &&   short_rsi < 40)
                {
                    state = 0;
                    break;
                }


                if(short_rsi > 50 )
                {
                    x_flag = true;
                }

                break;
            }

            case 2:
            {
                if(last_avr_price < long_moving_avarage          ||
                    short_moving_avarage < long_moving_avarage   ||
                    long_rsi > 60                                ||
                    short_rsi > 80 )
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

        data_short_container->add(ohlc_seed);

        data_long_container->add(ohlc_seed);

        last_price = ohlc_seed;

        logger_signal(trade_logger_signals::LAST_PRICE ,
                      last_price.get_average_all_price(),
                      account);
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
        save_purchased_price = last_price;
        logger_signal(trade_logger_signals::BUYED ,
                      last_price.get_average_all_price(),
                      account);
        if(account)
            account->buy_order();
        return trade_strategy_order_e::BUY;
    }

    trade_strategy_order_e SELL(){
        is_buy_order_set_before = false;
        save_purchased_price.clear();
        logger_signal(trade_logger_signals::SELLED ,
                      last_price.get_average_all_price(),
                      account);
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

    // ALGO params
    int ohlc_counter = 0;
    int state = 0;

private:  // must be static
    std::size_t short_period;
    std::size_t long_period;
    OHLC_s save_purchased_price ; // satın alınan fiyat
    OHLC_s last_price;
    OHLC_container  *data_short_container;
    OHLC_container  *data_long_container;
    account_interface * account;
    bool is_buy_order_set_before;
    strategy_params* user_configs ;
};





#endif // SIMPLE_MX_STRATEGY_H
