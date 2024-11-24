#ifndef AAXE_LONG_STRATEGY_H
#define AAXE_LONG_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**

 Strateji açıklaması:
  bu şuan düşüşden çok kazanıyor
SHORT lar ile ilgili hiç bir katsayı düzgün çalışmıyor , KAR alma bile neden? : örnek gün: 2024-02-29T09:00:00
***/

class aaxe_long_st_c : public trade_strategy_interface{

public:
    aaxe_long_st_c(std::size_t _short_period = 26,
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
            user_configs->add_param("max_dif_long_ma_short_ma" , 0.022);
            user_configs->add_param("take_max_profit_factor"  , 0.033);
            user_configs->add_param("dangerous_ohlc_size" , 0.65);
            user_configs->add_param("sell_trig_percent" , 0.0030);
            user_configs->add_param("take_loss_percent" , 0.0145);
            user_configs->add_param("max_ohlc_signal_period" , 10.0);
            // ...
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("max_dif_long_ma_short_ma" , 0.022);
            user_configs->add_param("take_max_profit_factor"  , 0.033);
            user_configs->add_param("dangerous_ohlc_size" , 0.65);
            user_configs->add_param("sell_trig_percent" , 0.0030);
            user_configs->add_param("take_loss_percent" , 0.0145);
            user_configs->add_param("max_ohlc_signal_period" , 10.0);
        }
    }

    enum class state_e {
        WAIT_NOMINATED_SIGNAL,
        WAIT_POSITION_DETECT_SIGNAL,
        WAIT_NORMAL_SHORT_SELL_SIGNAL,
        WAIT_NORMAL_LONG_SELL_SIGNAL,
        WAIT_HIGH_SHORT_SELL_SIGNAL,
        WAIT_LOW_LONG_SELL_SIGNAL
    };

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

        const double long_moving_avarage = data_long_container->last_moving_avarage;

        if(long_moving_avarage > 0){

            switch (state)
            {

            case state_e::WAIT_NOMINATED_SIGNAL:
            {
                bool signal = _is_market_nominated();

                if(signal == true && IS_SELLED()){
                    GOTO_STEP(state_e::WAIT_POSITION_DETECT_SIGNAL);
                    // return trade_strategy_order_e::PROCESSING_MARK_1;
                }

                break;
            }

            case state_e::WAIT_POSITION_DETECT_SIGNAL:
            {
                bool signal = _find_short_buy_signal();
                if(signal == true && IS_SELLED()){
                    if(last_price.close > long_moving_avarage){
                        GOTO_STEP(state_e::WAIT_HIGH_SHORT_SELL_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NORMAL_SHORT_SELL_SIGNAL);
                    }
                    return BUY_SHORT();
                }

                signal = _find_long_buy_signal();
                if(signal == true && IS_SELLED()){
                    if(last_price.close > long_moving_avarage){
                        GOTO_STEP(state_e::WAIT_NORMAL_LONG_SELL_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_LOW_LONG_SELL_SIGNAL);
                    }
                    return BUY_LONG();
                }

                break;
            }

            case state_e::WAIT_NORMAL_SHORT_SELL_SIGNAL:
            {
                if(_find_short_sell_signal(false) && IS_BUY_SHORT_DONE()){
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_NORMAL_LONG_SELL_SIGNAL:
            {
                if(_find_long_sell_signal(false) && IS_BUY_LONG_DONE()){
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_HIGH_SHORT_SELL_SIGNAL:
            {
                if(_find_short_sell_signal(true) && IS_BUY_SHORT_DONE()){
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_LOW_LONG_SELL_SIGNAL:
            {
                if(_find_long_sell_signal(true) && IS_BUY_LONG_DONE()){
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    return SELL();
                }
                break;
            }

            default:
                log_print(" HARD ERROR XX458 ");
                GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                break;
            }
        }

        return trade_strategy_order_e::PROCESSING;
    }

private:

    bool _find_short_buy_signal() {

        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("dangerous_ohlc_size") ;
        const double _MA_DIF_FACTOR = user_configs->get_param_value("max_dif_long_ma_short_ma") ;
        const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;

        std::pair<bool, float> triple_out =  data_short_container->is_last_three_data_red();
        int red_ohlc_count = data_short_container->get_red_ohlc_count_in_period(_MAX_OHLC_PERIOD);

        if(last_price.close < long_moving_avarage){

            // normal SHORT emri, düşüşü öngörmek, MA ler bizi destekler
            if(triple_out.first == true &&
                triple_out.second > _DANGEROUS_OHLC_SIZE &&
                last_price.close < short_moving_avarage )
            {
               // return true;
            }
        }
        else if(last_price.close > PERCENT_ADDER( long_moving_avarage , _MA_DIF_FACTOR)){

            // şişkin fiyat , SHORT fırsatı olabilir
            if(triple_out.first == true &&
                triple_out.second > _DANGEROUS_OHLC_SIZE &&
                red_ohlc_count > (_MAX_OHLC_PERIOD/2) )
            {
               // return true;
            }
        }

        return false;
    }

    bool _find_long_buy_signal() {

        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _DANGEROUS_OHLC_SIZE = user_configs->get_param_value("dangerous_ohlc_size") ;
        const double _MA_DIF_FACTOR = user_configs->get_param_value("max_dif_long_ma_short_ma") ;
        const double _MAX_OHLC_PERIOD = user_configs->get_param_value("max_ohlc_signal_period") ;

        std::pair<bool, float> triple_out =  data_short_container->is_last_three_data_green();
        int green_ohlc_count = data_short_container->get_green_ohlc_count_in_period(_MAX_OHLC_PERIOD);

        if(last_price.close > long_moving_avarage){

            // normal LONG emri, yükselişi öngörmek, MA ler bizi destekler
            if( triple_out.first == true &&
                triple_out.second > _DANGEROUS_OHLC_SIZE &&
                last_price.close > short_moving_avarage )
            {
                return true;
            }

        }
        else if(last_price.close < PERCENT_SUBTRACTOR( long_moving_avarage , _MA_DIF_FACTOR)){

            // çok derin düşmüş fiyat , LONG fırsatı olabilir
            if(triple_out.first == true &&
                triple_out.second > _DANGEROUS_OHLC_SIZE &&
                green_ohlc_count > (_MAX_OHLC_PERIOD/2)-2 )
            {
                return true;
            }
        }

        return false;
    }

    bool _is_market_nominated() {

        // Belki ilk açılışta sadece 1 kere LONG_MA ve SHORT_MA yakınlaşması beklenebilinir?
        return true;
    }

    /****/

    bool _find_short_sell_signal(bool select_is_high) {

        const double purchased_price = save_purchased_price.get_average_all_price();
        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _MAX_PROFIT_FACTOR = user_configs->get_param_value("take_max_profit_factor") ;
        const double _SELL_TRIG_VAL = user_configs->get_param_value("sell_trig_percent") ;
        const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;


        if (last_price.low < PERCENT_SUBTRACTOR( purchased_price , _MAX_PROFIT_FACTOR))
        {
            log_print("max profit sale for SHORT :)");
            return true;
        }

        if(select_is_high == true){ // LONG_MA yüksek iken aldık

            if (last_price.close > PERCENT_ADDER(purchased_price,_TAKE_LOSS_FACTOR))
            {
                // zarar kes , aksi durumda KARI GÖRENE KADAR SATMA DAYI !!
                return true;
            }
        }
        else{ // LONG_MA düşük iken aldık , yüksekken satmak gerek

            if (last_price.close > PERCENT_ADDER(short_moving_avarage,_SELL_TRIG_VAL))
            {
                // yüksek ihtimal zarar ettik , ama bu tetikleme olmaz ise LONG pozisyon kaçırabiliriz
                return true;
            }

            if (last_price.low > long_moving_avarage)
            {
                // zararsada karsada burda satmak ZORUNLU
                return true;
            }
        }

        return false;
    }

    bool _find_long_sell_signal(bool select_is_low) {

        const double purchased_price = save_purchased_price.get_average_all_price();
        const double long_moving_avarage = data_long_container->last_moving_avarage;
        const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double _MAX_PROFIT_FACTOR = user_configs->get_param_value("take_max_profit_factor") ;
        const double _SELL_TRIG_VAL = user_configs->get_param_value("sell_trig_percent") ;
        const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;


        if (last_price.high > PERCENT_ADDER( purchased_price , _MAX_PROFIT_FACTOR))
        {
            log_print("max profit sale for LONG :)");
            return true;
        }

        if(select_is_low == true){  // LONG_MA düşük iken aldık

            if (last_price.close < PERCENT_SUBTRACTOR(purchased_price, _TAKE_LOSS_FACTOR))
            {
                // zarar kes, aksi durumda KARI GÖRENE KADAR SATMA DAYI !!
                 return true;
            }
        }
        else{  // LONG_MA yüksek iken aldık , düşerken satmak gerek
/*
            if (last_price.close < PERCENT_SUBTRACTOR(short_moving_avarage, _SELL_TRIG_VAL))
            {
                // yüksek ihtimal zarar ettik , ama bu tetikleme olmaz ise SHORT pozisyon kaçırabiliriz
                return true;
            }
*/
            if (last_price.high < long_moving_avarage)
            {
                // zararsada karsada burda satmak ZORUNLU
                return true;
            }
        }


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

    }

    void feed_price_quantity_volume(double quantity) override{

    }


private:

    void GOTO_STEP(state_e stepx){
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
    state_e state = state_e::WAIT_NOMINATED_SIGNAL;
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
