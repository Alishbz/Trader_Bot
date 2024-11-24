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
#ifndef TEMPLATE_STRATEGY_H
#define TEMPLATE_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

***/

class template_strategy_c : public trade_strategy_interface{
public:
    template_strategy_c(std::size_t _short_period = 5,
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
        is_short_order_set_before = false;

        if(user_configs){
            //default values
            // user_configs->add_param("rsi_max" , 30);
            // write your own configs
        }
        else{
            user_configs = new strategy_params();
            //user_configs->add_param("rsi_max" , 30);
            // write your own configs
        }

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

        /*
        double long_moving_avarage = data_long_container->last_moving_avarage;
        double short_moving_avarage = data_short_container->last_moving_avarage;
        double long_rsi  = data_long_container->last_rsi;
        double short_rsi = data_short_container->last_rsi;
        double last_avr_price = last_price.get_average_all_price();
        */

        // log_print("test");  you can print

        //double _rsi_max = user_configs->get_param_value("rsi_max") ;  // get your configs like that

        // TODO Write me a streatgy for earn money

        // return BUY_LONG(); // means my API will buy

        // return BUY_SHORT(); // means my API will short

        // return SELL(); // SELL for LONG & SHORT

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

        // todo hacim/quantity is here
    }

private:

    trade_strategy_order_e BUY_LONG(){
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
        is_short_order_set_before = false;
        save_purchased_price.clear();
        logger_signal(trade_logger_signals::SELLED ,
                      last_price.get_average_all_price(),
                      account);
        if(account)
            account->sell_order();
        return trade_strategy_order_e::SELL;
    }

    trade_strategy_order_e BUY_SHORT(){
        is_short_order_set_before = true;
        save_purchased_price = last_price;
        logger_signal(trade_logger_signals::SHORT ,
                      last_price.get_average_all_price(),
                      account);
        if(account)
            account->short_order();
        return trade_strategy_order_e::SHORT;
    }

    bool IS_BUY_LONG_DONE(){
        return is_buy_order_set_before;
    }

    bool IS_SELLED(){
        if(is_buy_order_set_before == false && is_short_order_set_before == false )
            return true;
        return false;
    }

    bool IS_BUY_SHORT_DONE(){
        return is_short_order_set_before;
    }

    // ALGO SET configs

    // ALGO params

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
    bool is_short_order_set_before;
};






#endif
