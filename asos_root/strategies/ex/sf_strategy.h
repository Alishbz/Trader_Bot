#ifndef SF_STRATEGY_H
#define SF_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**

 Strateji açıklaması: SF'in amacı trendi yakalamak ve trende bağlı SHORT
                      veya LONG yönlü pozisyon açmaktır, yüksek kar hedefler

   - 160_MA nın altında kalan hareketler trend düşüş eğilimindedir ,
   - 160_MA in üstünde ise trend yükseliş eğiliminde demektir
   - 160_MA nın son 10 değerinin eğimi trendin eğimini verir
   - 26_MA bizim için satım noktalarını verir,
      - SHORT pozisyonda 26_MA yukarıdan aşağı kesildiyse satılır,
      - LONG pozisyonda 26_MA aşağıdan yukarı kesildiyse satılır,
   - SHORT SIGNAL: 160_MA yukarında aşağı kesildi ve aşağıda min 3 kırmızı MUM yaktı ise
   - LONG SIGNAL: 160_MA aşağıdan yukarı kesildi ve aşağıda min 3 yeşil MUM yaktı ise

   - Maks KAR: %2.2 ;
     Yüksek olmasının sebebi, 160 ın altı ve üstünde yüksek kar
     oranlarını yakalamak, bunun dışında MA_26 zaten bizi koruyacak ,
     hatta kar ettirebilecek

***/

class sf_strategy_c : public trade_strategy_interface{

public:
    sf_strategy_c(std::size_t _short_period = 26,
                  std::size_t _long_period = 160,
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
            user_configs->add_param("min_standart_dev" , 450);
            user_configs->add_param("max_ohlc_signal_period" , 12);
            user_configs->add_param("min_short_long_ma_dif" , 0.0512);
            user_configs->add_param("take_max_profit_factor"  , 0.019);
            user_configs->add_param("Dangerous_OHLC_Size_%" , 0.61);
            user_configs->add_param("sell_trig_percent" , 0.0039);
            // ...
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("min_standart_dev" , 450);
            user_configs->add_param("max_ohlc_signal_period" , 12);
            user_configs->add_param("min_short_long_ma_dif" , 0.0512);
            user_configs->add_param("take_max_profit_factor"  , 0.019);
            user_configs->add_param("Dangerous_OHLC_Size_%" , 0.61);
            user_configs->add_param("sell_trig_percent" , 0.0039);
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
        const double purchased_price = save_purchased_price.get_average_all_price();
        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;

        // @USER configs
        const double _MAX_PROFIT_FACTOR = user_configs->get_param_value("take_max_profit_factor") ;
        const double _SELL_TRIG_VAL = user_configs->get_param_value("sell_trig_percent") ;

        if(long_moving_avarage > 0){

            switch (state)
            {

            case 0:  // wait nominated
            {
                bool signal = _is_market_nominated();

                if(signal == true && IS_SELLED()){
                    GOTO_STEP(1);
                    // return trade_strategy_order_e::PROCESSING_MARK_1;
                }

                break;
            }

            case 1:  // wait REAL buy short SİGNALS
            {
                bool signal = _find_short_signal();
                if(signal == true && IS_SELLED()){
                    GOTO_STEP(6);
                    return BUY_SHORT();
                }

                signal = _find_long_signal();
                if(signal == true && IS_SELLED()){
                    GOTO_STEP(11);
                    return BUY_LONG();
                }

                break;
            }

            case 6:   // SHORT process working , wait SELL signals
            {
                if (last_price.close > PERCENT_ADDER(short_moving_avarage,_SELL_TRIG_VAL) &&
                    IS_BUY_SHORT_DONE())
                {
                    GOTO_STEP(0);
                    return SELL();
                }

                if (last_price.low < PERCENT_SUBTRACTOR( purchased_price , _MAX_PROFIT_FACTOR) &&
                    IS_BUY_SHORT_DONE())
                {
                    log_print("max profit sale for SHORT :)");
                    GOTO_STEP(0);
                    return SELL();
                }

                if (last_price.low > long_moving_avarage &&
                    IS_BUY_SHORT_DONE())
                {
                    GOTO_STEP(0);
                    return SELL();
                }

                break;
            }

            case 11:   // LONG process working , wait SELL signals
            {
                if (last_price.close < PERCENT_SUBTRACTOR(short_moving_avarage, _SELL_TRIG_VAL) &&
                    IS_BUY_LONG_DONE())
                {
                    GOTO_STEP(0);
                    return SELL();
                }

                if (last_price.high > PERCENT_ADDER( purchased_price , _MAX_PROFIT_FACTOR) &&
                    IS_BUY_LONG_DONE())
                {
                    log_print("max profit sale for LONG :)");
                    GOTO_STEP(0);
                    return SELL();
                }

                if (last_price.high < long_moving_avarage &&
                    IS_BUY_LONG_DONE())
                {
                    GOTO_STEP(0);
                    return SELL();
                }

                break;
            }

            default:
                log_print(" HARD ERROR XX458 ");
                state = 0;
                break;
            }
        }

        return trade_strategy_order_e::PROCESSING;
    }

private:

    bool _find_short_signal()const{
/*
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("Dangerous_OHLC_Size_%") ;
        const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;

        int red_ohlc_count = data_short_container->get_red_ohlc_count_in_period(_MAX_OHLC_PERIOD);
        int high_red_counter = data_short_container->get_desire_red_ohlc_size_count_in_period(_MAX_OHLC_PERIOD ,_DANGEROUS_OHLC_SIZE );

        if(high_red_counter > 0 &&
            red_ohlc_count > _MAX_OHLC_PERIOD/2 &&
            last_price.close < short_moving_avarage)
        {
            return true;
        }*/

        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("Dangerous_OHLC_Size_%") ;
        //const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;

        std::pair<bool, float> triple_out =  data_short_container->is_last_three_data_red();

        if(triple_out.first == true &&
           triple_out.second > _DANGEROUS_OHLC_SIZE &&
           last_price.close < short_moving_avarage &&
           last_price.close < long_moving_avarage)
        {
            return true;
        }
        return false;
    }

    bool _find_long_signal()const{
/*
        const double short_moving_avarage = data_short_container->last_moving_avarage;

        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("Dangerous_OHLC_Size_%") ;
        const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;

        int green_ohlc_count = data_short_container->get_green_ohlc_count_in_period(_MAX_OHLC_PERIOD);
        int high_green_counter = data_short_container->get_desire_green_ohlc_size_count_in_period(_MAX_OHLC_PERIOD ,_DANGEROUS_OHLC_SIZE );

        if(high_green_counter > 0 &&
            green_ohlc_count > _MAX_OHLC_PERIOD/2 &&
            last_price.close > short_moving_avarage)
        {
            return true;
        }*/

        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("Dangerous_OHLC_Size_%") ;
        //const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;
        const double long_moving_avarage = data_long_container->last_moving_avarage;

        std::pair<bool, float> triple_out =  data_short_container->is_last_three_data_green();

        if( triple_out.first == true &&
            triple_out.second > _DANGEROUS_OHLC_SIZE &&
            last_price.close > short_moving_avarage &&
            last_price.close > long_moving_avarage)
        {
            return true;
        }

        return false;
    }

    bool _is_market_nominated()const{

        const double _MIN_STD_DEV = user_configs->get_param_value("min_standart_dev") ;
        const double _MAX_MA_FACTOR = user_configs->get_param_value("min_short_long_ma_dif") ;

        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
/*
        double ma_difference_percentage = 0;


        static bool trig_signal = false;
        static bool long_bigger_than_short = false;

        if(trig_signal == true){
            if(long_bigger_than_short == true){
                if(long_moving_avarage > short_moving_avarage){
                    return false;
                }
            }
            else{
                if(long_moving_avarage < short_moving_avarage){
                    return false;
                }
            }
        }

        if(short_moving_avarage > long_moving_avarage)
            ma_difference_percentage = (short_moving_avarage - long_moving_avarage) / short_moving_avarage  * 100.0;
        else
            ma_difference_percentage = (long_moving_avarage - short_moving_avarage) / long_moving_avarage  * 100.0;

        if (ma_difference_percentage <= _MAX_MA_FACTOR) {

            trig_signal = true;

            if(long_moving_avarage > short_moving_avarage)
                long_bigger_than_short = true;
            else
                long_bigger_than_short = false;

           // if(data_short_container->last_standard_deviation < _MIN_STD_DEV )
                return true;
        }*/

       // if(data_short_container->last_standard_deviation < _MIN_STD_DEV )
            return true;

        return false;
    }

public:

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

    void GOTO_STEP(int stepx){
        state = stepx;
        ohlc_counter_long = 0;
        ohlc_counter_short = 0;
    }

    trade_strategy_order_e BUY_LONG(){
        is_buy_order_set_before = true;
        is_short_order_set_before = false;
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
        is_buy_order_set_before = false;
        save_purchased_price = last_price;
        logger_signal(trade_logger_signals::SHORT ,
                      last_price.get_average_all_price(),
                      account);
        if(account)
            account->short_order();
        return trade_strategy_order_e::SHORT;
    }

    bool IS_BUY_LONG_DONE()const{
        return is_buy_order_set_before;
    }

    bool IS_SELLED()const{
        if(is_buy_order_set_before == false && is_short_order_set_before == false )
            return true;
        return false;
    }

    bool IS_BUY_SHORT_DONE()const{
        return is_short_order_set_before;
    }

    // ALGO params
    int ohlc_counter_long = 0;
    int ohlc_counter_short = 0;
    int state = 0;
    bool short_trig_wait_flag = false;
    bool long_trig_wait_flag = false;

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
