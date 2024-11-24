#ifndef FIRE_STRATEGY_H
#define FIRE_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

***/

class fire_strategy_c : public trade_strategy_interface{
public:
    fire_strategy_c(std::size_t _short_period = 5,
                    std::size_t _long_period = 10,
                    account_interface * _account = nullptr,
                    strategy_params* _user_configs = nullptr) :
        short_period(_short_period),
        long_period(_long_period),
        account(_account),
        user_configs(_user_configs)
    {
        data_short_container = new OHLC_container(short_period);
        data_long_container = new OHLC_container(long_period);
        is_buy_order_set_before = false;


        if(user_configs){
            //default values
            user_configs->add_param("rsi_max" , 70);
            user_configs->add_param("rsi_min" , 35);
            user_configs->add_param("slope_max" , 2);
            user_configs->add_param("slope_min" , -3);
            user_configs->add_param("ohlc_wait_count_after_deep" , 12);
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("rsi_max" , 70);
            user_configs->add_param("rsi_min" , 35);
            user_configs->add_param("slope_max" , 2);
            user_configs->add_param("slope_min" , -3);
            user_configs->add_param("ohlc_wait_count_after_deep" , 12);
        }

    }

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
        double short_slope = data_short_container->last_slope;
        double long_slope = data_long_container->last_slope;

        double _rsi_max = user_configs->get_param_value("rsi_max") ;
        double _rsi_min = user_configs->get_param_value("rsi_min") ;
        double _slope_min = user_configs->get_param_value("slope_min") ;
        double _slope_max = user_configs->get_param_value("slope_max") ;
        double _ohlc_wait_count_after_deep  = user_configs->get_param_value("ohlc_wait_count_after_deep") ;

        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:  // idle , wait signal
            {
                if (IS_SELLED() &&
                    short_rsi < _rsi_min )
                {
                    // it is deep signal
                    ohlc_counter = 0;
                    state = 1;
                }

                break;
            }

            case 1:  // deep signal appeared
            {
                ohlc_counter++;

                if (IS_SELLED() &&
                    short_moving_avarage > long_moving_avarage &&
                    short_slope > _slope_max &&
                    short_rsi < 58 )                       // 50 is middele of rsi
                {
                    // it is buy signal
                    ohlc_counter = 0;
                    state = 2;
                    return BUY();
                }

                if(ohlc_counter > _ohlc_wait_count_after_deep)
                {
                    // goto idle state
                    ohlc_counter = 0;
                    state = 0;
                }
                break;
            }

            case 2:  // exit waiting
            {
                ohlc_counter++;

                if (IS_BUYED() &&
                    ohlc_counter > 2 &&         // for little think time
                    short_rsi > _rsi_max  )
                {
                    ohlc_counter = 0;
                    state = 0;
                    return SELL();
                }
                break;
            }
            }
        }

        return trade_strategy_order_e::PROCESSING;
    }

    void feed_ohlc(const OHLC_s& ohlc_seed) override{

        data_short_container->add(ohlc_seed);

        data_long_container->add(ohlc_seed);

        last_price = ohlc_seed;

    }

    void feed_depth(double bids_ratio) override{

        // todo add depth to list in a period and look in 1 minute chnages radio ?
    }

    void feed_price_quantity_volume(double quantity) override{

        // todo hacmi/quantity is here
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

    // ALGO SET configs



    // ALGO params
    int ohlc_counter = 0;
    int state = 0;

private:  // must be static
    std::size_t short_period;
    std::size_t long_period;
    OHLC_s save_purchased_price ; // buy price
    OHLC_s last_price;
    OHLC_container  *data_short_container;
    OHLC_container  *data_long_container;
    account_interface * account;
    strategy_params* user_configs ;  // all configs by connected GUI
    bool is_buy_order_set_before;
};






#endif
