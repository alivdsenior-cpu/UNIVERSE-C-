#include "OrderBook.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>

using namespace apextrade;
using namespace std::chrono;

uint64_t getCurrentTime() {
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int main() {
    // Para que Python lea el stream sin buffer
    std::setvbuf(stdout, NULL, _IONBF, 0);

    OrderBook ob;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_int_distribution<> priceDist(9500, 10500); // Rango de precio $95.00 a $105.00
    std::uniform_int_distribution<> qtyDist(1, 50);
    std::uniform_int_distribution<> delayDist(50, 500); // ms entre ráfagas

    uint64_t orderIdCounter = 1;

    // Bucle infinito para simular el mercado en vivo
    while (true) {
        // Generar una ráfaga de 1 a 10 órdenes
        std::uniform_int_distribution<> burstDist(1, 10);
        int burst = burstDist(gen);

        for (int i = 0; i < burst; ++i) {
            Side side = (sideDist(gen) == 0) ? Side::Buy : Side::Sell;
            Price price = priceDist(gen);
            Quantity qty = qtyDist(gen);

            auto order = std::make_shared<Order>(
                orderIdCounter++, side, OrderType::Limit, price, qty, getCurrentTime()
            );

            // Al añadir la orden, el OrderBook internamente generará Trades si hay match
            ob.addOrder(order);

            // Salida JSON para el backend Python
            std::string sideStr = (side == Side::Buy) ? "BUY" : "SELL";
            std::cout << "{\"type\":\"order\", \"id\":" << order->id 
                      << ", \"side\":\"" << sideStr 
                      << "\", \"price\":" << price 
                      << ", \"quantity\":" << qty << "}\n";
        }

        // Obtener operaciones recientes e imprimirlas
        const auto& trades = ob.getTrades();
        // Para no volver a imprimir los trades antiguos, deberíamos limpiar o trackear el índice.
        // Como simplificación para la visualización, solo vamos a leer el tamaño.
        // Una mejora sería modificar OrderBook para que extraiga los trades recientes.
        // Pero por ahora, simularemos emitir un log genérico de estado si es necesario, 
        // o mejor, que OrderBook expulse los eventos de trade al momento.
        // Como no queremos cambiar OrderBook.cpp, trackeamos el tamaño aquí.
        static size_t lastTradeCount = 0;
        if (trades.size() > lastTradeCount) {
            for (size_t i = lastTradeCount; i < trades.size(); ++i) {
                const auto& t = trades[i];
                std::cout << "{\"type\":\"trade\", \"buyer\":" << t.buyerOrderId 
                          << ", \"seller\":" << t.sellerOrderId 
                          << ", \"price\":" << t.price 
                          << ", \"quantity\":" << t.quantity << "}\n";
            }
            lastTradeCount = trades.size();
        }

        // Simular tiempo real
        std::this_thread::sleep_for(std::chrono::milliseconds(delayDist(gen)));
    }

    return 0;
}
