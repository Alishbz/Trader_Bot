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

#ifndef OHLC_CONTAINERS_H
#define OHLC_CONTAINERS_H

#include <QVector>
#include <numeric>
#include <QMap>
#include <QPair>
#include <QDebug>

#include <cmath>
#include <algorithm>

#include <limits>
#include <cmath>


struct OHLC_s{
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;

    void clear(){
        open = 0;
        high = 0;
        low = 0;
        close = 0;
    }

    bool is_green() const {
        return close > open;
    }

    bool is_empty() const {
        return open == 0 && close == 0 && high == 0 && low == 0;
    }

    double get_average_all_price() const {
        return (open + high + low + close) / 4.0;
    }

    double get_average_mid_point() const {
        return  close;
    }
};

inline bool is_rising_three_method(const QVector<OHLC_s>& candles) {

    if (candles.size() < 5) {
        return false; // En az 5 mum çubuğu olmalı.
    }

    // Son beş mum çubuğunu alıyoruz.
    const OHLC_s& first = candles[candles.size() - 5];
    const OHLC_s& second = candles[candles.size() - 4];
    const OHLC_s& third = candles[candles.size() - 3];
    const OHLC_s& fourth = candles[candles.size() - 2];
    const OHLC_s& fifth = candles[candles.size() - 1];

    if (first.close <= first.open) {
        return false;
    }

    if (second.open >= first.close || second.close <= first.open ||
        third.open >= first.close || third.close <= first.open ||
        fourth.open >= first.close || fourth.close <= first.open ||
        second.close >= second.open || third.close >= third.open || fourth.close >= fourth.open) {
        return false;
    }

    if (fifth.close <= fifth.open || fifth.close <= first.close) {
        return false;
    }

    return true;
}










/**
  TODO add features:
1. Bollinger Bands
2. MACD (Moving Average Convergence Divergence)
3. Stochastic Oscillator
4. Average True Range (ATR)
5. Parabolic SAR (Stop and Reverse)
6. Fibonacci Retracement
7. Ichimoku Cloud
8. On-Balance Volume (OBV)
10. Chaikin Money Flow (CMF)
11. Exponential Moving Average (EMA)
12. Keltner Channel
**/
struct OHLC_container {

#define OHLC_C_MAX_SIZE period

    OHLC_container( size_t  _period = 25 ):
        period(_period)
    {}

    void init() {
        clear();
    }

    void set_period(size_t  _period) {
        if(_period > 0)
            period = _period ;
    }




    void add(const OHLC_s& ohlc) {
        int c_size = container.size();
        if (c_size > OHLC_C_MAX_SIZE-1) {

            if(current_avarage_gain != 0){
                previous_avarage_gain = current_avarage_gain;
                //previous_avarage_gain = (current_avarage_gain  +  previous_avarage_gain )/2;
            }
            current_avarage_gain = calculate_gain(true);

            if(current_avarage_loss != 0){
                previous_avarage_loss = current_avarage_loss;
                //previous_avarage_loss = (previous_avarage_loss  +  current_avarage_loss )/2;
            }
            current_avarage_loss = calculate_loss(true);

            current_gain = calculate_gain(false);
            current_loss = calculate_loss(false);

            last_standard_deviation = calculate_standard_deviation();

            last_rsi = calculate_rsi();
            last_moving_avarage = calculate_moving_average();
            last_slope = calculate_slope();

            //qDebug() << "Angle (degrees):" << last_slope;// todo clear

            last_dema = calculate_dema();
            calculate_max_min_in_period();
            last_mode_avarage = calculate_mode_average();  // must be calculate_max_min_in_period called first
            container.removeFirst();
        }
        container.push_back(ohlc); //container.append(ohlc);
    }

    void clear() {
        container.clear();
        container.resize(OHLC_C_MAX_SIZE);
    }

    bool is_empty() const {
        return container.isEmpty();
    }


    /** Math: *********/

    int get_desire_red_ohlc_size_count_in_period( int desire_last_period , float ohlc_percent_value) const {
        if (is_empty() || desire_last_period < 1 || period < desire_last_period || period > container.size()) {
            return false;
        }

        int count = 0;
        for (int i = container.size() - desire_last_period; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (!ohlc.is_green()) {
                float data_size = ((ohlc.low - ohlc.high) / ohlc.high) * 100;

                if(data_size > ohlc_percent_value*(-1))
                    count++;
            }
        }

        return count;
    }

    int get_desire_green_ohlc_size_count_in_period( int desire_last_period , float ohlc_percent_value) const {
        if (is_empty() || desire_last_period < 1 || period < desire_last_period || period > container.size()) {
            return false;
        }

        int count = 0;
        for (int i = container.size() - desire_last_period; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (ohlc.is_green()) {
                float data_size = ((ohlc.high - ohlc.low) / ohlc.low) * 100;

                if(data_size > ohlc_percent_value)
                    count++;
            }
        }

        return count;
    }

    int get_red_ohlc_count_in_period( int desire_period  ) const {
        if (is_empty() || desire_period < 1 || period < desire_period || period > container.size()) {
            return 0;
        }

        int count = 0;
        for (int i = container.size() - desire_period; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (!ohlc.is_green() ) {
                count++;
            }
        }

        return count;
    }

    int get_green_ohlc_count_in_period( int desire_period  ) const {
        if (is_empty() || desire_period < 1 || period < desire_period || period > container.size()) {
            return 0;
        }

        int count = 0;
        for (int i = container.size() - desire_period; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (ohlc.is_green() ) {
                count++;
            }
        }

        return count;
    }


    std::pair<bool, float> is_last_three_data_green() const {
        if (is_empty() || period < 4 || period > container.size()) {
            return std::make_pair(false, 0.0f);
        }

        bool is_green = true;
        float three_data_size = 0.0f;

        for (int i = container.size() - 3; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (!ohlc.is_green()) {
                return std::make_pair(false, 0.0f);
            }

            switch (i - (container.size() - 3)) {
            case 0:
                three_data_size += ((ohlc.close - ohlc.low) / ohlc.low) * 100;
                break;
            case 1:
                three_data_size += ((ohlc.close - ohlc.open) / ohlc.open) * 100;
                break;
            case 2:
                three_data_size += ((ohlc.high - ohlc.open) / ohlc.open) * 100;
                break;
            default:
                three_data_size = 0; // ERROR
                break;
            }
        }

        return std::make_pair(is_green, three_data_size);
    }

    std::pair<bool, float> is_last_three_data_red() const {
        if (is_empty() || period < 4 || period > container.size()) {
            return std::make_pair(false, 0.0f);
        }

        bool is_red = true;
        float three_data_size = 0.0f;

        for (int i = container.size() - 3; i < container.size(); ++i) {
            const OHLC_s& ohlc = container[i];
            if (ohlc.is_green()) {
                return std::make_pair(false, 0.0f);
            }

            switch (i - (container.size() - 3)) {
            case 0:
                three_data_size += ((ohlc.high - ohlc.close) / ohlc.close) * 100;
                break;
            case 1:
                three_data_size += ((ohlc.open - ohlc.close) / ohlc.close) * 100;
                break;
            case 2:
                three_data_size += ((ohlc.open - ohlc.low) / ohlc.low) * 100;
                break;
            default:
                three_data_size = 0; // ERROR
                break;
            }
        }

        return std::make_pair(is_red, three_data_size);
    }


    double calculate_standard_deviation() const {
        if (container.size() < period) {
            return 0.0;
        }

        double mean = 0.0;
        for (const auto& ohlc : container) {
            mean += ohlc.get_average_mid_point();
        }
        mean /= container.size();

        double variance = 0.0;
        for (const auto& ohlc : container) {
            double diff = ohlc.get_average_mid_point() - mean;
            variance += diff * diff;
        }
        variance /= container.size();

        return std::sqrt(variance);
    }

    double calculate_moving_average() const
    {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        double sum = 0.0;
        for (const auto& ohlc : container) {
            sum += ohlc.get_average_all_price();
        }

        return sum / container.size();
    }

    double calculate_rsi(int rsi_period = 26) const {       // not: konteynır periodu rsi periodundan çok daha uzun olunca çalıştı gibi
        if (is_empty() || rsi_period >= container.size()) {
            return 0.0;
        }

        double gain = 0.0;
        double loss = 0.0;

        // İlk rsi_period için ortalama kazanç ve kaybı hesapla
        for (size_t i = 1; i <= rsi_period; ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                gain += change;
            } else {
                loss += -change;
            }
        }

        gain /= rsi_period;
        loss /= rsi_period;

        // Sonraki dönemler için değerleri güncelle
        for (size_t i = rsi_period + 1; i < container.size(); ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                gain = (gain * (rsi_period - 1) + change) / rsi_period;
                loss = (loss * (rsi_period - 1)) / rsi_period;
            } else {
                gain = (gain * (rsi_period - 1)) / rsi_period;
                loss = (loss * (rsi_period - 1) - change) / rsi_period;
            }
        }

        if (loss == 0) {
            return 100.0; // Tüm kazançlar pozitif olduğunda RSI 100 olur
        }

        double rs = gain / loss;
        double rsi = 100.0 - (100.0 / (1.0 + rs));

        return rsi;
    }


    /*
    double calculate_rsi() const
    {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        QVector<double> gains;
        QVector<double> losses;

        for (int i = 1; i < container.size(); ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                gains.append(change);
                losses.append(0);
            } else {
                gains.append(0);
                losses.append(-change);
            }
        }

        double avg_gain = std::accumulate(gains.end() - period, gains.end(), 0.0) / period;
        double avg_loss = std::accumulate(losses.end() - period, losses.end(), 0.0) / period;

        double rs = avg_loss == 0 ? 100 : avg_gain / avg_loss;
        return 100 - (100 / (1 + rs));
    }*/

/*
    double calculate_rsi( ) const
    {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        QVector<double> gains;
        QVector<double> losses;

        for (int i = 1; i < container.size(); ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                gains.append(change);
                losses.append(0);
            } else {
                gains.append(0);
                losses.append(-change);
            }
        }

        double avg_gain = std::accumulate(gains.end() - period, gains.end(), 0.0) / period;
        double avg_loss = std::accumulate(losses.end() - period, losses.end(), 0.0) / period;

        // EMA
        for (int i = container.size() - period+1; i < container.size(); ++i) { // bu period + 1 olacak
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                avg_gain = ((avg_gain * (period - 1)) + change) / period;
                avg_loss = ((avg_loss * (period - 1))) / period;
            } else {
                avg_gain = ((avg_gain * (period - 1))) / period;
                avg_loss = ((avg_loss * (period - 1)) - change) / period;
            }
        }

        double rs = avg_loss == 0 ? 100 : avg_gain / avg_loss;
        double rsi = 100 - (100 / (1 + rs));  // normal rsi
        return rsi;

        // Kalibrasyon
        //double m = 0.6471718004517067;
        // double b = 17.72321161251955;
        // return m * rsi + b;
    }
*/
    double calculate_ema2(const QVector<double>& prices, size_t period) const {
        if (prices.isEmpty() || period > prices.size()) {
            return 0.0;
        }

        double multiplier = 2.0 / (period + 1);
        double sma = std::accumulate(prices.begin(), prices.begin() + period, 0.0) / period;  // İlk period kadar fiyatın SMA'sını hesapla
        double ema = sma;

        for (int i = period; i < prices.size(); ++i) {
            ema = ((prices[i] - ema) * multiplier) + ema;
        }

        return ema;
    }







    /**
        Average Gain = [(Previous Average Gain) x (N-1) + Current Gain] / N

        Where N is the number of time periods selected, typically 14 days.

        The formula for calculating the average loss is:

        Average Loss = [(Previous Average Loss) x (N-1) + Current Loss] / N

        Where N is the number of time periods selected, typically 14 days.

        The RSI calculation is then as follows:

        RSI = 100 - (100 / (1 + RS))
        **/

    /* double calculate_rsi() const {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        double avg_gain = (previous_avarage_gain * ( period-1) + current_gain)/period;
        double avg_loss = (previous_avarage_loss * ( period-1) + current_loss)/period;

        double rs = avg_loss == 0 ? 100 : avg_gain / avg_loss;
        double rsi = 100 - (100 / (1 + rs));
        return rsi;

    }*/

    double calculate_dema() const {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        QVector<double> prices;
        for (const auto& ohlc : container) {
            prices.append(ohlc.get_average_all_price());
        }

        double ema1 = calculate_ema(prices, period);
        QVector<double> ema_prices;
        for (int i = period - 1; i < prices.size(); ++i) {
            ema_prices.append(calculate_ema(prices.mid(i - period + 1, period), period));
        }

        double ema2 = calculate_ema(ema_prices, period);

        return 2 * ema1 - ema2;
    }

    double calculate_ema(const QVector<double>& prices, size_t period) const {
        if (prices.isEmpty() || period > prices.size()) {
            return 0.0;
        }

        double multiplier = 2.0 / (period + 1);
        double ema = prices[0];

        for (int i = 1; i < prices.size(); ++i) {
            ema = ((prices[i] - ema) * multiplier) + ema;
        }

        return ema;
    }

    /*
    double calculate_slope( ) const{
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        double y1 = container.first().get_average_all_price();
        double y2 = container.last().get_average_all_price();
        double x1 = 0.0;
        double x2 = static_cast<double>(container.size() - 1);

        double slope = (y2 - y1) / (x2 - x1);

       return slope;
    }*/

    double calculate_slope(int slope_size = 0) const {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        if(slope_size > 0 && slope_size < container.size() )
        {
            // container'ın son "slope_size" kadarlık elemanının eğimini hesapla
            double y1 = container[container.size() - slope_size].get_average_all_price();
            double y2 = container.back().get_average_all_price();
            double x1 = static_cast<double>(container.size() - slope_size);
            double x2 = static_cast<double>(container.size() - 1);

            std::vector<double> prices;
            for (auto it = container.end() - slope_size; it != container.end(); ++it) {
                prices.push_back(it->get_average_all_price());
            }
            double min_price = *std::min_element(prices.begin(), prices.end());
            double max_price = *std::max_element(prices.begin(), prices.end());

            double norm_y1 = (y1 - min_price) / (max_price - min_price);
            double norm_y2 = (y2 - min_price) / (max_price - min_price);

            double norm_x1 = (x1 - x1) / (x2 - x1);
            double norm_x2 = (x2 - x1) / (x2 - x1);

            double slope = (norm_y2 - norm_y1) / (norm_x2 - norm_x1);

            double angle_radians = std::atan(slope);
            double angle_degrees = angle_radians * (180.0 / M_PI); // Radyanları dereceye çevirme
            return angle_degrees;
        }
        else{
            // burada containerın içindeki max price ve min price bulunarak eğim hesaplanır aşağıdaki kod doğru çalışır
            double y1 = container.first().get_average_all_price();
            double y2 = container.last().get_average_all_price();
            double x1 = 0.0;
            //double x2 = static_cast<double>(container.size() - 1);

            std::vector<double> prices;
            for (const auto& item : container) {
                prices.push_back(item.get_average_all_price());
            }
            double min_price = *std::min_element(prices.begin(), prices.end());
            double max_price = *std::max_element(prices.begin(), prices.end());

            double norm_y1 = (y1 - min_price) / (max_price - min_price);
            double norm_y2 = (y2 - min_price) / (max_price - min_price);

            double norm_x1 = 0.0;
            double norm_x2 = 1.0;

            double slope = (norm_y2 - norm_y1) / (norm_x2 - norm_x1);

            double angle_radians = std::atan(slope);
            double angle_degrees = angle_radians * (180.0 / M_PI); // Radyanları dereceye çevirme
            return angle_degrees;
        }
        return 0;
    }


    void calculate_max_min_in_period() {
        if (is_empty()) {
            return;
        }

        double max_price = container.first().high;
        double min_price = container.first().low;

        for (const auto& ohlc : container) {
            if (ohlc.high > max_price) {
                max_price = ohlc.high;
            }
            if (ohlc.low < min_price) {
                min_price = ohlc.low;
            }
        }

        last_max_price = max_price;
        last_min_price = min_price;
    }

    double calculate_mode_average(int number_of_bins = 10) const {
        if (is_empty()) {
            return 0.0;
        }

        double min_price = last_min_price; // it must be found before
        double max_price = last_max_price; // it must be found before


        double price_range = max_price - min_price;
        double range_size = price_range / number_of_bins;

        QMap<double, int> frequency_map;

        for (const auto& ohlc : container) {
            double price_range = std::floor(ohlc.get_average_all_price() / range_size) * range_size;
            frequency_map[price_range]++;
        }

        int max_frequency = 0;
        double most_frequent_range = 0.0;
        for (auto it = frequency_map.constBegin(); it != frequency_map.constEnd(); ++it) {
            if (it.value() > max_frequency) {
                max_frequency = it.value();
                most_frequent_range = it.key();
            }
        }

        double sum = 0.0;
        int count = 0;
        for (const auto& ohlc : container) {
            double price_range = std::floor(ohlc.get_average_all_price() / range_size) * range_size;
            if (price_range == most_frequent_range) {
                sum += ohlc.get_average_all_price();
                count++;
            }
        }

        return count > 0 ? sum / count : 0.0;
    }


    double calculate_gain(bool is_avarage_active) const {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        QVector<double> gains;

        // Günlük değişiklikleri hesapla ve kazançları toplamak için gains dizisine ekle
        for (int i = 1; i < container.size(); ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change > 0) {
                gains.append(change);
            } else {
                gains.append(0); // Sadece pozitif değişiklikleri ekliyoruz, negatifler 0 olarak ekleniyor
            }
        }

        // Kazançların toplamını hesapla ve dönem sayısına bölerek ortalama kazancı bul
        double total_gain = std::accumulate(gains.end() - period, gains.end(), 0.0);

        double _gain = total_gain  ;
        if(is_avarage_active == true)
            _gain = _gain / period;

        return _gain;
    }

    double calculate_loss( bool is_avarage_active) const {
        if (is_empty() || period > container.size()) {
            return 0.0;
        }

        QVector<double> losses;

        // Günlük değişiklikleri hesapla ve kayıpları toplamak için losses dizisine ekle
        for (int i = 1; i < container.size(); ++i) {
            double change = container[i].get_average_mid_point() - container[i - 1].get_average_mid_point();
            if (change < 0) {
                losses.append(-change);  // Negatif değişiklikleri pozitif yaparak ekliyoruz
            } else {
                losses.append(0);  // Pozitif değişiklikler 0 olarak ekleniyor
            }
        }

        // Kayıpların toplamını hesapla ve dönem sayısına bölerek ortalama kaybı bul
        double total_loss = std::accumulate(losses.end() - period, losses.end(), 0.0);

        double  _loss = total_loss;

        if(is_avarage_active == true)
            _loss=   total_loss / period;

        return  _loss;
    }


    // params..

    double       previous_avarage_gain = 0;
    double       current_avarage_gain = 0;
    double       current_gain = 0;
    double       previous_avarage_loss = 0;
    double       current_avarage_loss = 0;
    double       current_loss = 0;

    double       last_standard_deviation = 0;

    double       last_rsi = 0;
    double       last_moving_avarage = 0;
    double       last_slope = 0;  // angle
    double       last_dema = 0;
    double       last_max_price = 0;
    double       last_min_price = 0;
    double       last_mode_avarage = 0; // fiyatın yoğun olduğu alanın ortalaması , destek veya direnç olabilir

    size_t       period = 25;
    QVector<OHLC_s> container;
};





// math outsource .....

inline double calculate_EMA(const QVector<double>& prices, int period, int start_index) {
    double k = 2.0 / (period + 1);
    double ema = prices[start_index];
    for (int i = start_index + 1; i < prices.size(); ++i) {
        ema = prices[i] * k + ema * (1 - k);
    }
    return ema;
}




inline void calculate_MACD(const QVector<double>& prices, QVector<double>& macd, QVector<double>& signal, int short_period, int long_period, int signal_period) {
    QVector<double> shortEMA;
    QVector<double> longEMA;

    for (int i = 0; i < prices.size(); ++i) {
        if (i >= long_period - 1) {
            shortEMA.append(calculate_EMA(prices, short_period, i - short_period + 1));
            longEMA.append(calculate_EMA(prices, long_period, i - long_period + 1));
            macd.append(shortEMA.last() - longEMA.last());
        } else {
            macd.append(0);
        }
    }

    for (int i = 0; i < macd.size(); ++i) {
        if (i >= signal_period - 1) {
            signal.append(calculate_EMA(macd, signal_period, i - signal_period + 1));
        } else {
            signal.append(0);
        }
    }
}

















struct OHLC_core_creator{
    void async_data_update(double data){  // it shold be call fast than 1 second
        ll_container.push_back(data);
    }
    /** you can call this every organised 1 second **/
    OHLC_s create_ohlc() {
        if (ll_container.empty()) {
            return OHLC_s{};
        }
        OHLC_s ohlc;
        ohlc.open = ll_container.front();
        ohlc.high = *std::max_element(ll_container.begin(), ll_container.end());
        ohlc.low = *std::min_element(ll_container.begin(), ll_container.end());
        ohlc.close = ll_container.back();
        ll_container.clear();
        return ohlc;
    }
private:
    std::vector<double> ll_container;
};









#endif
