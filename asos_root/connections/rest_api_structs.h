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

#ifndef REST_API_STRUCTS_H
#define REST_API_STRUCTS_H

#include <QString>

enum class side_select_e {
    BUY,
    SELL
};

inline QString side_select_to_string(side_select_e select) {
    switch (select) {
    case side_select_e::BUY:
        return "BUY";
    case side_select_e::SELL:
        return "SELL";
    default:
        return "";
    }
}


/*******************/

enum class type_select_e {
    MARKET,
    LIMIT
};

inline QString type_select_to_string(type_select_e select) {
    switch (select) {
    case type_select_e::MARKET:
        return "MARKET";
    case type_select_e::LIMIT:
        return "LIMIT";
    default:
        return "";
    }
}




/*******************/

enum class kline_select_e {
    _1_second,
    _1_minute,
    _3_minute,
    _5_minute,
    _15_minute,
    _30_minute,
    _1_hour,
    _2_hour,
    _4_hour,
    _6_hour,
    _8_hour,
    _12_hour,
    _1_day,
    _3_day,
    _1_week,
    _1_month
};

inline QString kline_select_to_string(kline_select_e select) {
    switch (select) {
    case kline_select_e::_1_second:
        return "1s";
    case kline_select_e::_1_minute:
        return "1m";
    case kline_select_e::_3_minute:
        return "3m";
    case kline_select_e::_5_minute:
        return "5m";
    case kline_select_e::_15_minute:
        return "15m";
    case kline_select_e::_30_minute:
        return "30m";
    case kline_select_e::_1_hour:
        return "1h";
    case kline_select_e::_2_hour:
        return "2h";
    case kline_select_e::_4_hour:
        return "4h";
    case kline_select_e::_6_hour:
        return "6h";
    case kline_select_e::_8_hour:
        return "8h";
    case kline_select_e::_12_hour:
        return "12h";
    case kline_select_e::_1_day:
        return "1d";
    case kline_select_e::_3_day:
        return "3d";
    case kline_select_e::_1_week:
        return "1w";
    case kline_select_e::_1_month:
        return "1M";
    default:
        return "";
    }
}

// it is for timespend
inline qint64 kline_select_to_milisecond(kline_select_e select) {
    switch (select) {
    case kline_select_e::_1_second:
        return 1000;
    case kline_select_e::_1_minute:
        return 60 * 1000;
    case kline_select_e::_3_minute:
        return 3 * 60 * 1000;
    case kline_select_e::_5_minute:
        return 5 * 60 * 1000;
    case kline_select_e::_15_minute:
        return 15 * 60 * 1000;
    case kline_select_e::_30_minute:
        return 30 * 60 * 1000;
    case kline_select_e::_1_hour:
        return 60 * 60 * 1000;
    case kline_select_e::_2_hour:
        return 2 * 60 * 60 * 1000;
    case kline_select_e::_4_hour:
        return 4 * 60 * 60 * 1000;
    case kline_select_e::_6_hour:
        return 6 * 60 * 60 * 1000;
    case kline_select_e::_8_hour:
        return 8 * 60 * 60 * 1000;
    case kline_select_e::_12_hour:
        return 12 * 60 * 60 * 1000;
    case kline_select_e::_1_day:
        return 24 * 60 * 60 * 1000;
    case kline_select_e::_3_day:
        return 3 * 24 * 60 * 60 * 1000;
    case kline_select_e::_1_week:
        return 7 * 24 * 60 * 60 * 1000;
    case kline_select_e::_1_month:
       // return 30 * 24 * 60 * 60 * 1000; // Ortalama bir ayı 30 gün olarak hesaplıyoruz
        break;
    }
    return 0;
}


#endif
