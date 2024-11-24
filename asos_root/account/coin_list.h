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
#ifndef COIN_LIST_H
#define COIN_LIST_H

#include <QString>
#include <Map>

#include "configs.h"

#define MAX_COUIN_SUPPORT_COUNT 20

enum class coin_select_e {
    BTC,
    ETH,
    BNB,
    XRP ,
    LTC ,
    ADA ,
    DOT ,
    LINK ,
    XLM ,
    TRX ,
    EOS ,
    XTZ ,
    BCH ,
    VET ,
    ICX ,
    XAI ,
    ATOM ,
    PIXEL ,
    ONT ,
    IOTA ,
    EMPTY    // index of MAX_COUIN_SUPPORT_COUNT
};








/*********************************************************************/
/*** NORMAL SELECTION - ex: "BTC" ******************************************************************/
/*********************************************************************/


static constexpr std::array<const char*, MAX_COUIN_SUPPORT_COUNT> coin_name_list = {
    "BTC",
    "ETH",
    "BNB",
    "XRP",
    "LTC",
    "ADA",
    "DOT",
    "LINK",
    "XLM",
    "TRX",
    "EOS",
    "XTZ",
    "BCH",
    "VET",
    "ICX",
    "XAI",
    "ATOM",
    "PIXEL",
    "ONT",
    "IOTA"
};


inline const std::map<coin_select_e, QString>& get_coin_name_map() {
    static const std::map<coin_select_e, QString> coin_name_map = {
        {coin_select_e::BTC, coin_name_list[0]},
        {coin_select_e::ETH, coin_name_list[1]},
        {coin_select_e::BNB, coin_name_list[2]},
        {coin_select_e::XRP, coin_name_list[3]},
        {coin_select_e::LTC, coin_name_list[4]},
        {coin_select_e::ADA, coin_name_list[5]},
        {coin_select_e::DOT, coin_name_list[6]},
        {coin_select_e::LINK,coin_name_list[7]},
        {coin_select_e::XLM, coin_name_list[8]},
        {coin_select_e::TRX, coin_name_list[9]},
        {coin_select_e::EOS, coin_name_list[10]},
        {coin_select_e::XTZ, coin_name_list[11]},
        {coin_select_e::BCH, coin_name_list[12]},
        {coin_select_e::VET, coin_name_list[13]},
        {coin_select_e::ICX, coin_name_list[14]},
        {coin_select_e::XAI, coin_name_list[15]},
        {coin_select_e::ATOM, coin_name_list[16]},
        {coin_select_e::PIXEL,coin_name_list[17]},
        {coin_select_e::ONT, coin_name_list[18]},
        {coin_select_e::IOTA, coin_name_list[19]}
    };
    return coin_name_map;
}


inline QString coin_map_get_enum_to_str(coin_select_e _selected_coin) {
    const auto& map = get_coin_name_map();
    auto it = map.find(_selected_coin);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}












/*********************************************************************/
/*** for WebSocket-CHART TRANSFORMS - ex: "btcusdt" ******************************************************************/
/*********************************************************************/



static constexpr std::array<const char*, MAX_COUIN_SUPPORT_COUNT> coin_stream_name_list = {
    "btcusdt",
    "ethusdt",
    "bnbusdt",
    "xrpusdt",
    "ltcusdt",
    "adausdt",
    "dotusdt",
    "linkusdt",
    "xlmusdt",
    "trxusdt",
    "eosusdt",
    "xtzusdt",
    "bchusdt",
    "vetusdt",
    "icxusdt",
    "xaiusdt",
    "atomusdt",
    "pixelusdt",
    "ontusdt",
    "iotausdt"
};






inline const std::map<coin_select_e, QString>& get_coin_stream_name_map() {
    static const std::map<coin_select_e, QString> coin_stream_name_map = {
        {coin_select_e::BTC, coin_stream_name_list[0]},
        {coin_select_e::ETH, coin_stream_name_list[1]},
        {coin_select_e::BNB, coin_stream_name_list[2]},
        {coin_select_e::XRP, coin_stream_name_list[3]},
        {coin_select_e::LTC, coin_stream_name_list[4]},
        {coin_select_e::ADA, coin_stream_name_list[5]},
        {coin_select_e::DOT, coin_stream_name_list[6]},
        {coin_select_e::LINK,coin_stream_name_list[7]},
        {coin_select_e::XLM, coin_stream_name_list[8]},
        {coin_select_e::TRX, coin_stream_name_list[9]},
        {coin_select_e::EOS, coin_stream_name_list[10]},
        {coin_select_e::XTZ, coin_stream_name_list[11]},
        {coin_select_e::BCH, coin_stream_name_list[12]},
        {coin_select_e::VET, coin_stream_name_list[13]},
        {coin_select_e::ICX, coin_stream_name_list[14]},
        {coin_select_e::XAI, coin_stream_name_list[15]},
        {coin_select_e::ATOM, coin_stream_name_list[16]},
        {coin_select_e::PIXEL,coin_stream_name_list[17]},
        {coin_select_e::ONT, coin_stream_name_list[18]},
        {coin_select_e::IOTA, coin_stream_name_list[19]}
    };
    return coin_stream_name_map;
}



inline QString coin_stream_map_get_enum_to_str(coin_select_e _selected_coin) {
    const auto& map = get_coin_stream_name_map();
    auto it = map.find(_selected_coin);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}























/*********************************************************************/
/*** for HTTP API - ex: "BTCUSDT" ******************************************************************/
/*********************************************************************/



static constexpr std::array<const char*, MAX_COUIN_SUPPORT_COUNT> coin_api_name_list = {
    "BTCUSDT",
    "ETHUSDT",
    "BNBUSDT",
    "XRPUSDT",
    "LTCUSDT",
    "ADAUSDT",
    "DOTUSDT",
    "LINKUSDT",
    "XLMUSDT",
    "TRXUSDT",
    "EOSUSDT",
    "XTZUSDT",
    "BCHUSDT",
    "VETUSDT",
    "ICXUSDT",
    "XAIUSDT",
    "ATOMUSDT",
    "PIXELUSDT",
    "ONTUSDT",
    "IOTAUSDT"
};





inline const std::map<coin_select_e, QString>& get_coin_api_name_map() {
    static const std::map<coin_select_e, QString> coin_api_name_map = {
        {coin_select_e::BTC, coin_api_name_list[0]},
        {coin_select_e::ETH, coin_api_name_list[1]},
        {coin_select_e::BNB, coin_api_name_list[2]},
        {coin_select_e::XRP, coin_api_name_list[3]},
        {coin_select_e::LTC, coin_api_name_list[4]},
        {coin_select_e::ADA, coin_api_name_list[5]},
        {coin_select_e::DOT, coin_api_name_list[6]},
        {coin_select_e::LINK,coin_api_name_list[7]},
        {coin_select_e::XLM, coin_api_name_list[8]},
        {coin_select_e::TRX, coin_api_name_list[9]},
        {coin_select_e::EOS, coin_api_name_list[10]},
        {coin_select_e::XTZ, coin_api_name_list[11]},
        {coin_select_e::BCH, coin_api_name_list[12]},
        {coin_select_e::VET, coin_api_name_list[13]},
        {coin_select_e::ICX, coin_api_name_list[14]},
        {coin_select_e::XAI, coin_api_name_list[15]},
        {coin_select_e::ATOM, coin_api_name_list[16]},
        {coin_select_e::PIXEL,coin_api_name_list[17]},
        {coin_select_e::ONT, coin_api_name_list[18]},
        {coin_select_e::IOTA, coin_api_name_list[19]}
    };
    return coin_api_name_map;
}



inline QString coin_api_map_get_enum_to_str(coin_select_e _selected_coin) {
    const auto& map = get_coin_api_name_map();
    auto it = map.find(_selected_coin);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}















#endif // COIN_LIST_H
