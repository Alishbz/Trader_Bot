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
#ifndef ACCOUNT_INTERFACE_H
#define ACCOUNT_INTERFACE_H

#include "coin_list.h"

class account_interface {
public:
    virtual ~account_interface() = default;

    virtual void select_coin(coin_select_e s) = 0;
    virtual void buy_order( ) = 0;
    virtual void sell_order() = 0;
    virtual double get_wallet() = 0;
    virtual bool is_buy_done() = 0;
    virtual bool is_sell_done() = 0;
    virtual void short_order( ) = 0;
    virtual bool is_short_done() = 0;
};

#endif // ACCOUNT_INTERFACE_H
