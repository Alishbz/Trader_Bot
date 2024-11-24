#ifndef MACD_STRATEGY_H
#define MACD_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

***/


class macd_strategy_c : public trade_strategy_interface{
public:
    macd_strategy_c(std::size_t _short_period = 10,
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
            user_configs->add_param("macd_signal" , _short_period-1);
        }
        /*else{
            user_configs = new strategy_params(); // memory leak
            user_configs->add_param("macd_signal" , 9);
        }*/

        macd_container = new std::vector<double>();
        signal_container = new std::vector<double>();

        data_short_container = new OHLC_container(short_period);
        data_long_container = new OHLC_container(long_period);
        is_buy_order_set_before = false;;
    }

    ~macd_strategy_c() {
        delete data_short_container;
        delete data_long_container;
        delete macd_container;
        delete signal_container;
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

        double _signal_period = user_configs->get_param_value("macd_signal" );

        double macd_value = macd_container->back();
        double signal_value = signal_container->back();
        double short_ema_value = short_ema.back();
        double long_ema_value = long_ema.back();

        bool is_macd_crossing_above_signal = macd_value > signal_value && macd_container->at(macd_container->size() - 2) <= signal_container->at(signal_container->size() - 2);
        bool is_macd_crossing_below_signal = macd_value < signal_value && macd_container->at(macd_container->size() - 2) >= signal_container->at(signal_container->size() - 2);
        bool is_rsi_oversold = true;   //short_rsi < 30;
        bool is_rsi_overbought = true; //hort_rsi > 70;
        bool is_price_above_emas = last_avr_price > short_ema_value && last_avr_price > long_ema_value;
        bool is_price_below_emas = last_avr_price < short_ema_value && last_avr_price < long_ema_value;
        /*
        if (is_macd_crossing_above_signal && is_rsi_oversold && is_price_above_emas && IS_SELLED()) {
            return BUY();
        } else if (is_macd_crossing_below_signal && is_rsi_overbought && is_price_below_emas && IS_BUYED()) {
            return SELL();
        }*/

        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:
            {
                if (IS_SELLED() &&
                    short_moving_avarage <  last_avr_price  )
                {
                    state = 1;
                    break;
                }

                break;
            }


            case 1:
            {
               /* if (is_macd_crossing_above_signal && is_rsi_oversold && is_price_above_emas && IS_SELLED()) {
                    state = 2;
                    return BUY();
                }*/
                 if (is_macd_crossing_below_signal && is_rsi_overbought && is_price_below_emas && IS_SELLED()) {
                    state = 2;
                    return BUY();
                }
                break;
            }

            case 2:
            {
               /* if (is_macd_crossing_below_signal && is_rsi_overbought && is_price_below_emas && IS_BUYED()) {
                    state = 0;
                    return SELL();
                }*/
                if (is_macd_crossing_above_signal && is_rsi_oversold && is_price_above_emas && IS_BUYED()) {
                    state = 0;
                    return SELL();
                }
                break;
            }

            default:
                break;
            }
        }

        /*
        if (macd_value > signal_value && IS_SELLED())
        {
            return BUY();
        }
        else if (macd_value < signal_value && IS_BUYED())
        {
            return SELL();
        }*/

        return trade_strategy_order_e::PROCESSING;
    }


    void feed_ohlc(const OHLC_s& ohlc_seed) override{

        data_short_container->add(ohlc_seed);

        data_long_container->add(ohlc_seed);

        last_price = ohlc_seed;

        double signal_period = user_configs->get_param_value("macd_signal" );

        calculate_ema();
        double macd_value = short_ema.back() - long_ema.back();
        macd_container->push_back(macd_value);

        if (macd_container->size() >= signal_period) {
            calculate_signal();
        }


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

    void calculate_ema() {
        if (short_ema.empty()) {
            short_ema.push_back(last_price.close);
        } else {
            double short_ema_value = (last_price.close - short_ema.back()) * (2.0 / (short_period + 1)) + short_ema.back();
            short_ema.push_back(short_ema_value);
        }

        if (long_ema.empty()) {
            long_ema.push_back(last_price.close);
        } else {
            double long_ema_value = (last_price.close - long_ema.back()) * (2.0 / (long_period + 1)) + long_ema.back();
            long_ema.push_back(long_ema_value);
        }
    }

    void calculate_signal() {

        double signal_period = user_configs->get_param_value("macd_signal" );

        if (signal_container->empty()) {
            signal_container->push_back(macd_container->front());
        } else {
            double signal_value = (macd_container->back() - signal_container->back()) * (2.0 / (signal_period + 1)) + signal_container->back();
            signal_container->push_back(signal_value);
        }
    }

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

    std::vector<double> *macd_container;
    std::vector<double> *signal_container;
    std::vector<double> short_ema;
    std::vector<double> long_ema;
};





#endif
