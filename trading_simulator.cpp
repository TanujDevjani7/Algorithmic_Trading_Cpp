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

    if (short_window >= long_window || long_window >= prices.size()) {
        return 0.0;
    }

    auto short_sma = calculateSMA(prices, short_window);
    auto long_sma = calculateSMA(prices, long_window);

    double initial_balance = 10000;
    double balance = initial_balance;
    double position = 0;

    for (size_t i = long_window - 1; i < prices.size(); ++i) {
        if (short_sma[i - (long_window - 1)] > long_sma[i - (long_window - 1)] && position == 0) {
            position = balance / prices[i];
            balance = 0;
        } else if (short_sma[i - (long_window - 1)] < long_sma[i - (long_window - 1)] && position > 0) {
            balance += position * prices[i];
            position = 0;
        }
    }

    balance += position * prices.back();
    return ((balance - initial_balance) / initial_balance) * 100;
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

        int short_window = 5;
        int long_window = 10;

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
