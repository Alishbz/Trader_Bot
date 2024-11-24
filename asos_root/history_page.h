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
#ifndef HISTORY_PAGE_H
#define HISTORY_PAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QScrollBar>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QMessageAuthenticationCode>
#include <QCheckBox>

//@strategyies
#include "strategies/trade_strategy_interface.h"
#include "strategies/ex/simple_mx_strategy.h"
#include "strategies/ex/asos_high_risk_strategy.h"
#include "strategies/ex/asos_low_risk_strategy.h"
#include "strategies/ex/chatgpt_strategy.h"
#include "strategies/ex/chatgpt2_strategy.h"
#include "strategies/ex/fido_strategy.h"
#include "strategies/ex/simple_mx_pro_strategy.h"
#include "strategies/ex/fire_strategy.h"
#include "strategies/ex/fax_strategy.h"
#include "strategies/ex/fidorsi_strategy.h"
#include "strategies/ex/macd_strategy.h"
#include "strategies/ex/fxal_strategy.h"
#include "strategies/ex/sf_strategy.h"
#include "strategies/ex/aaxe_long_st.h"
#include "strategies/ex/aaxe_mid_st.h"
#include "strategies/ex/aaxe_short_st.h"

#include "charts/candle_graph.h"

#include "account/account_interface.h"
#include "connections/binance_rest_api.h"
#include "connections/binance_web_socket.h"

#include "generals/ohlc.h"

//#include "custom_widgets/as_date_selector.h"


// some fast changable configs
#define HISTORY_MA_INDEX_DEF       26
#define HISTORY_MA_INDEX_DEF_MAX   160

/***
@about HISTORY_START_KLINE_TEXTBOX and HISTORY_STOP_KLINE_TEXTBOX

"2024-07-10T09:00:00"       to        "2024-07-11T16:00:00"

bu zaman aralığında bitcoin kocaman bir aşağı kova yapmış geri aynı yerine gelmiş , burda para kazanan heryerde kazanır

düşüş:    "2024-07-10T09:00:00"       to        "2024-07-10T16:00:00"
yükseliş: "2024-07-11T09:00:00"       to        "2024-07-11T16:00:00"

*/

// "2023-12-15T09:00:00" todo bu gün de rsi saat 22.34 de hatalı ?
//#define HISTORY_START_KLINE_TEXTBOX  "2023-12-15T09:00:00" //"2024-07-11T09:00:00"
//#define HISTORY_STOP_KLINE_TEXTBOX   "2023-12-16T09:00:00" //"2024-07-11T16:00:00"

#define HISTORY_START_KLINE_TEXTBOX  "2024-03-01T09:00:00" //"2024-04-20T09:00:00" //"2022-07-01T09:00:00"
#define HISTORY_STOP_KLINE_TEXTBOX   "2024-05-02T09:00:00" //"2024-04-21T09:00:00" //"2022-07-02T09:00:00"


#define HISTORY_NEXT_KLINE_TIME      60*48//(1440)//(420)

#define HISTORY_GET_MAX_DATA_TO_REST_API   100

#define HISTORY_CANDLE_TIME_SELECT    kline_select_e::_5_minute

#define HISTORY_CANDLE_SHOW_MAX_DATA       300//400

#define HISTORY_CONFIG_WIDGETS_MAX_AREA  10




class history_page_c : public QWidget ,public account_interface {
    Q_OBJECT

private:
    QTimer *timer = nullptr;
public:
    history_page_c(QWidget*                 parent = nullptr ,
                   coin_select_e            e = coin_select_e::BTC,
                   binance_web_socket_c*    web_socket = nullptr,  // todo history web socket hiç kullanmıyor? gerek var mı ?
                   binance_rest_api_c*      rest_api = nullptr):
        QWidget(parent),
        _web_socket(web_socket),
        _rest_api(rest_api)
    {
        QVBoxLayout* main_layout = new QVBoxLayout(this);

        _selected_coin = e;

        _params_init_create();

        _load_design_page(main_layout);

        setLayout(main_layout);

        _trader = new trader_c(this);
        connect(_trader, &trader_c::log_print, this, &history_page_c::log_print);

        if(_rest_api){
            QObject::connect(_rest_api, &binance_rest_api_c::kline_coin_price_update,
                             this, &history_page_c::kline_coin_price_update  );
        }

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &history_page_c::one_second_timer_callback);
        timer->start(400);
    }

    void web_socket_set(binance_web_socket_c* s)  {
        _web_socket = s;
    }

    void rest_api_set(binance_rest_api_c* s)  {
        _rest_api = s;
        if(_rest_api){
            QObject::connect(_rest_api, &binance_rest_api_c::kline_coin_price_update,
                             this, &history_page_c::kline_coin_price_update  );
        }
    }

    /*** Interface  *********************************************************/

    void select_coin(coin_select_e s) override{
        _selected_coin = s;
    }

    void buy_order()override{
        direct_buy_order();
    }

    bool is_buy_done()override{
        return _is_buy_enable;
    }


    void short_order()override{
        direct_short_order();
    }

    bool is_short_done()override{
        return _is_short_enable;
    }


    bool is_sell_done()override{

        return !_is_buy_enable;

    }

    void sell_order()override{
        direct_sell_order();
    }

    double get_wallet()override{
        return read_wallet_text();
    }

private slots:

    void one_second_timer_callback() {

        check_is_there_kline_request_and_get();
    }

    void kline_coin_price_update(const OHLC_s& data , qint64 timestamp, double price_quantity_volume){

        _is_get_kline_request_active = false;

        wallet_update(data.get_average_mid_point());

        _trader->async_data_update(data);

        _trader->async_price_quantity_volume_update(price_quantity_volume);

        update_containers(data);

        trade_strategy_order_e st = _trader->execute_strategy();

        if(_chart_show_state){

            _chart->OHLC_update(data , timestamp);

            if(st == trade_strategy_order_e::BUY){
                _chart->point_chart("BUY",QColor(162, 255, 151));
            }
            else if(st == trade_strategy_order_e::SELL){
                _chart->point_chart("SELL",QColor(255, 50, 200));
            }
            else if(st == trade_strategy_order_e::SHORT){
                _chart->point_chart("SHORT",QColor(255, 255, 150));
            }
            else if(st == trade_strategy_order_e::PROCESSING_MARK_1){
                _chart->point_chart("MARK_1",QColor(135, 206, 235));
            }
            else if(st == trade_strategy_order_e::PROCESSING_MARK_2){
                _chart->point_chart("MARK_2",QColor(50, 50, 50));
            }

            if(_RSI_MA_container->last_moving_avarage > 0)
                _chart->line_update(candle_line_select_e::LINE_1 ,_RSI_MA_container->calculate_moving_average() , timestamp);

            if(_high_RSI_MA_container->last_moving_avarage > 0)
                _chart->line_update(candle_line_select_e::LINE_2 ,_high_RSI_MA_container->last_moving_avarage , timestamp);



            if(_RSI_MA_container->last_rsi > 0)
                _rsi_chart->line_update(candle_line_select_e::LINE_1 ,_RSI_MA_container->last_rsi , timestamp);

            if(_high_RSI_MA_container->last_rsi > 0)
                _rsi_chart->line_update(candle_line_select_e::LINE_2 ,_high_RSI_MA_container->last_rsi , timestamp);


        }
    }


signals:
    void log_print(const QString &message);



private:  // functions

    void update_containers(const OHLC_s& ohlc_seed) {
        _RSI_MA_container->add(ohlc_seed);
        _high_RSI_MA_container->add(ohlc_seed);
    }


    void wallet_update(double data){
        _last_coin_price = data;
        if(  _is_buy_enable){
            double new_wallet_price =   _record_wallet_for_buy * _last_coin_price /  _record_coin_price_for_buy; // @475
            set_wallet_text(new_wallet_price);
        }
        else if (_is_short_enable) {
           // double new_wallet_price = _record_wallet_for_short * (_record_coin_price_for_short / _last_coin_price); // TODO BU HESAP YANLIS

            double new_wallet_price = _record_wallet_for_short;

           if((_record_coin_price_for_short / _last_coin_price) > 1 ){
                new_wallet_price += new_wallet_price*  (1-( _last_coin_price / _record_coin_price_for_short ));
            }
           else{
                 new_wallet_price -= new_wallet_price*  (1-( _record_coin_price_for_short / _last_coin_price ));
           }

            set_wallet_text(new_wallet_price);
        }
    }


    void deduct_commision(){
        if(FAKE_COMMISSION_RATE>0){
            double hold_wallet = read_wallet_text();
            hold_wallet -= (hold_wallet *FAKE_COMMISSION_RATE);
            set_wallet_text(hold_wallet);
        }
    }

    void direct_buy_order(){
        if( _is_buy_enable == false){
            emit log_print(" HISTORY_PAGE: buy: " + QString::number( _last_coin_price));
            _is_buy_enable = true;
            _is_short_enable = false;
            deduct_commision();
            _record_coin_price_for_buy =  _last_coin_price;
            _record_wallet_for_buy = read_wallet_text();
        }
    }


    void direct_short_order(){
        if( _is_short_enable == false){
            emit log_print(" HISTORY_PAGE: short: " + QString::number( _last_coin_price));
            _is_short_enable = true;
            _is_buy_enable = false;
            deduct_commision();
            _record_coin_price_for_short =  _last_coin_price;
            _record_wallet_for_short = read_wallet_text();
        }
    }

    void direct_sell_order(){
        if( _is_buy_enable){
            emit log_print(" HISTORY_PAGE: LONG sell: " + QString::number( _last_coin_price));
            _is_buy_enable = false;
            _is_short_enable = false;
            deduct_commision();
            return;
        }

        if( _is_short_enable){
            emit log_print(" HISTORY_PAGE: SHORT sell: " + QString::number( _last_coin_price));
            _is_buy_enable = false;
            _is_short_enable = false;
            deduct_commision();
        }
    }


    /**************/

    void set_wallet_text(double value){
        _wallet_textbox->setText(QString::number(value, 'f', 4));
    }

    double read_wallet_text( ){
        bool ok;
        double value = _wallet_textbox->text().toDouble(&ok);
        if (!ok) {
            qWarning("HISTORY_PAGE: Conversion to double failed. Invalid input.xx");
            return 0.0;
        }
        return value;
    }

    void receive_data_blinky(){
        if (blink_label->text().isEmpty()) {
            blink_label->setText("data receive ");
        } else {
            blink_label->setText("");
        }
    }

    void check_is_there_kline_request_and_get(){  // NOT 200 dakika sorgu sorulup graphın yenilenmesi vs bile 6 7 saniye kesin sürüyor ?

        static int period_changer = 0;
        static int protection_count = 0;

        protection_count++;
        period_changer++;

        if(_is_get_kline_request_active == true){
            period_changer = 0;
            if(protection_count > 20){
                _is_get_kline_request_active = false;
                _kline_stop_time = 0;
                _kline_start_time = 0;
                emit log_print("HISTORY_PAGE: kline request timeout ERROR");
            }
            return;
        }

        protection_count = 0;

        if(period_changer > 0  &&
            _kline_stop_time != _kline_start_time &&
            _is_get_kline_request_active == false){

            period_changer = 0;

            if(_kline_start_time > 0 && _kline_stop_time > 0 && _kline_stop_time > _kline_start_time){
                qint64 dif = _kline_stop_time - _kline_start_time;
                qint64 max_request_interval = kline_select_to_milisecond(selected_kline_type) * HISTORY_GET_MAX_DATA_TO_REST_API;

                receive_data_blinky();

                if(dif > max_request_interval )
                {
                    _is_get_kline_request_active = true;

                    _rest_api->get_klines(
                        _selected_coin,
                        selected_kline_type,
                        _kline_start_time,
                        _kline_start_time + max_request_interval
                        );

                    _kline_start_time += max_request_interval;
                }
                else if(dif>0 && _kline_stop_time > _kline_start_time){

                    _is_get_kline_request_active = true;

                    _rest_api->get_klines(
                        _selected_coin,
                        selected_kline_type,
                        _kline_start_time,
                        _kline_stop_time
                        );

                    _kline_stop_time = 0;
                    _kline_start_time = 0;
                }
            }
        }
    }

    void update_strategy_config_container(){
        for (int i = 0; i < HISTORY_CONFIG_WIDGETS_MAX_AREA; ++i) {
            QString text = _strategy_user_textboxes[i]->text();
            bool ok;
            double value = text.toDouble(&ok);
            if (ok) {
                _strategy_user_configs.add_by_index(i, value);
            }
        }
    }

    void update_strategy_config_widgets() {
        for (int i = 0; i < HISTORY_CONFIG_WIDGETS_MAX_AREA; ++i) {
            QString paramName = _strategy_user_configs.get_param_name(i);
            if(!paramName.isEmpty()){
                _strategy_user_labels[i]->setText(paramName);
                double paramValue = _strategy_user_configs.get_param_value(paramName);
                _strategy_user_textboxes[i]->setText(QString::number(paramValue, 'f', 4));
            }
        }
    }

private:  // ex datas

    bool _is_get_kline_request_active;
    bool _is_buy_enable ;
    bool _is_short_enable ;
    double _record_coin_price_for_buy ;
    double _record_wallet_for_buy ;
    double _record_coin_price_for_short  ;
    double _record_wallet_for_short ;

    double _last_coin_price ;
    bool _chart_show_state;
    int _generic_period = HISTORY_MA_INDEX_DEF;
    int _high_period = HISTORY_MA_INDEX_DEF_MAX;

    kline_select_e selected_kline_type = HISTORY_CANDLE_TIME_SELECT;

    qint64 _kline_start_time ;
    qint64 _kline_stop_time  ;

    // comms ...
    binance_web_socket_c*    _web_socket= nullptr;
    binance_rest_api_c*      _rest_api= nullptr;
    strategy_params          _strategy_user_configs ;

    // containers ...
    OHLC_core_creator        _1_sc_ohlc_creator;

    OHLC_container*          _RSI_MA_container = nullptr;
    OHLC_container*          _high_RSI_MA_container = nullptr;

    coin_select_e _selected_coin;

    trader_c* _trader= nullptr;
    simple_mx_strategy_c * _strategy_smx= nullptr;
    asos_high_risk_strategy_c * _asos_high_risk_st = nullptr;
    asos_low_risk_strategy_c * _asos_low_risk_st = nullptr;
    chatgpt_strategy_c* _chatgpt_st = nullptr;
    chatgpt2_strategy_c* _chatgpt2_st = nullptr;
    fido_strategy_c*_fido_st = nullptr;
    simple_mx_pro_strategy_c * _strategy_smx_pro= nullptr;
    fire_strategy_c * _fire_st= nullptr;
    fax_strategy_c * _fax_st= nullptr;
    fidorsi_strategy_c* _fidorsi_st = nullptr;
    macd_strategy_c* _macd_st = nullptr;
    fxal_strategy_c* _fxal_st = nullptr;
    sf_strategy_c* _sf_st = nullptr;
    aaxe_long_st_c* _aaxe_long_st = nullptr;
    aaxe_short_st_c* _aaxe_short_st = nullptr;
    aaxe_mid_st_c* _aaxe_mid_st = nullptr;

    void _params_init_create(){

        selected_kline_type = HISTORY_CANDLE_TIME_SELECT;
        _generic_period = HISTORY_MA_INDEX_DEF;
        _high_period = HISTORY_MA_INDEX_DEF_MAX;
        _is_get_kline_request_active = false;
        _is_buy_enable = false;
        _is_short_enable = false;
        _record_coin_price_for_buy = 0;
        _record_wallet_for_buy = 0;
        _last_coin_price = 0;
        _kline_start_time=0;
        _kline_stop_time = 0;
        _chart_show_state = true;
        _record_coin_price_for_short = 0 ;
        _record_wallet_for_short = 0 ;
        _strategy_smx = nullptr;
        _strategy_smx_pro = nullptr;
        _asos_high_risk_st = nullptr;
        _asos_low_risk_st = nullptr;

        // create containers
        _RSI_MA_container = new OHLC_container(HISTORY_MA_INDEX_DEF);
        _high_RSI_MA_container = new OHLC_container(HISTORY_MA_INDEX_DEF_MAX);

    }

private:  // widget datas

    candle_graph_c *_chart = nullptr;
    candle_graph_c *_rsi_chart = nullptr;

    QLineEdit *_wallet_textbox = nullptr;
    QLineEdit *_kline_start_time_textbox = nullptr;
    QLineEdit *_kline_stop_time_textbox = nullptr;
    QComboBox *_strategy_select_combo_box = nullptr;
    QComboBox *_coin_select_combo_box= nullptr;
    QComboBox *_kline_select_combo_box= nullptr;
    QLineEdit *_periot_textbox = nullptr;
    QLineEdit *_high_periot_textbox = nullptr;
    QLineEdit *_kline_next_time_label_textbox = nullptr;
    QLineEdit *_process_timer_textbox = nullptr;
    QLabel *blink_label = nullptr;

    QLabel * _strategy_user_labels[HISTORY_CONFIG_WIDGETS_MAX_AREA] ;
    QLineEdit * _strategy_user_textboxes[HISTORY_CONFIG_WIDGETS_MAX_AREA];

    void _load_design_page(QVBoxLayout* history_layout){

        std::array<QString, static_cast<size_t>(candle_line_select_e::MAX_SIZE)>
            _history_candle_line_names = {
                "MA_1", "MA_2", "MA_3"
            };

        std::array<QString, static_cast<size_t>(candle_line_select_e::MAX_SIZE)>
            _history_rsi_line_names = {
                "rsi_1", "rsi_2", "rsi_3"
            };

        _chart = new candle_graph_c(this," history test " , _history_candle_line_names,HISTORY_CANDLE_SHOW_MAX_DATA);
        _rsi_chart = new candle_graph_c(this," history rsi " , _history_rsi_line_names,HISTORY_CANDLE_SHOW_MAX_DATA);

        QHBoxLayout *strategy_selection_layout = new QHBoxLayout(this);
        {
            blink_label = new QLabel("data receive", this) ;
            blink_label->setStyleSheet("QLabel { color : red; }");

            QLabel* combobox_description = new QLabel("select strategy :", this) ;
            _strategy_select_combo_box = new QComboBox(this);

            _strategy_select_combo_box->addItem("_aaxe_mid_st strategy");
            _strategy_select_combo_box->addItem("_aaxe_short_st strategy");
            _strategy_select_combo_box->addItem("_aaxe_long_st strategy");

            _strategy_select_combo_box->addItem("sf strategy");
            _strategy_select_combo_box->addItem("fxal strategy");
            _strategy_select_combo_box->addItem("simple mx strategy");
            _strategy_select_combo_box->addItem("simple mx pro strategy");
            _strategy_select_combo_box->addItem("asos low risk strategy");
            _strategy_select_combo_box->addItem("asos high risk strategy");
            _strategy_select_combo_box->addItem("chatgpt strategy");
            _strategy_select_combo_box->addItem("chatgpt2 strategy");
            _strategy_select_combo_box->addItem("fido strategy");
            _strategy_select_combo_box->addItem("fire strategy");
            _strategy_select_combo_box->addItem("fax strategy");
            _strategy_select_combo_box->addItem("fidorsi strategy");
            _strategy_select_combo_box->addItem("macd strategy");

            QPushButton *start_strategy_button = new QPushButton("Start Strategy", this);
            QPushButton *stop_strategy_button = new QPushButton("Stop Strategy", this);
            connect(start_strategy_button, &QPushButton::clicked,
                    this, [this]() {
                        QString strategyName = _strategy_select_combo_box->currentText();

                        if (strategyName == "simple mx strategy") {
                            emit log_print("simple mx strategy CREATING & STARING");
                            if(_strategy_smx)
                                delete _strategy_smx;
                            _strategy_smx = new simple_mx_strategy_c(_generic_period , this);
                            _trader->set_strategy(_strategy_smx);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "_aaxe_mid_st strategy") {
                            emit log_print("_aaxe_mid_st strategy CREATING & STARING");
                            if(_aaxe_mid_st)
                                delete _aaxe_mid_st;
                            _aaxe_mid_st = new aaxe_mid_st_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_aaxe_mid_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "_aaxe_long_st strategy") {
                            emit log_print("_aaxe_long_st strategy CREATING & STARING");
                            if(_aaxe_long_st)
                                delete _aaxe_long_st;
                            _aaxe_long_st = new aaxe_long_st_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_aaxe_long_st);
                            update_strategy_config_widgets();
                        }


                        else if (strategyName == "_aaxe_short_st strategy") {
                            emit log_print("_aaxe_short_st strategy CREATING & STARING");
                            if(_aaxe_short_st)
                                delete _aaxe_short_st;
                            _aaxe_short_st = new aaxe_short_st_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_aaxe_short_st);
                            update_strategy_config_widgets();
                        }


                        else if (strategyName == "sf strategy") {
                            emit log_print("fxal strategy CREATING & STARING");
                            if(_sf_st)
                                delete _sf_st;
                            _sf_st = new sf_strategy_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_sf_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "fxal strategy") {
                            emit log_print("fxal strategy CREATING & STARING");
                            if(_fxal_st)
                                delete _fxal_st;
                            _fxal_st = new fxal_strategy_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_fxal_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "simple mx pro strategy") {
                            emit log_print("simple mx pro strategy CREATING & STARING");
                            if(_strategy_smx_pro)
                                delete _strategy_smx_pro;
                            _strategy_smx_pro = new simple_mx_pro_strategy_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_strategy_smx_pro);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "fidorsi strategy") {
                            emit log_print(" _fidorsi_st strategy CREATING & STARING");
                            if(_fidorsi_st)
                                delete _fidorsi_st;
                            _fidorsi_st = new fidorsi_strategy_c(_generic_period , _high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_fidorsi_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "macd strategy") {
                            emit log_print("macd strategy CREATING & STARING");
                            if(_macd_st)
                                delete _macd_st;
                            _macd_st = new macd_strategy_c(_generic_period ,_high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_macd_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "fax strategy") {
                            emit log_print(" fax strategy CREATING & STARING");
                            if(_fax_st)
                                delete _fax_st;
                            _fax_st = new fax_strategy_c(_generic_period , _high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_fax_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "asos low risk strategy") {
                            emit log_print("asos low risk strategy CREATING & STARING");
                            if(_asos_low_risk_st)
                                delete _asos_low_risk_st;
                            _asos_low_risk_st = new asos_low_risk_strategy_c(_generic_period , this);
                            _trader->set_strategy(_asos_low_risk_st);
                            update_strategy_config_widgets();
                        }


                        else if (strategyName == "asos high risk strategy") {
                            emit log_print("asos high risk strategy CREATING & STARING");
                            if(_asos_high_risk_st)
                                delete _asos_high_risk_st;
                            _asos_high_risk_st = new asos_high_risk_strategy_c(_generic_period ,_high_period   , this , &_strategy_user_configs);
                            _trader->set_strategy(_asos_high_risk_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "chatgpt strategy") {
                            emit log_print(" chatgpt_strategy CREATING & STARING");
                            if(_chatgpt_st)
                                delete _chatgpt_st;
                            _chatgpt_st = new chatgpt_strategy_c(_generic_period , this);
                            _trader->set_strategy(_chatgpt_st);
                            update_strategy_config_widgets();
                        }
                        else if (strategyName == "chatgpt2 strategy") {
                            emit log_print(" chatgpt_strategy2 CREATING & STARING");
                            if(_chatgpt2_st)
                                delete _chatgpt2_st;
                            _chatgpt2_st = new chatgpt2_strategy_c(_generic_period,_high_period , this , &_strategy_user_configs);
                            _trader->set_strategy(_chatgpt2_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "fido strategy") {
                            emit log_print(" _fido_strategy CREATING & STARING");
                            if(_fido_st)
                                delete _fido_st;
                            _fido_st = new fido_strategy_c(_generic_period , _high_period, this);
                            _trader->set_strategy(_fido_st);
                            update_strategy_config_widgets();
                        }

                        else if (strategyName == "fire strategy") {
                            emit log_print(" fire strategy CREATING & STARING");
                            if(_fire_st)
                                delete _fire_st;
                            _fire_st = new fire_strategy_c(_generic_period , _high_period, this , &_strategy_user_configs);
                            _trader->set_strategy(_fire_st);
                            update_strategy_config_widgets();
                        }

                    });
            connect(stop_strategy_button, &QPushButton::clicked,
                    this, [this]() {
                        emit log_print("selected strategy STOPING");
                        _kline_stop_time = 0;
                        _kline_start_time = 0;
                        sell_order();
                        _trader->set_strategy(nullptr);
                    });
            /** add to layout **/

            strategy_selection_layout->addWidget(blink_label);
            strategy_selection_layout->addWidget(combobox_description);
            strategy_selection_layout->addWidget(_strategy_select_combo_box);
            strategy_selection_layout->addWidget(start_strategy_button);
            strategy_selection_layout->addWidget(stop_strategy_button);
        }

        QHBoxLayout *coin_selection_layout = new QHBoxLayout(this);
        {
            QLabel* coin_combobox_description = new QLabel("select trade coin :", this) ;
            _coin_select_combo_box= new QComboBox(this);
            for(int i = 0 ;i< MAX_COUIN_SUPPORT_COUNT ; i++){
                _coin_select_combo_box->addItem(coin_map_get_enum_to_str((coin_select_e)i));
            }
            QPushButton *select_coin_button = new QPushButton("SET COIN",this);
            connect(select_coin_button, &QPushButton::clicked,
                    this, [this]() {
                        QString coin_str = _coin_select_combo_box->currentText();
                        for(int i = 0 ;i< MAX_COUIN_SUPPORT_COUNT ; i++){
                            if(coin_name_list[i] == coin_str)
                            {
                                coin_select_e _new_coin = (coin_select_e)i;
                                select_coin(_new_coin);
                                _chart->clear_chart();
                                _rsi_chart->clear_chart();
                                _RSI_MA_container->clear();
                                _high_RSI_MA_container->clear();
                                _wallet_textbox->setText("1000");
                                emit log_print("  SET COIN DONE   ");
                            }
                        }
                    });

            /** add to layout **/
            coin_selection_layout->addWidget(coin_combobox_description);
            coin_selection_layout->addWidget(_coin_select_combo_box);
            coin_selection_layout->addWidget(select_coin_button);
        }




        QHBoxLayout *kline_layout = new QHBoxLayout(this);
        {
            QLabel *kline_start_label = new QLabel("Start Kline Data:", this);
            _kline_start_time_textbox = new QLineEdit(HISTORY_START_KLINE_TEXTBOX,this);

            QLabel *kline_stop_label = new QLabel("Stop Kline Data:", this);
            _kline_stop_time_textbox = new QLineEdit(HISTORY_STOP_KLINE_TEXTBOX,this);

            /** add to layout **/
            kline_layout->addWidget(kline_start_label);
            kline_layout->addWidget(_kline_start_time_textbox);
            kline_layout->addWidget(kline_stop_label);
            kline_layout->addWidget(_kline_stop_time_textbox);
        }




        QHBoxLayout *wallet_buy_sell_layout = new QHBoxLayout(this);
        {
            _wallet_textbox = new QLineEdit(this);
            _wallet_textbox->setText("1000");
            QFont font = _wallet_textbox->font();
            font.setPointSize(32);
            _wallet_textbox->setFont(font);
            _wallet_textbox->setPlaceholderText("Enter amount");

            QCheckBox * gr_selection = new QCheckBox("graph show mode: " , this);
            gr_selection->setChecked(true);
            connect(gr_selection, &QCheckBox::stateChanged, this, [this](int state){
                if (state == Qt::Checked) {
                    _chart_show_state = true;
                } else {
                    _chart_show_state = false;
                }
            });

            /** add to layout **/
            wallet_buy_sell_layout->addWidget(new QLabel("History Wallet:", this));
            wallet_buy_sell_layout->addWidget(_wallet_textbox);
            wallet_buy_sell_layout->addWidget(gr_selection);
        }


        QHBoxLayout *c_kline_layout = new QHBoxLayout(this);
        {
            QPushButton *kline_test_button = new QPushButton("GET TEST klines", this);
            connect(kline_test_button, &QPushButton::clicked, this, [this]() {
                _chart->clear_chart();
                _rsi_chart->clear_chart();
                _kline_start_time = _rest_api->convert_string_to_timestamp(_kline_start_time_textbox->text());
                _kline_stop_time = _rest_api->convert_string_to_timestamp(_kline_stop_time_textbox->text());
            });

            QLabel *kline_next_time_label = new QLabel("Next Kline Minute:", this);
            _kline_next_time_label_textbox = new QLineEdit(QString::number(HISTORY_NEXT_KLINE_TIME),this);

            QPushButton *next_kline_test_button = new QPushButton("Next klines", this);
            connect(next_kline_test_button, &QPushButton::clicked, this, [this]() {

                QString text = _kline_next_time_label_textbox->text();
                bool ok;
                double next_minute = text.toDouble(&ok);

                if (ok) {
                    _kline_start_time = _rest_api->convert_string_to_timestamp(_kline_start_time_textbox->text());
                    _kline_stop_time = _rest_api->convert_string_to_timestamp(_kline_stop_time_textbox->text());

                    _kline_start_time_textbox->setText(_rest_api->convert_timestamp_to_string(_kline_start_time + 60 * 1000 *next_minute));
                    _kline_stop_time_textbox->setText(_rest_api->convert_timestamp_to_string(_kline_stop_time + 60 * 1000 *next_minute));

                    emit log_print("  NEXT TİME ");
                }
                else{
                    emit log_print("wrong next kline minute select");
                }
            });



            QPushButton *next_day_button = new QPushButton("Next day", this);
            connect(next_day_button, &QPushButton::clicked, this, [this]() {

                qint64 _start_time = _rest_api->convert_string_to_timestamp(_kline_start_time_textbox->text());
                qint64 _stop_time = _rest_api->convert_string_to_timestamp(_kline_stop_time_textbox->text());

                _kline_start_time_textbox->setText(_rest_api->convert_timestamp_to_string(_start_time + 60 * 1000 *1440));
                _kline_stop_time_textbox->setText(_rest_api->convert_timestamp_to_string(_stop_time + 60 * 1000 *1440));


            });

            QPushButton *next_5_hour_button = new QPushButton("Next 5 Hour", this);
            connect(next_5_hour_button, &QPushButton::clicked, this, [this]() {

                qint64 _start_time = _rest_api->convert_string_to_timestamp(_kline_start_time_textbox->text());
                qint64 _stop_time = _rest_api->convert_string_to_timestamp(_kline_stop_time_textbox->text());

                _kline_start_time_textbox->setText(_rest_api->convert_timestamp_to_string(_start_time + 60 * 1000 *300));
                _kline_stop_time_textbox->setText(_rest_api->convert_timestamp_to_string(_stop_time + 60 * 1000 *300));


            });




            QLabel *process_timer_label = new QLabel("process timer ms: ", this);
            _process_timer_textbox = new QLineEdit(QString::number(1000),this);

            QPushButton *process_timer_button = new QPushButton("Set process timer", this);
            connect(process_timer_button, &QPushButton::clicked, this, [this   ]() {

                QString text = _process_timer_textbox->text();
                bool ok;
                double timer_val = text.toDouble(&ok);

                if (ok) {
                    timer->stop();
                    timer->start(timer_val);
                    emit log_print("  process time SET ");
                }
                else{
                    emit log_print("wrong process time select");
                }
            });


            c_kline_layout->addWidget(kline_test_button);
            c_kline_layout->addWidget(next_kline_test_button);
            c_kline_layout->addWidget(next_day_button);
            c_kline_layout->addWidget(next_5_hour_button);
            c_kline_layout->addWidget(kline_next_time_label);
            c_kline_layout->addWidget(_kline_next_time_label_textbox);
            c_kline_layout->addWidget(process_timer_label);
            c_kline_layout->addWidget(_process_timer_textbox);
            c_kline_layout->addWidget(process_timer_button);
        }


        QHBoxLayout *period_selector_layout = new QHBoxLayout(this);
        {
            QLabel *period_label = new QLabel("Standart Period Select:", this);

            _periot_textbox = new QLineEdit(this);
            _periot_textbox->setText(QString::number(HISTORY_MA_INDEX_DEF));

            QPushButton *set_period_button = new QPushButton("Set Period", this);
            connect(set_period_button, &QPushButton::clicked, this, [this]() {
                bool ok;
                int period = _periot_textbox->text().toInt(&ok);
                if (ok) {
                    _generic_period = period;

                    _RSI_MA_container->set_period(_generic_period);

                    emit log_print("periot SET done");
                }
            });

            // Add widgets to the layout
            period_selector_layout->addWidget(period_label);
            period_selector_layout->addWidget(_periot_textbox);
            period_selector_layout->addWidget(set_period_button);
        }

        QHBoxLayout *high_period_selector_layout = new QHBoxLayout(this);
        {
            QLabel *period_label = new QLabel("High Period Select:", this);

            _high_periot_textbox = new QLineEdit(this);
            _high_periot_textbox->setText(QString::number(HISTORY_MA_INDEX_DEF_MAX));

            QPushButton *set_period_button = new QPushButton("Set Period", this);
            connect(set_period_button, &QPushButton::clicked, this, [this]() {
                bool ok;
                int period = _high_periot_textbox->text().toInt(&ok);
                if (ok) {

                    _high_period = period;

                    _high_RSI_MA_container->set_period(_high_period);

                    emit log_print("high periot SET done");
                }
            });

            // Add widgets to the layout
            high_period_selector_layout->addWidget(period_label);
            high_period_selector_layout->addWidget(_high_periot_textbox);
            high_period_selector_layout->addWidget(set_period_button);
        }

        QHBoxLayout *kline_type_selection_layout = new QHBoxLayout(this);
        {
            QLabel* combobox_description = new QLabel("kline select :", this) ;
            _kline_select_combo_box = new QComboBox(this);
            _kline_select_combo_box->addItem("1 minute");
            _kline_select_combo_box->addItem("5 minute");
            _kline_select_combo_box->addItem("15 minute");
            _kline_select_combo_box->addItem("30 minute");
            _kline_select_combo_box->addItem("1 hour");
            _kline_select_combo_box->addItem("2 hour");
            _kline_select_combo_box->addItem("4 hour");

            QPushButton *set_kline_button = new QPushButton("Set Kline Type", this);
            connect(set_kline_button, &QPushButton::clicked, this, [this]() {

                QString strategyName = _kline_select_combo_box->currentText();

                emit log_print("  minute SET ");

                if (strategyName == "1 minute") {
                    selected_kline_type = kline_select_e::_1_minute;
                } else if (strategyName == "5 minute") {
                    selected_kline_type = kline_select_e::_5_minute;
                } else if (strategyName == "15 minute") {
                    selected_kline_type = kline_select_e::_15_minute;
                } else if (strategyName == "30 minute") {
                    selected_kline_type = kline_select_e::_30_minute;
                } else if (strategyName == "1 hour") {
                    selected_kline_type = kline_select_e::_1_hour;
                } else if (strategyName == "2 hour") {
                    selected_kline_type = kline_select_e::_2_hour;
                } else if (strategyName == "4 hour") {
                    selected_kline_type = kline_select_e::_4_hour;
                }
            });

            kline_type_selection_layout->addWidget(combobox_description);
            kline_type_selection_layout->addWidget(_kline_select_combo_box);
            kline_type_selection_layout->addWidget(set_kline_button);
        }



        QWidget* rsi_container = new QWidget(this);  // below @4578
        QWidget* candle_chart_container = new QWidget(this);  // below @4579

        history_layout->addWidget(candle_chart_container);
        history_layout->addWidget(rsi_container);
        history_layout->addLayout(coin_selection_layout);
        history_layout->addLayout(strategy_selection_layout);
        history_layout->addLayout(wallet_buy_sell_layout);
        history_layout->addLayout(kline_layout);
        history_layout->addLayout(c_kline_layout);
        history_layout->addLayout(period_selector_layout);
        history_layout->addLayout(high_period_selector_layout);
        history_layout->addLayout(kline_type_selection_layout);


        QHBoxLayout *strategy_configs_layout = new QHBoxLayout(this);
        {
            for(int i = 0 ; i <HISTORY_CONFIG_WIDGETS_MAX_AREA ; i++ ){
                _strategy_user_labels[i] = new QLabel(QString("config %1").arg(i + 1), this);
                _strategy_user_textboxes[i] = new QLineEdit("0",this);
                strategy_configs_layout->addWidget(_strategy_user_labels[i]);
                strategy_configs_layout->addWidget(_strategy_user_textboxes[i]);
                if((i+1)%5 == 0){
                    history_layout->addLayout(strategy_configs_layout);
                    strategy_configs_layout = new QHBoxLayout(this);
                }
            }

            QPushButton *set_config_button = new QPushButton("Set Configs", this);
            connect(set_config_button, &QPushButton::clicked, this, [this]() {

                update_strategy_config_container();

                emit log_print("configs SET done");

            });

            QVBoxLayout* containerLayout = new QVBoxLayout(rsi_container); //   @4578
            containerLayout->addWidget(_rsi_chart);
            QPushButton* rsi_shower_button = new QPushButton("Show/Hide rsi", this);
            connect(rsi_shower_button, &QPushButton::clicked, this, [rsi_container]() {
                rsi_container->setVisible(!rsi_container->isVisible());
            });

            QVBoxLayout* containerLayout2 = new QVBoxLayout(candle_chart_container); //   @4579
            containerLayout2->addWidget(_chart);
            QPushButton* candle_shower_button = new QPushButton("Show/Hide candles/MA", this);
            connect(candle_shower_button, &QPushButton::clicked, this, [candle_chart_container]() {
                candle_chart_container->setVisible(!candle_chart_container->isVisible());
            });


            strategy_configs_layout->addWidget(set_config_button);
            strategy_configs_layout->addWidget(rsi_shower_button);
            strategy_configs_layout->addWidget(candle_shower_button);
        }
        history_layout->addLayout(strategy_configs_layout);

    }
};


#endif
