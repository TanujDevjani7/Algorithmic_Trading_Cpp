#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>
#include <cmath>

// Structure to store market data
struct MarketData {
    std::string date;
    double close_price;
};

// Function to load CSV data
std::vector<MarketData> loadCSV(const std::string& filename) {
    std::vector<MarketData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open file " + filename);
    }

    std::string line, date;
    double close_price;
    // Skip the header
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        if (std::getline(ss, date, ',') && ss >> close_price) {
            data.push_back({date, close_price});
        }
    }
    file.close();
    return data;
}

// Calculate Simple Moving Average (SMA)
std::vector<double> calculateSMA(const std::vector<double>& prices, int window_size) {
    std::vector<double> sma;
    double sum = 0;
    for (size_t i = 0; i < prices.size(); ++i) {
        sum += prices[i];
        if (i >= window_size) {
            sum -= prices[i - window_size];
        }
        if (i >= window_size - 1) {
            sma.push_back(sum / window_size);
        }
    }
    return sma;
}

// Backtest SMA Strategy
double backtestSMA(const std::vector<MarketData>& data, int short_window, int long_window) {
    std::vector<double> prices;
    for (const auto& entry : data) {
        prices.push_back(entry.close_price);
    }

    auto short_sma = calculateSMA(prices, short_window);
    auto long_sma = calculateSMA(prices, long_window);

    double initial_balance = 10000; // Initial capital in USD
    double balance = initial_balance;
    double position = 0; // Number of shares held

    for (size_t i = long_window - 1; i < prices.size(); ++i) {
        if (short_sma[i - (long_window - 1)] > long_sma[i - (long_window - 1)] && position == 0) {
            // Buy signal
            position = balance / prices[i];
            balance = 0;
        } else if (short_sma[i - (long_window - 1)] < long_sma[i - (long_window - 1)] && position > 0) {
            // Sell signal
            balance += position * prices[i];
            position = 0;
        }
    }

    // Final balance after selling any remaining position
    balance += position * prices.back();
    double roi = ((balance - initial_balance) / initial_balance) * 100;
    return roi;
}

int main() {
    try {
        // Load historical data
        auto data = loadCSV("data.csv");

        // Parameters for the strategy
        int short_window = 10;
        int long_window = 50;

        // Backtest the strategy
        double roi = backtestSMA(data, short_window, long_window);

        // Display results
        std::cout << "SMA Strategy Backtest Results:\n";
        std::cout << "Short Window: " << short_window << " days\n";
        std::cout << "Long Window: " << long_window << " days\n";
        std::cout << "Return on Investment (ROI): " << roi << "%\n";
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
