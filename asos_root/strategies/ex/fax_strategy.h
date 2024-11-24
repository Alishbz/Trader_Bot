#ifndef FAX_STRATEGY_H
#define FAX_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**
 Strateji açıklaması:

fax algorithm: Yükselişleri yakalama


- step_0:   last_avr_price < MA_20 &&
            MA_20_eğimi < -10        : dip bulundu -> goto step1

- step_1:   MA_20_eğimi > 40  &&
            last_avr_price > MA_20 + MA_20*3/1000 : yükseliş ilk dalgası bulundu -> goto step2

            last_avr_price < MA_20 :  Not find, goto step_0

- step_2:   last_avr_price '=' MA_20 +- 1/1000
            MA_20_eğimi > 10        : fiyat MA_20 ye tekrar yaklaştı -> goto step3

            last_avr_price < MA_20 - MA_20*3/1000 :  Not find, goto step_0

- step_3:   MA_20_eğimi > 30  &&
            last_avr_price > MA_20 + MA_20*2/1000 : yükseliş ikinci dalgası bulundu
                                                  -> BUY()    goto step4

            last_avr_price < MA_20 - MA_20*2/1000 :  Not find, goto step_0


- step_4: 	last_avr_price < 	MA_100   -> SELL , goto step_0

            last_avr_price > last_buy_price + last_buy_price*10/1000   -> kar çok , goto step_5


- step_5: 	ilk veya 2. kırmızı mumda sat	SELL -> 		goto step_0

***/


class fax_strategy_c : public trade_strategy_interface{
public:
    fax_strategy_c(std::size_t _short_period = 10,
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
            user_configs->add_param("deep_detect_slope" , -10);
            user_configs->add_param("nominal_detect_slope" , 10);
            user_configs->add_param("rise_detect_slope" , 40);
            user_configs->add_param("high_rise_rate" , 0.003);
            user_configs->add_param("mid_rise_rate" ,  0.002);
            user_configs->add_param("low_rise_rate" ,  0.001);
            user_configs->add_param("profit" ,  0.01);
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("deep_detect_slope" , -10);
            user_configs->add_param("nominal_detect_slope" , 10);
            user_configs->add_param("rise_detect_slope" , 40);
            user_configs->add_param("high_rise_rate" , 0.003);
            user_configs->add_param("mid_rise_rate" ,  0.002);
            user_configs->add_param("low_rise_rate" ,  0.001);
            user_configs->add_param("profit" ,  0.01);
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
        //double long_rsi  = data_long_container->last_rsi;
        //double short_rsi = data_short_container->last_rsi;
        double last_avr_price = last_price.get_average_all_price();
        double slope_short_moving_avarage = data_short_container->last_slope;


        double _deep_detect_slope = user_configs->get_param_value("deep_detect_slope" );
        double _nominal_detect_slope = user_configs->get_param_value("nominal_detect_slope" );
        double _rise_detect_slope = user_configs->get_param_value("rise_detect_slope" );
        double _high_rise_rate = user_configs->get_param_value("high_rise_rate" );
        double _mid_rise_rate = user_configs->get_param_value("mid_rise_rate" );
        double _low_rise_rate = user_configs->get_param_value("low_rise_rate" );
        double _profit = user_configs->get_param_value("profit" );


        if(long_moving_avarage>0){

            switch (state)
            {
            case 0:  // first deep
            {
                if(last_avr_price < short_moving_avarage        &&
                    slope_short_moving_avarage < _deep_detect_slope            &&
                    IS_SELLED())
                {
                    log_print("step 0: deep finded -> goto step 1 ");
                    state = 1;
                    ohlc_counter = 0;
                }

                break;
            }

            case 1:
            {
                if(last_avr_price > PERCENT_ADDER(short_moving_avarage , _high_rise_rate)       &&
                    slope_short_moving_avarage > _rise_detect_slope            &&
                    IS_SELLED())
                {
                    log_print("step 1: rise finded -> goto step 2 ");
                    state = 2;
                    ohlc_counter = 0;
                    break;
                }

                if(last_avr_price < PERCENT_SUBTRACTOR(short_moving_avarage , _high_rise_rate)  ){
                    log_print("step 1: low price -> goto step 0 ");
                    state = 0;
                }

                break;
            }

            case 2:
            {
                if(last_avr_price < PERCENT_ADDER(short_moving_avarage , _low_rise_rate)       &&
                    last_avr_price < PERCENT_SUBTRACTOR(short_moving_avarage , _low_rise_rate)  &&
                    slope_short_moving_avarage > _nominal_detect_slope            &&
                    IS_SELLED())
                {
                    log_print("step 2: price was near to MA -> goto step 3 ");
                    state = 3;
                    ohlc_counter = 0;
                    break;
                }

                if(last_avr_price < PERCENT_SUBTRACTOR(short_moving_avarage , _high_rise_rate)  ){
                    log_print("step 2: low price -> goto step 0 ");
                    state = 0;
                }

                break;
            }


            case 3:
            {
                if(last_avr_price > PERCENT_ADDER(short_moving_avarage , _mid_rise_rate)       &&
                    slope_short_moving_avarage > _rise_detect_slope            &&
                    IS_SELLED())
                {
                    log_print("step 3: second rise finded , BUY() -> goto step 4 ");
                    state = 4;
                    ohlc_counter = 0;
                    return BUY();
                }

                if(last_avr_price < PERCENT_SUBTRACTOR(short_moving_avarage , _high_rise_rate)  ){
                    log_print("step 3: low price -> goto step 0 ");
                    state = 0;
                }
                break;
            }

            case 4:
            {
                if(last_avr_price < long_moving_avarage       &&
                    IS_BUYED())
                {
                    log_print("step 4: price down to MA 100 , SELL()  -> goto step 0 ");
                    state = 0;
                    ohlc_counter = 0;
                    return SELL();
                }

                if(last_avr_price > PERCENT_ADDER(save_purchased_price.get_average_all_price() , _profit)  ){
                    log_print("step 4: high profit detected XXX -> goto step 5 ");
                    state = 5;
                    ohlc_counter = 0;
                }

                break;
            }

            case 5:
            {
                if(last_avr_price < long_moving_avarage       &&
                    IS_BUYED())
                {
                    log_print("step 5: price down to MA 100 , SELL()  -> goto step 0 ");
                    state = 0;
                    ohlc_counter = 0;
                    return SELL();
                }

                if( !last_price.is_green()  ){ // todo 2 red ?
                    log_print("step 5: high profit plus red candle , SELL()  -> goto step 0 ");
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





#endif
