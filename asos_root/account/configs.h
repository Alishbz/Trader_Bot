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
#ifndef ACC_CONFIGS_H
#define ACC_CONFIGS_H



//#define FUTURES_MODE_ON 1  // Vadeli işlemler MODU , API tarafında farklar oluyor



// Dikkat BNB ile al satda bunların hepsi düşüyor !!
//#define FAKE_COMMISSION_RATE    (double)(0.001)  // SPOT için
//#define FAKE_COMMISSION_RATE    (double)(0.0002)  // Vadeli işlemler Maker için, yani piyasaya likidite sağlayanlar, yani belli bir fiyattan alması için piyasaya emir girip bekleyenler
#define FAKE_COMMISSION_RATE    (double)(0.0005)  // Vadeli işlemler Taker için, yani piyasaya likidite sağlamadan alanlar, yani direk emirleri yerine getirilenler









#endif // COIN_LIST_H
