#ifndef AAXE_MID_STRATEGY_H
#define AAXE_MID_STRATEGY_H

#include <QVector>
#include <QDebug>

#include "../trade_strategy_interface.h"

/**

 Strateji açıklaması:
  ( MA_12 < MA_48 ) && ( RSI_160 > 52 )  -> SHORT
  ( MA_12 > MA_48 ) && ( RSI_160 < 48 )  -> LONG

  - %1 ZARAR KES
  - %4 KAR AL

--------------------------------------------------------

// Long koşulu (Alım)
if (current.rsi14 > 70 && (current.high - current.ema20) > (current.atr14 * 2)) {
    std::cout << "Buy Signal at index " << i << " - Price: " << current.close << std::endl;
    double stopLoss = current.close - (current.atr14 * 4);
    std::cout << "Stop Loss set at: " << stopLoss << std::endl;
}

// Short koşulu (Satım)
if (current.rsi14 < 30 && (current.low - current.ema20) < (current.atr14 * -2)) {
    std::cout << "Sell Signal at index " << i << " - Price: " << current.close << std::endl;
    double stopLoss = current.close + (current.atr14 * 4);
    std::cout << "Stop Loss set at: " << stopLoss << std::endl;
}

// Çıkış koşulu
if (current.close > current.ema20) {
    std::cout << "Exit Signal at index " << i << " - Price: " << current.close << std::endl;
}

***/

class aaxe_mid_st_c : public trade_strategy_interface{

public:
    aaxe_mid_st_c(std::size_t _short_period = 26,
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
        data_special_container = new OHLC_container(160*4);

        is_buy_order_set_before = false;
        is_short_order_set_before = false;

        if(user_configs){
            //default values
            user_configs->add_param("max_dif_long_ma_short_ma" , 0.027);  // LONG_MA ve SHORT_MA arasındaki fark düşükken long fırsatı, yüksekken short fırsatı doğurur
            user_configs->add_param("take_max_profit_factor"  , 0.015);   // KAR al, yüksek değer olmalı
            user_configs->add_param("dangerous_ohlc_size" , 0.64);        // 3 arka arka yakılan sinyal mumlarının min uzunluğu
            user_configs->add_param("sell_trig_percent" , 0.0095);        // KAR veya ZARAR satışı yapılır SHORT_MA bu faktör çarpanına bağlı aşağı veya yukarı kalırsa diye
            user_configs->add_param("take_loss_percent" , 0.0055);        // KARAR KES
            user_configs->add_param("max_ohlc_signal_period" , 10.0);     // LONG veya SHORT sinyali analizi için gerekli analiz edilecek son mumların sayısı

            user_configs->add_param("rsi_max" , 68.5);
            user_configs->add_param("rsi_min" , 31.5);
            user_configs->add_param("m_rsi_maxx" , 52.4);
            user_configs->add_param("m_rsi_minx" , 47.6);       // (rsi_max - rsi_sell_trig_range) : satış sinyali , (rsi_min + rsi_sell_trig_range) alış sinyali
            // ...
        }
        else{
            user_configs = new strategy_params();
            user_configs->add_param("max_dif_long_ma_short_ma" , 0.027);  // LONG_MA ve SHORT_MA arasındaki fark düşükken long fırsatı, yüksekken short fırsatı doğurur
            user_configs->add_param("take_max_profit_factor"  , 0.015);   // KAR al, yüksek değer olmalı
            user_configs->add_param("dangerous_ohlc_size" , 0.64);        // 3 arka arka yakılan sinyal mumlarının min uzunluğu
            user_configs->add_param("sell_trig_percent" , 0.0055);        // KAR veya ZARAR satışı yapılır SHORT_MA bu faktör çarpanına bağlı aşağı veya yukarı kalırsa diye
            user_configs->add_param("take_loss_percent" , 0.0195);        // KARAR KES
            user_configs->add_param("max_ohlc_signal_period" , 10.0);     // LONG veya SHORT sinyali analizi için gerekli analiz edilecek son mumların sayısı

            user_configs->add_param("rsi_max" , 65.0);
            user_configs->add_param("rsi_min" , 35.0);
            user_configs->add_param("m_rsi_maxx" , 55.0);
            user_configs->add_param("m_rsi_minx" , 48.0);
        }
    }

    enum class state_e {
        WAIT_NOMINATED_SIGNAL,
        WAIT_POSITION_DETECT_SIGNAL,
        WAIT_JUST_LONG_SIGNAL,
        WAIT_JUST_SHORT_SIGNAL,
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

            _record_critical_rsi_prices();

            switch (state)
            {
                /*
            case state_e::WAIT_NOMINATED_SIGNAL:
            {
                ohlc_counter_long = 0;
                ohlc_counter_short = 0;

                bool signal = _is_market_nominated();

                if(signal == true && IS_SELLED()){
                    GOTO_STEP(state_e::WAIT_POSITION_DETECT_SIGNAL);
                    // return trade_strategy_order_e::PROCESSING_MARK_1;
                }

                break;
            }*/

            case state_e::WAIT_NOMINATED_SIGNAL:
            case state_e::WAIT_POSITION_DETECT_SIGNAL:
            {
                /*
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
*/
                break;
            }

            case state_e::WAIT_JUST_LONG_SIGNAL:
            {
                bool signal = _find_long_buy_signal();

                ohlc_counter_long++;

                if(ohlc_counter_long>20)
                {
                    ohlc_counter_long = 0;
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    break;
                }

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

            case state_e::WAIT_JUST_SHORT_SIGNAL:
            {
                bool signal = _find_short_buy_signal();

                ohlc_counter_short++;

                if(ohlc_counter_short>20)
                {
                    ohlc_counter_short = 0;
                    GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    break;
                }

                if(signal == true && IS_SELLED()){
                    if(last_price.close > long_moving_avarage){
                        GOTO_STEP(state_e::WAIT_HIGH_SHORT_SELL_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NORMAL_SHORT_SELL_SIGNAL);
                    }
                    return BUY_SHORT();
                }

                break;
            }


            case state_e::WAIT_NORMAL_SHORT_SELL_SIGNAL:
            {
                if(_find_short_sell_signal(false) && IS_BUY_SHORT_DONE()){
                    if(_short_sell_check_is_suitable_to_just_long_position())
                    {
                        GOTO_STEP(state_e::WAIT_JUST_LONG_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    }
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_NORMAL_LONG_SELL_SIGNAL:
            {
                if(_find_long_sell_signal(false) && IS_BUY_LONG_DONE()){

                    if(_long_sell_check_is_suitable_to_just_short_position())
                    {
                        GOTO_STEP(state_e::WAIT_JUST_SHORT_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    }
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_HIGH_SHORT_SELL_SIGNAL:
            {
                if(_find_short_sell_signal(true) && IS_BUY_SHORT_DONE()){

                    if(_short_sell_check_is_suitable_to_just_long_position())
                    {
                        GOTO_STEP(state_e::WAIT_JUST_LONG_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    }
                    return SELL();
                }
                break;
            }

            case state_e::WAIT_LOW_LONG_SELL_SIGNAL:
            {
                if(_find_long_sell_signal(true) && IS_BUY_LONG_DONE()){

                    if(_long_sell_check_is_suitable_to_just_short_position())
                    {
                        GOTO_STEP(state_e::WAIT_JUST_SHORT_SIGNAL);
                    }
                    else{
                        GOTO_STEP(state_e::WAIT_NOMINATED_SIGNAL);
                    }
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



        /****


*****/
        if(data_special_container->last_moving_avarage > 0)
        {
            const double rsi12_min = user_configs->get_param_value("rsi_min") ;
            const double rsi12_max = user_configs->get_param_value("rsi_max") ;

            const double rsi160_min = user_configs->get_param_value("m_rsi_minx") ;
            const double rsi160_max = user_configs->get_param_value("m_rsi_maxx") ;

            const double rsi12 = data_special_container->calculate_rsi(16); // 12
            const double rsi160 = data_special_container->calculate_rsi(160*2); // 160

            static bool m_flag_for_buy = false;
            static bool m_flag_for_short = false;

            if( IS_SELLED())
            {
                if( rsi160 > rsi160_max )
                {
                    if( rsi12 < rsi12_min )
                    {
                        m_flag_for_buy = true;
                    }
                    else if( m_flag_for_buy  == true &&  rsi12 > rsi12_min )
                    {
                        m_flag_for_buy = false;
                        return BUY_LONG();
                    }
                }
                else
                {
                    m_flag_for_buy = false;
                }


                if( rsi160 < rsi160_min )
                {
                    if( rsi12 > rsi12_max )
                    {
                        m_flag_for_short = true;
                    }
                    else if( m_flag_for_short  == true &&  rsi12 < rsi12_max )
                    {
                        m_flag_for_short = false;
                        return BUY_SHORT();
                    }
                }
                else
                {
                    m_flag_for_short = false;
                }
            }
            else if(IS_BUY_LONG_DONE()){

                if( rsi12 > rsi12_max )
                {
                    return SELL();
                }

                // PROTECTİON
                if( rsi160 < rsi160_min )
                {
                    return SELL();
                }

                // zarar kes koruma !!
                const double purchased_price = save_purchased_price.get_average_all_price();
                const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;

                if (last_price.close < PERCENT_SUBTRACTOR(purchased_price, _TAKE_LOSS_FACTOR))
                {
                    return SELL();
                }
            }
            else if(IS_BUY_SHORT_DONE()){

                if( rsi12 < rsi12_min )
                {
                    return SELL();
                }

                // PROTECTİON
                if( rsi160 > rsi160_max )
                {
                    return SELL();
                }


                // zarar kes koruma !!
                const double purchased_price = save_purchased_price.get_average_all_price();
                const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;

                if (last_price.close > PERCENT_ADDER(purchased_price,_TAKE_LOSS_FACTOR))
                {
                    return SELL();
                }

            }

        }


        return trade_strategy_order_e::PROCESSING;
    }

private:

    bool _long_sell_check_is_suitable_to_just_short_position(){

        return false;
    }


    bool _short_sell_check_is_suitable_to_just_long_position(){

        return false;
    }

    int rsi_low_signal_for_long_buy = 0;
    int rsi_high_signal_for_short_buy = 0;

    bool overload_price_detected_for_short = false;
    bool underload_price_detected_for_long = false;


    double last_price_for_rsi_max = 0;
    double last_price_for_rsi_min = 0;


    void _record_critical_rsi_prices() {
        static int protection_1 = 0;
        static int protection_2 = 0;

        const double rsi_max = user_configs->get_param_value("rsi_max");
        const double rsi_min = user_configs->get_param_value("rsi_min");

        protection_1++;
        protection_2++;

        if( data_long_container->last_rsi > rsi_max ){
            protection_1 = 0;
            last_price_for_rsi_max = last_price.close;
        }

        if( data_long_container->last_rsi < rsi_min ){
            protection_2 = 0;
            last_price_for_rsi_min = last_price.close;
        }

        if(protection_1 > 200){
            protection_1 = 0;
            last_price_for_rsi_max = 0;
        }

        if(protection_2 > 200){
            protection_2 = 0;
            last_price_for_rsi_min = 0;
        }

    }

    /************/

    bool _find_short_buy_signal() {

        const double long_moving_avarage = data_long_container->last_moving_avarage;

        // önceden rsi fazla şişti ve o kaydettiğimiz sinyale yakın değerde sinyal yaktı
        if(last_price_for_rsi_max > 0 &&
            last_price.close < long_moving_avarage &&
            !last_price.is_green() &&
            //last_price.close < PERCENT_ADDER(last_price_for_rsi_max,0.013) &&
            last_price.close > PERCENT_SUBTRACTOR(last_price_for_rsi_max,0.015)){  // %1.3

            return true;
        }

        return false;
    }

    bool _find_short_sell_signal(bool select_is_high) {

        //const double short_moving_avarage = data_short_container->last_moving_avarage;
        //const double long_moving_avarage = data_long_container->last_moving_avarage;
        // const double _MA_DIF_FACTOR = user_configs->get_param_value("sell_trig_percent") ;
        //const double rsi_sell_trig_range = user_configs->get_param_value("rsi_sell_trig_range") ;

        double rsi_min = user_configs->get_param_value("rsi_min");
        const double long_moving_avarage = data_long_container->last_moving_avarage;

        static bool sell_rsi_signal = false;

        if( data_long_container->last_rsi < rsi_min ){
            sell_rsi_signal = true;
        }

        if(sell_rsi_signal &&
            last_price.close > long_moving_avarage &&
            last_price.is_green()){
            sell_rsi_signal = false;
            return true;
        }


        // zarar kes koruma !!
        const double purchased_price = save_purchased_price.get_average_all_price();
        const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;

        if (last_price.close > PERCENT_ADDER(purchased_price,_TAKE_LOSS_FACTOR))
        {
            sell_rsi_signal = false;
            return true;
        }

        return false;
    }

    /**************/

    bool _find_long_buy_signal() {

        const double long_moving_avarage = data_long_container->last_moving_avarage;
        //const double short_moving_avarage = data_short_container->last_moving_avarage;
        const double rsi_min = user_configs->get_param_value("rsi_min") ;

        if(data_long_container->last_rsi < rsi_min ){
            rsi_low_signal_for_long_buy++;
        }

        // rsi en dipten yukarı doğru çıkmaya başlarken al
        if(rsi_low_signal_for_long_buy > 0 &&
            data_long_container->last_rsi > 43 &&
            data_long_container->last_rsi < 61 &&
            last_price.close > long_moving_avarage )  // short_moving_avarage
        {
            rsi_low_signal_for_long_buy = 0;
            return true;
        }

        if(data_long_container->last_rsi > 55 ){
            rsi_low_signal_for_long_buy = 0;
        }

        return false;
    }

    bool _find_long_sell_signal(bool select_is_low) {

        const double long_moving_avarage = data_long_container->last_moving_avarage;

        double rsi_max = user_configs->get_param_value("rsi_max")  ;

        static bool sell_rsi_signal = false;

        if( data_long_container->last_rsi > rsi_max ){
            sell_rsi_signal = true;
        }


        // Kar al, ilk kırmızı mumda !
        if(sell_rsi_signal &&
            last_price.close < long_moving_avarage &&
            !last_price.is_green()){
            sell_rsi_signal = false;
            return true;
        }


        // zarar kes koruma !!
        const double purchased_price = save_purchased_price.get_average_all_price();
        const double _TAKE_LOSS_FACTOR = user_configs->get_param_value("take_loss_percent") ;

        if (last_price.close < PERCENT_SUBTRACTOR(purchased_price, _TAKE_LOSS_FACTOR))
        {
            sell_rsi_signal = false;
            return true;
        }

        return false;
    }


    bool _is_market_nominated() {

        return true;
    }


public:

    void feed_ohlc(const OHLC_s& ohlc_seed) override{

        data_short_container->add(ohlc_seed);

        data_long_container->add(ohlc_seed);

        data_special_container->add(ohlc_seed);

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

        rsi_low_signal_for_long_buy = 0;
        rsi_high_signal_for_short_buy = 0;
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

    OHLC_container  *data_special_container;

    account_interface * account;
    strategy_params* user_configs ;  // all configs by connected GUI
    bool is_buy_order_set_before;
    bool is_short_order_set_before;
};





#endif
