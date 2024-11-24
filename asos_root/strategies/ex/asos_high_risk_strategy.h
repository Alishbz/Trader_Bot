#ifndef ASOS_HIGH_RISK_STRATEGY_H
#define ASOS_HIGH_RISK_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:


***/


class asos_high_risk_strategy_c : public trade_strategy_interface{

public:
    asos_high_risk_strategy_c(std::size_t _short_period = 5,
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
            user_configs->add_param("rsi_for_buy" , 30);
            user_configs->add_param("rsi_for_sell" , 72);
            user_configs->add_param("min_ohlc_wait_count" , 3);
            user_configs->add_param("max_ohlc_wait_count" , 12);
            user_configs->add_param("min_ma_factor" , 0.0006);
            user_configs->add_param("take_max_profit_factor"  , 0.007);
            // ...
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("rsi_for_buy" , 30);
            user_configs->add_param("rsi_for_sell" , 72);
            user_configs->add_param("min_ohlc_wait_count" , 3);
            user_configs->add_param("max_ohlc_wait_count" , 12);
            user_configs->add_param("min_ma_factor" , 0.0006);
            user_configs->add_param("take_max_profit_factor" , 0.007);
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

        // @calculaters
        double purchased_price = save_purchased_price.get_average_all_price();
        double long_moving_avarage = data_long_container->last_moving_avarage;
        double short_moving_avarage = data_short_container->last_moving_avarage;
        double last_avr_price = last_price.get_average_all_price();
        double short_rsi = data_short_container->last_rsi;
        // double long_rsi  = data_long_container->last_rsi;


        // @USER configs
        double _RSI_FOR_BUY = user_configs->get_param_value("rsi_for_buy") ;
        double _RSI_FOR_SELL = user_configs->get_param_value("rsi_for_sell") ;
        double _MIN_OHLC_MAIT = user_configs->get_param_value("min_ohlc_wait_count") ;
        double _MAX_OHLC_MAIT = user_configs->get_param_value("max_ohlc_wait_count") ;
        double _MIN_MA_FACTOR = user_configs->get_param_value("min_ma_factor") ;
        double _MAX_PROFIT_FACTOR = user_configs->get_param_value("take_max_profit_factor") ;

        if(long_moving_avarage > 0){

            switch (state)
            {

            case 0:  // wait idle step
            {

                if (last_avr_price > PERCENT_ADDER(short_moving_avarage,_MAX_PROFIT_FACTOR) &&
                    short_moving_avarage > long_moving_avarage &&
                    short_rsi > _RSI_FOR_SELL &&
                    IS_SELLED())
                {
                    log_print("normal short process triggered ");
                    ohlc_counter = 0;
                    state = 4;
                    break;
                }

                if (last_avr_price > short_moving_avarage &&
                    IS_SELLED())
                {
                    ohlc_counter = 0;
                    state = 1;
                    break;
                }

                break;
            }

            case 1:  // process could be start wait for it to fall
            {
                ohlc_counter++;

                if (short_rsi > _RSI_FOR_BUY &&
                    last_avr_price < short_moving_avarage &&
                    ohlc_counter > _MIN_OHLC_MAIT &&
                    ohlc_counter < _MAX_OHLC_MAIT)
                {
                    state = 2;
                    ohlc_counter = 0;
                    break;
                }

                if(ohlc_counter >= _MAX_OHLC_MAIT){
                    state = 0;
                    ohlc_counter = 0;
                    break;
                }

                break;
            }


            case 2:  // check second MA pass green candle
            {
                ohlc_counter++;

                if (last_avr_price > PERCENT_ADDER(short_moving_avarage,_MIN_MA_FACTOR) &&
                    ohlc_counter < _MAX_OHLC_MAIT)
                {
                    state = 3;

                    ohlc_counter = 0;

                    return BUY_LONG();
                }

                if(ohlc_counter >= _MAX_OHLC_MAIT){
                    state = 0;
                    ohlc_counter = 0;
                }

                break;
            }


            case 3:  // wait SELL
            {
                ohlc_counter++;

                if(last_avr_price > short_moving_avarage){
                    ohlc_counter = 0;
                }

                if (short_rsi > _RSI_FOR_SELL &&
                    long_moving_avarage  < PERCENT_SUBTRACTOR( short_moving_avarage , _MIN_MA_FACTOR) &&
                    ohlc_counter > _MIN_OHLC_MAIT &&
                    IS_BUY_LONG_DONE())
                {
                    // profit sale

                    log_print("profit sale , short process triggered :)");

                    state = 4;
                    ohlc_counter = 0;
                    return SELL();
                }

                if (last_avr_price < PERCENT_SUBTRACTOR(short_moving_avarage, _MIN_MA_FACTOR) &&
                    ohlc_counter >= _MIN_OHLC_MAIT &&
                    IS_BUY_LONG_DONE())
                {
                    // loss sale
                    log_print("loss sale :(");

                    state = 0;
                    ohlc_counter = 0;
                    return SELL();
                }

                if (last_avr_price > PERCENT_ADDER( purchased_price , _MAX_PROFIT_FACTOR) &&
                    IS_BUY_LONG_DONE())
                {
                    log_print("max profit sale , short process triggered :)");

                    state = 4;
                    ohlc_counter = 0;
                    return SELL();
                }

                break;
            }


            case 4:  //  SHORT process
            {
                ohlc_counter++;

                if (!last_price.is_green() &&
                    last_avr_price > PERCENT_ADDER(short_moving_avarage, _MIN_MA_FACTOR) &&
                    IS_SELLED() )
                {
                    state = 5;

                    ohlc_counter = 0;

                    return BUY_SHORT();
                }

                if( ohlc_counter >= _MIN_OHLC_MAIT ||
                    last_avr_price < short_moving_avarage)
                {
                    log_print("short process NOT AVAILABLE");
                    state = 0;
                    ohlc_counter = 0;
                }

                break;
            }

            case 5:  // wait SELL for SHORT
            {
                ohlc_counter++;

                if ( ohlc_counter >= _MIN_OHLC_MAIT &&
                    last_avr_price > PERCENT_ADDER( purchased_price , _MAX_PROFIT_FACTOR) &&
                    IS_BUY_SHORT_DONE())
                {
                    log_print("we did loss in short position :( ");

                    state = 0;
                    ohlc_counter = 0;
                    return SELL();
                }

                if (last_avr_price < short_moving_avarage &&
                    //last_avr_price < PERCENT_SUBTRACTOR( purchased_price , _MAX_PROFIT_FACTOR/2) &&
                    IS_BUY_SHORT_DONE())
                {
                    log_print("probably we did win in short position :) ");

                    state = 0;
                    ohlc_counter = 0;
                    return SELL();
                }

                break;
            }



            default:
                state = 0;
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
    bool is_short_order_set_before;
};





#endif
