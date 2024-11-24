#ifndef FIDORSI_STRATEGY_H
#define FIDORSI_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

***/


class fidorsi_strategy_c : public trade_strategy_interface{
public:
    fidorsi_strategy_c(std::size_t _short_period = 10,
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
            user_configs->add_param("max_rsi" , 65.9);
            user_configs->add_param("min_rsi" , 32.1);
            user_configs->add_param("sell_protection_avarage" , 0.001);
            user_configs->add_param("buy_protection_avarage" , 0.001);
        }
        /*else{
            user_configs = new strategy_params(); // memory leak
            user_configs->add_param("max_rsi" , 66.9);
            user_configs->add_param("min_rsi" , 34.1);
            user_configs->add_param("sell_protection_avarage" , 0.003);
            user_configs->add_param("buy_protection_avarage" , 0.002);
        }*/

        data_short_container = new OHLC_container(short_period);
        data_long_container = new OHLC_container(long_period);
        is_buy_order_set_before = false;;
    }

    ~fidorsi_strategy_c() {
        delete data_short_container;
        delete data_long_container;
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
        double slope_short_moving_avarage = data_short_container->last_slope;

        double _max_rsi = user_configs->get_param_value("max_rsi" );
        double _min_rsi = user_configs->get_param_value("min_rsi" );
        double _sell_protection_avarage = user_configs->get_param_value("sell_protection_avarage" );
        double _buy_protection_avarage = user_configs->get_param_value("buy_protection_avarage" );

        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:
            {
                if (IS_SELLED() &&
                    short_rsi < _min_rsi &&
                    last_avr_price < short_moving_avarage)
                {
                    if(!buy_price_recorder.is_empty()){
                        if(last_avr_price < buy_price_recorder.close)
                            buy_price_recorder = last_price;
                    }
                    else{
                        buy_price_recorder = last_price;
                    }
                    break;
                }

                if(!buy_price_recorder.is_empty()){

                    if(last_avr_price > PERCENT_ADDER(buy_price_recorder.close , _buy_protection_avarage) )
                    {
                        sell_price_recorder.clear();
                        state = 1;
                        return BUY();
                    }

                }
                break;
            }


            case 1:
            {
                if (IS_BUYED() &&
                    short_rsi > _max_rsi &&
                    last_avr_price > short_moving_avarage)
                {
                    if(!sell_price_recorder.is_empty()){
                        if(last_avr_price > sell_price_recorder.close)
                            sell_price_recorder = last_price;
                    }
                    else{
                        sell_price_recorder = last_price;
                    }
                    break;
                }

                if(!sell_price_recorder.is_empty()){

                    if(last_avr_price < PERCENT_SUBTRACTOR(sell_price_recorder.close , _sell_protection_avarage) ){

                        state = 0;
                        buy_price_recorder.clear();
                        return SELL();
                    }

                }

                break;
            }


            default:
                break;
            }
        }


        /*
        if (IS_SELLED() &&
            short_rsi < _min_rsi &&
            last_avr_price < short_moving_avarage) {
            return BUY();
        }
        else if (IS_BUYED() &&
                 short_rsi > _max_rsi &&
                 last_avr_price > short_moving_avarage) {
            return SELL();
        }
*/
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

    OHLC_s sell_price_recorder ;
    OHLC_s buy_price_recorder ;

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





#endif
