#include "main_window.h"
#include "./ui_main_window.h"


#include "asos_root/logger_page.h"
#include "asos_root/history_page.h"
#include "asos_root/fake_account_page.h"
#include "asos_root/binance_account_page.h"


#define DEFAULT_SELECTED_COIN      coin_select_e::BTC


main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::main_window)
{
    ui->setupUi(this);

    /***************/

    binance_web_socket_c* web_socket = new binance_web_socket_c(this , DEFAULT_SELECTED_COIN);
    binance_rest_api_c*   rest_api = new binance_rest_api_c(this , DEFAULT_SELECTED_COIN);

    /***************/

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *_layout = new QHBoxLayout(this);

    logger_page_c * logger_page = new logger_page_c(this );

    binance_account_page_c* account_page = new binance_account_page_c(this,DEFAULT_SELECTED_COIN,web_socket,rest_api);
    history_page_c* history_page = new history_page_c(this,DEFAULT_SELECTED_COIN,web_socket,rest_api);
    fake_account_page_c* fake_page = new fake_account_page_c(this,DEFAULT_SELECTED_COIN,web_socket,rest_api);

    QTabWidget * tab_widget_controls = new QTabWidget(this);

    tab_widget_controls->addTab(fake_page, "fake account test");
    tab_widget_controls->addTab(account_page, "binance account");
    tab_widget_controls->addTab(history_page, "history test");

    _layout->addWidget(tab_widget_controls , 3);

    _layout->addWidget(logger_page , 1);

    layout->addLayout(_layout);

    setWindowTitle("ASOS BOT");

    QWidget *centralWidget = new QWidget(this);

    centralWidget->setLayout(layout);

    setCentralWidget(centralWidget);

    QIcon icon(":photos/icon3.png");
    if(!icon.isNull())
        setWindowIcon(icon);

    QObject::connect(rest_api, &binance_rest_api_c::user_data_stream_finded,
                     this, [this,web_socket](const QString& listen_key) {
                         web_socket->user_stream_web_socket_start(listen_key);
                     });

    QObject::connect(account_page, &binance_account_page_c::log_print, logger_page, &logger_page_c::log_print);
    QObject::connect(history_page, &history_page_c::log_print, logger_page, &logger_page_c::log_print);
    QObject::connect(web_socket, &binance_web_socket_c::log_print,logger_page, &logger_page_c::log_print);
    QObject::connect(rest_api, &binance_rest_api_c::log_print,logger_page, &logger_page_c::log_print);

}

main_window::~main_window()
{
    delete ui;
}
