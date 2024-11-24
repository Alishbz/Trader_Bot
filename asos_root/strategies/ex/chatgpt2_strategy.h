#ifndef CHATGPT2_STRATEGY_H
#define CHATGPT2_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

***/
// not: dikkat periyodu 10 yapınca bu algoritma iyi tepki verdi
class chatgpt2_strategy_c : public trade_strategy_interface{
public:
    chatgpt2_strategy_c(std::size_t _short_period = 5,
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
            user_configs->add_param("rsi_max" , 30);
            user_configs->add_param("rsi_min" , 70);
            user_configs->add_param("slope_max" , 7);
            user_configs->add_param("slope_min" , -8);
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("rsi_max" , 30);
            user_configs->add_param("rsi_min" , 70);
            user_configs->add_param("slope_max" , 7);
            user_configs->add_param("slope_min" , -8);
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

        if (IS_SELLED() &&
            short_slope < user_configs->get_param_value("slope_min") &&
            short_moving_avarage < long_moving_avarage &&
            short_rsi > user_configs->get_param_value("rsi_max"))
        {
            return BUY();
        }
        else if (IS_BUYED() &&
            short_slope > user_configs->get_param_value("slope_max") &&
            short_moving_avarage > long_moving_avarage &&
            short_rsi < user_configs->get_param_value("rsi_min") )
        {
            return SELL();
        }

        // return BUY(); // means my API will buy

        // return SELL(); // means my API will sell

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
