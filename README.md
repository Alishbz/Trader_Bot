# Trader_Bot

### **Author:**  
**Ali Şahbaz**  
📧 **Email:** ali_sahbaz@outlook.com  

---

## **Description**  
**Trader_Bot** is a high-performance trade bot built with **C++ and Qt Framework 6.7.1**, specifically designed for cryptocurrency markets.  
This bot leverages advanced algorithms, multithreading, and real-time data analysis to execute trades based on user-defined strategies. With its customizable parameters and robust risk management tools, it serves as a powerful tool for both backtesting and live trading.

---

## **Features**
### **Core Functionalities**
- **Custom Graphics Processing:** High-performance visualization tools for monitoring and analysis.  
- **Trade / HFT Algorithm Integration:** Implements cutting-edge algorithms for high-frequency trading.  
- **Modern Software Patterns:** Employs advanced design patterns for maintainable and scalable development.  
- **Simplified Strategy Management:** Easily add and test new trading strategies with minimal effort.  
- **Custom Binance WebSocket Library:** Built for efficient real-time data retrieval and communication.  
- **Custom Binance REST API Implementation:** Provides seamless interaction with Binance endpoints.  
- **Real-Time Testing:** Direct testing of strategies with live market data.  
- **Backtesting Support:** Analyze historical data to optimize and validate strategies.  
- **Fake Account Testing:** Test strategies without risking actual funds.  
- **Custom OHLC Structures & Math Models:** Supports complex calculations and high-precision operations.  
- **20+ Pre-Built Strategies:** Includes templates (`template_strategy.h`) for creating your own strategies.  
- **Comprehensive Logging Support:** Provides detailed logs for debugging, analysis, and auditing.  
- **User-Friendly GUI:** Offers an intuitive interface for traders to easily manage and monitor their trading activities.

---

## **Screenshots**  

### **1. Binance Fake Account & Realtime Monitoring**  
![Fake Account Monitoring](photos/p1.png)  
This page emulates a Binance account. It supports:  
- Monitoring real-time coin data  
- Placing buy/sell orders  
- Integrating strategies  
- Viewing open orders, wallet balance, and logs  

### **2. History Testing Page**  
![History Testing](photos/p2.png)  
Provides an interface to visualize and analyze **RSI** and real-time data graphics. Features include:  
- Configurable visibility of data elements  
- Adjustable display settings  

### **3. Strategy Testing & Backtesting**  
![Backtesting](photos/p3.png)  
Allows users to:  
- Select a strategy and a time range for backtesting  
- Start testing and monitor data flow  
- Control data flow speed via the GUI  
- Log activities and set custom parameters for strategies  
- Select coins and specify candle types, supporting up to second-level granularity  

### **4. Visual Trade Analysis**  
![Trade Analysis](photos/p4.png)  
Displays **LONG** and **SHORT** orders directly on the graph. Features include:  
- Analyzing past wallet performance  
- Evaluating the strength of strategies through logs and graphical representations  

---

## **How to Use**  

1. Clone the repository:  
   ```bash
   git clone https://github.com/Alishbz/Trader_Bot.git
   cd Trader_Bot
