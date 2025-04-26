#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>
#include <cmath>
#include <thread>
#include <mutex>
#include <unordered_map>

struct MarketData {
    std::string date;
    std::string symbol;
    double close_price;
};

std::mutex cout_mutex;

// Load CSV with multiple symbols
std::unordered_map<std::string, std::vector<MarketData>> loadCSV(const std::string& filename) {
    std::unordered_map<std::string, std::vector<MarketData>> data_by_symbol;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open file " + filename);
    }

    std::string line;
    std::getline(file, line); // Skip header

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string date, symbol, price_str;
        if (std::getline(ss, date, ',') && std::getline(ss, symbol, ',') && std::getline(ss, price_str, ',')) {
            try {
                double price = std::stod(price_str);
                data_by_symbol[symbol].push_back({date, symbol, price});
            } catch (...) {
                continue; // Skip invalid lines
            }
        }
    }
    return data_by_symbol;
}

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

double backtestSMA(const std::vector<MarketData>& data, int short_window, int long_window) {
    std::vector<double> prices;
    for (const auto& entry : data) {
        prices.push_back(entry.close_price);
    }

    auto short_sma = calculateSMA(prices, short_window);
    auto long_sma = calculateSMA(prices, long_window);

    std::cout << "short_sma size: " << short_sma.size() << "\n";
    std::cout << "long_sma size: " << long_sma.size() << "\n";
    std::cout << "prices size: " << prices.size() << "\n";

    double initial_balance = 10000; // Initial capital
    double balance = initial_balance;
    double position = 0; // Shares held

    size_t start = std::max(short_window, long_window) - 1;

    for (size_t i = start; i < prices.size(); ++i) {
        size_t short_idx = i - (short_window - 1);
        size_t long_idx = i - (long_window - 1);

        if (short_idx >= short_sma.size() || long_idx >= long_sma.size()) continue;

        double short_val = short_sma[short_idx];
        double long_val = long_sma[long_idx];
        double price = prices[i];

        std::cout << "Date: " << data[i].date
                  << " | Price: " << price
                  << " | Short SMA: " << short_val
                  << " | Long SMA: " << long_val << "\n";

        if (short_val > long_val && position == 0) {
            // Buy
            std::cout << "  → Buy Signal at " << price << "\n";
            position = balance / price;
            balance = 0;
        } else if (short_val < long_val && position > 0) {
            // Sell
            std::cout << "  → Sell Signal at " << price << "\n";
            balance += position * price;
            position = 0;
        }
    }

    // Final sell if holding a position
    balance += position * prices.back();
    double roi = ((balance - initial_balance) / initial_balance) * 100;
    return roi;
}


// Threaded backtest for a specific symbol
void runBacktest(const std::string& symbol, const std::vector<MarketData>& data, int short_window, int long_window) {
    double roi = backtestSMA(data, short_window, long_window);
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[Symbol=" << symbol << ", Short=" << short_window << ", Long=" << long_window
              << "] ROI: " << roi << "%\n";
}

int main() {
    try {
        auto data_by_symbol = loadCSV("data.csv");

        int short_window = 3;
        int long_window = 5;

        std::vector<std::thread> threads;

        for (const auto& [symbol, data] : data_by_symbol) {
            threads.emplace_back(runBacktest, symbol, data, short_window, long_window);
        }

        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
