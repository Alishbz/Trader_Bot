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
#ifndef FAKE_ACCOUNT_PAGE_H
#define FAKE_ACCOUNT_PAGE_H

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

#include "strategies/trade_strategy_interface.h"
#include "strategies/ex/simple_mx_strategy.h"

#include "charts/candle_graph.h"

#include "account/account_interface.h"
#include "connections/binance_rest_api.h"
#include "connections/binance_web_socket.h"

#include "generals/ohlc.h"

#include "custom_widgets/as_progressbar.h"

// some fast changable configs
#define FAKE_ACC_MA_INDEX_1   7
#define FAKE_ACC_MA_INDEX_2   25
#define FAKE_ACC_MA_INDEX_3   99

#define FAKE_ORDER_ID  "asosxxx47174asos"



// todo: fake account alma satma emri fazla sanal ve anında oluyor sanki rest apiye sormuş gibi 500ms felan delay ile alıp satsın !!
class fake_account_page_c : public QWidget ,public account_interface {
    Q_OBJECT

public:
    fake_account_page_c(QWidget*                 parent = nullptr ,
                        coin_select_e            e = coin_select_e::BTC,
                        binance_web_socket_c*    web_socket = nullptr,
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

        if(_web_socket){
            QObject::connect(_web_socket, &binance_web_socket_c::coin_price_update,
                             this, &fake_account_page_c::async_candle_data_update);

            QObject::connect(_web_socket, &binance_web_socket_c::depth_bids_ratio_update,
                             this, [this](double bids_ratio){
                                 _trader.async_depth_update(bids_ratio);
                                 _depth_progress_bar->setValue((int)(bids_ratio*100));
                             });
        }

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &fake_account_page_c::one_second_timer_callback);
        timer->start(1000);
    }

    void web_socket_set(binance_web_socket_c* s)  {
        _web_socket = s;
        if(_web_socket){
            QObject::connect(_web_socket, &binance_web_socket_c::coin_price_update,
                             this, &fake_account_page_c::async_candle_data_update);

            QObject::connect(_web_socket, &binance_web_socket_c::depth_bids_ratio_update,
                             this, [this](double bids_ratio){
                                 _trader.async_depth_update(bids_ratio);
                                 _depth_progress_bar->setValue((int)(bids_ratio*100));
                             });
        }
    }

    void rest_api_set(binance_rest_api_c* s)  {
        _rest_api = s;
    }

    /*** Interface  *********************************************************/

    void select_coin(coin_select_e s) override
    {
        change_coin_selection_and_restart_websocket(s);
    }

    void buy_order()override{
        direct_buy_order();
    }

    void short_order()override{
        // todo
    }

    bool is_short_done()override{
        //todo
        return false;
    }

    bool is_buy_done()override{

        return _is_buy_enable;
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
        OHLC_s seed = _1_sc_ohlc_creator.create_ohlc();
        if(!seed.is_empty()){

            _trader.async_data_update(seed);

            _trader.execute_strategy();

            update_containers(seed);

            update_graphs(seed);
        }
    }

    void async_candle_data_update(double data){

        _1_sc_ohlc_creator.async_data_update(data);

        wallet_update(data);
    }

signals:
    void log_print(const QString &message);



private:  // functions

    void update_containers(const OHLC_s& ohlc_seed) {
        _RSI_MA_container_1->add(ohlc_seed);
        _RSI_MA_container_2->add(ohlc_seed);
        _RSI_MA_container_3->add(ohlc_seed);
    }

    void update_graphs(const OHLC_s& ohlc_seed) {

        _realtime_web_socket_chart->OHLC_update(ohlc_seed);

        // _rsi_chart->OHLC_update( );  // todo in the heature update HACIM !! to web soket but you must be resize ranges to 0 - 100

        if(_RSI_MA_container_1->calculate_moving_average() > 0)
            _realtime_web_socket_chart->line_update(candle_line_select_e::LINE_1 ,_RSI_MA_container_1->calculate_moving_average());

        if(_RSI_MA_container_2->calculate_moving_average() > 0)
            _realtime_web_socket_chart->line_update(candle_line_select_e::LINE_2 ,_RSI_MA_container_2->calculate_moving_average());

        if(_RSI_MA_container_3->calculate_moving_average() > 0)
            _realtime_web_socket_chart->line_update(candle_line_select_e::LINE_3 ,_RSI_MA_container_3->calculate_moving_average());

        if(_RSI_MA_container_1->last_rsi > 0)
            _rsi_chart->line_update(candle_line_select_e::LINE_1 , _RSI_MA_container_1->calculate_rsi());

        if(_RSI_MA_container_2->last_rsi > 0)
            _rsi_chart->line_update(candle_line_select_e::LINE_2 ,_RSI_MA_container_2->calculate_rsi());

        if(_RSI_MA_container_3->last_rsi > 0)
            _rsi_chart->line_update(candle_line_select_e::LINE_3 ,_RSI_MA_container_3->calculate_rsi());
    }

    void wallet_update(double data){
        _last_coin_price = data;
        if(  _is_buy_enable){
            double new_wallet_price =   _record_wallet_for_buy * _last_coin_price /  _record_coin_price_for_buy;
            set_wallet_text(new_wallet_price);
        }
    }

    void direct_buy_order(){
        if( _is_buy_enable == false){
            emit log_print(" FAKE_PAGE: buy order: coin current price: " + QString::number( _last_coin_price));
            _is_buy_enable = true;
            _record_coin_price_for_buy =  _last_coin_price;
            _record_wallet_for_buy = read_wallet_text();
        }
    }


    void direct_sell_order(){
        if( _is_buy_enable){
            emit log_print(" FAKE_PAGE: account sell order: coin current price: " + QString::number( _last_coin_price));
            _is_buy_enable = false;
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
            qWarning("Conversion to double failed. Invalid input.xx");
            return 0.0;
        }
        return value;
    }



    void change_coin_selection_and_restart_websocket(coin_select_e e = coin_select_e::EMPTY){

        coin_select_e _new_coin =  coin_select_e::EMPTY;

        if( e == coin_select_e::EMPTY){
            QString coin_str = _coin_select_combo_box->currentText();

            for(int i = 0 ;i< MAX_COUIN_SUPPORT_COUNT ; i++){

                if(coin_name_list[i] == coin_str)
                {
                    _new_coin = (coin_select_e)i;
                    break;
                }
            }

            if( _new_coin == coin_select_e::EMPTY){

                emit log_print("coin selection fail");

                return;
            }
        }
        else{
            _new_coin = e;
        }

        _selected_coin = _new_coin;

        sell_order();

        _rest_api->select_coin(_new_coin);

        _trader.set_strategy(nullptr);

        QTimer::singleShot(1000, this, [=]() {
            if (_web_socket) {
                _web_socket->trade_web_socket_close();
                _web_socket->depth_web_socket_close();

                QTimer::singleShot(1000, this, [=]() {
                    _web_socket->trade_web_socket_start(_new_coin);
                    _web_socket->depth_web_socket_start(_new_coin);

                    _RSI_MA_container_1->clear();
                    _RSI_MA_container_2->clear();
                    _RSI_MA_container_3->clear();

                    _rsi_chart->clear_chart();
                    _realtime_web_socket_chart->clear_chart();

                    emit log_print("FAKE: coin selection DONE");
                });
            }
        });
    }


private:  // ex datas

    bool _is_buy_enable ;
    double _record_coin_price_for_buy ;
    double _record_wallet_for_buy ;
    double _last_coin_price ;

    qint64 _st_time ;
    qint64 _sp_time  ;

    // comms ...
    binance_web_socket_c*    _web_socket= nullptr;;
    binance_rest_api_c*      _rest_api= nullptr;;

    // containers ...
    OHLC_core_creator        _1_sc_ohlc_creator;

    OHLC_container*          _RSI_MA_container_1= nullptr;;
    OHLC_container*          _RSI_MA_container_2= nullptr;;
    OHLC_container*          _RSI_MA_container_3= nullptr;;

    coin_select_e _selected_coin;

    trader_c _trader;
    simple_mx_strategy_c * _strategy_smx_1= nullptr;;
    simple_mx_strategy_c * _strategy_smx_2= nullptr;;
    simple_mx_strategy_c * _strategy_smx_3= nullptr;;

    void _params_init_create(){
        _is_buy_enable = false;
        _record_coin_price_for_buy = 0;
        _record_wallet_for_buy = 0;
        _last_coin_price = 0;
        _st_time=0;
        _sp_time = 0;

        _strategy_smx_1 = nullptr;
        _strategy_smx_2 = nullptr;
        _strategy_smx_3 = nullptr;

        // create containers
        _RSI_MA_container_1 = new OHLC_container(FAKE_ACC_MA_INDEX_1);
        _RSI_MA_container_2 = new OHLC_container(FAKE_ACC_MA_INDEX_2);
        _RSI_MA_container_3 = new OHLC_container(FAKE_ACC_MA_INDEX_3);
    }

private:  // widget datas

    candle_graph_c * _rsi_chart = nullptr;
    candle_graph_c * _realtime_web_socket_chart = nullptr;
    QLineEdit * _wallet_textbox = nullptr;
    QLineEdit * _kline_start_time_textbox = nullptr;
    QLineEdit * _kline_stop_time_textbox = nullptr;
    QComboBox * _strategy_select_combo_box = nullptr;
    QComboBox * _coin_select_combo_box= nullptr;
    as_progressbar *_depth_progress_bar = nullptr;

    void _load_design_page(QVBoxLayout* main_layout){

        std::array<QString, static_cast<size_t>(candle_line_select_e::MAX_SIZE)>
            _line_names = {
                "MA_1", "MA_2", "MA_3"
            };
        std::array<QString, static_cast<size_t>(candle_line_select_e::MAX_SIZE)>
            _rsi_line_names = {
                "RSI_1", "RSI_2", "RSI_3"
            };

        _realtime_web_socket_chart = new candle_graph_c(this," real time chart " , _line_names, 50);
        _rsi_chart = new candle_graph_c(this," rsi chart " , _rsi_line_names, 50);

        QHBoxLayout *strategy_selection_layout = new QHBoxLayout(this);
        {
            QLabel* combobox_description = new QLabel("select strategy :", this) ;
            _strategy_select_combo_box = new QComboBox(this);
            _strategy_select_combo_box->addItem("simple strategy mx1");
            _strategy_select_combo_box->addItem("simple strategy mx2");
            _strategy_select_combo_box->addItem("simple strategy mx3");
            QPushButton *start_strategy_button = new QPushButton("Start Strategy", this);
            QPushButton *stop_strategy_button = new QPushButton("Stop Strategy", this);
            connect(start_strategy_button, &QPushButton::clicked,
                    this, [this]() {
                        QString strategyName = _strategy_select_combo_box->currentText();
                        if (strategyName == "simple strategy mx1") {
                            emit log_print("simple strategy mx1 CREATING & STARING");
                            if(_strategy_smx_1)
                                delete _strategy_smx_1;
                            _strategy_smx_1 = new simple_mx_strategy_c(FAKE_ACC_MA_INDEX_1 , this);
                            _trader.set_strategy(_strategy_smx_1);
                        }
                        else if (strategyName == "simple strategy mx2") {
                            emit log_print("simple strategy mx2 CREATING & STARING");
                            if(_strategy_smx_2)
                                delete _strategy_smx_2;
                            _strategy_smx_2 = new simple_mx_strategy_c(FAKE_ACC_MA_INDEX_2 , this);
                            _trader.set_strategy(_strategy_smx_2);
                        }
                        else if (strategyName == "simple strategy mx3") {
                            emit log_print("simple strategy mx3 CREATING & STARING");
                            if(_strategy_smx_3)
                                delete _strategy_smx_3;
                            _strategy_smx_3 = new simple_mx_strategy_c(FAKE_ACC_MA_INDEX_3 , this);
                            _trader.set_strategy(_strategy_smx_3);
                        }
                    });
            connect(stop_strategy_button, &QPushButton::clicked,
                    this, [this]() {
                        emit log_print("selected strategy STOPING");
                        sell_order();
                        _trader.set_strategy(nullptr);
                    });
            /** add to layout **/
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
                        change_coin_selection_and_restart_websocket();
                    });

            /** add to layout **/
            coin_selection_layout->addWidget(coin_combobox_description);
            coin_selection_layout->addWidget(_coin_select_combo_box);
            coin_selection_layout->addWidget(select_coin_button);
        }

        QHBoxLayout *wallet_buy_sell_layout = new QHBoxLayout(this);
        {
            _wallet_textbox = new QLineEdit(this);
            _wallet_textbox->setText("1000");
            QFont font = _wallet_textbox->font();
            font.setPointSize(32);
            _wallet_textbox->setFont(font);
            _wallet_textbox->setPlaceholderText("Enter amount");
            QPushButton* buy_button = new QPushButton("Buy", this);
            QPushButton* sell_button = new QPushButton("Sell", this);
            connect(buy_button, &QPushButton::clicked, this, [this]() {
                direct_buy_order();
            });
            connect(sell_button, &QPushButton::clicked, this, [this]() {
                direct_sell_order();
            });
            QPushButton *start_web_socket_button = new QPushButton("start_web_socket", this);
            QPushButton *stop_web_socket_button = new QPushButton("stop_web_socket", this);
            connect(start_web_socket_button, &QPushButton::clicked, this, [this]() {
                if(_web_socket){
                    _web_socket->trade_web_socket_start(_selected_coin);
                    _web_socket->depth_web_socket_start(_selected_coin);
                }
            });
            connect(stop_web_socket_button, &QPushButton::clicked, this, [this]() {
                if(_web_socket){
                    _web_socket->trade_web_socket_close();
                    _web_socket->depth_web_socket_close();
                }
            });
            /** add to layout **/
            wallet_buy_sell_layout->addWidget(new QLabel("Fake Wallet:", this));
            wallet_buy_sell_layout->addWidget(_wallet_textbox);
            wallet_buy_sell_layout->addWidget(buy_button);
            wallet_buy_sell_layout->addWidget(sell_button);
            wallet_buy_sell_layout->addWidget(start_web_socket_button);
            wallet_buy_sell_layout->addWidget(stop_web_socket_button);
        }

        QHBoxLayout *rest_api_test_layout = new QHBoxLayout(this);
        {
            QPushButton *get_account_button = new QPushButton("Get account and Wallet", this);
            QPushButton *get_trades_button = new QPushButton("Get My Trades", this);
            QPushButton *get_open_orders_button = new QPushButton("Get Open Orders", this);
            QPushButton *send_order_button = new QPushButton("Send TEST Order", this);
            QPushButton *get_order_button = new QPushButton("Get Related Order", this);
            QPushButton *close_order_button = new QPushButton("Close Related Order", this);
            QPushButton *user_data_stream_button = new QPushButton("GET user data stream", this);

            connect(get_account_button, &QPushButton::clicked, this, [this]() {
                if(_rest_api)
                    _rest_api->get_account(5000);
            });

            connect(get_trades_button, &QPushButton::clicked, this, [this]() {
                if(_rest_api)
                    _rest_api->get_my_trades(_selected_coin, 0, 0 ,5000);
            });

            connect(get_open_orders_button, &QPushButton::clicked, this, [this]() {
                if(_rest_api)
                    _rest_api->get_open_orders(_selected_coin,5000);
            });

            connect(user_data_stream_button, &QPushButton::clicked, this, [this]() {
                if(_rest_api)
                    _rest_api->get_user_data_stream( );
            });

            connect(send_order_button, &QPushButton::clicked, this, [this]() {
                _rest_api->send_order(_selected_coin,
                                      side_select_e::SELL,
                                      type_select_e::LIMIT,
                                      45,
                                      0.6365,
                                      0,
                                      FAKE_ORDER_ID,
                                      5000);
            });

            connect(get_order_button, &QPushButton::clicked, this, [this]() {
                _rest_api->get_order(_selected_coin,
                                     FAKE_ORDER_ID,
                                     5000);
            });

            connect(close_order_button, &QPushButton::clicked, this, [this]() {
                _rest_api->close_order(_selected_coin,
                                       FAKE_ORDER_ID,
                                       5000);
            });
            /** add to layout **/
            rest_api_test_layout->addWidget(get_account_button);
            rest_api_test_layout->addWidget(get_trades_button);
            rest_api_test_layout->addWidget(get_open_orders_button);
            rest_api_test_layout->addWidget(send_order_button);
            rest_api_test_layout->addWidget(get_order_button);
            rest_api_test_layout->addWidget(close_order_button);
            rest_api_test_layout->addWidget(user_data_stream_button);

        }


        _depth_progress_bar = new as_progressbar(this);
        _depth_progress_bar->setRange(0, 100);
        _depth_progress_bar->setValue(40); // Progress değeri örnek


        main_layout->addWidget(_realtime_web_socket_chart);
        main_layout->addWidget(_rsi_chart);
        main_layout->addLayout(coin_selection_layout);
        main_layout->addLayout(strategy_selection_layout);
        main_layout->addLayout(wallet_buy_sell_layout);
        main_layout->addLayout(rest_api_test_layout);
        main_layout->addWidget(_depth_progress_bar);
    }
};






#endif
