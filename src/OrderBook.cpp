#include "OrderBook.h"
#include <algorithm>
#include <iostream>

namespace apextrade {

void OrderBook::addOrder(std::shared_ptr<Order> order) {
    orders_[order->id] = order;
    
    // Intentar emparejar (cruzar) la orden entrante con el libro existente
    matchOrder(order);
    
    // Si la orden aún tiene cantidad restante, se agrega al libro
    if (order->remainingQuantity > 0) {
        if (order->side == Side::Buy) {
            bids_[order->price].push_back(order);
        } else {
            asks_[order->price].push_back(order);
        }
    } else {
        // Si se llenó completamente, no necesitamos trackearla en O(1)
        orders_.erase(order->id);
    }
}

void OrderBook::cancelOrder(OrderId id) {
    auto it = orders_.find(id);
    if (it != orders_.end()) {
        auto order = it->second;
        order->remainingQuantity = 0; // Marcar como cancelada (lazy deletion para rendimiento)
        orders_.erase(it);
    }
}

void OrderBook::matchOrder(std::shared_ptr<Order> order) {
    if (order->side == Side::Buy) {
        // Emparejar una orden de compra con las mejores ofertas de venta (Asks)
        while (order->remainingQuantity > 0 && !asks_.empty()) {
            auto bestAskIt = asks_.begin();
            Price bestAskPrice = bestAskIt->first;
            
            // Si el precio más barato de venta es mayor que mi límite de compra, paramos
            if (bestAskPrice > order->price) {
                break;
            }

            auto& queue = bestAskIt->second;
            while (order->remainingQuantity > 0 && !queue.empty()) {
                auto askOrder = queue.front();
                
                // Si la orden en el libro fue cancelada previamente
                if (askOrder->remainingQuantity == 0) {
                    queue.erase(queue.begin());
                    continue;
                }

                Quantity tradeQty = std::min(order->remainingQuantity, askOrder->remainingQuantity);
                
                // Crear operación (Trade)
                auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                trades_.push_back({
                    order->id, askOrder->id, bestAskPrice, tradeQty, static_cast<uint64_t>(now)
                });

                // Actualizar cantidades
                order->remainingQuantity -= tradeQty;
                askOrder->remainingQuantity -= tradeQty;

                if (askOrder->remainingQuantity == 0) {
                    // Remover del índice O(1) y de la cola
                    orders_.erase(askOrder->id);
                    queue.erase(queue.begin());
                }
            }

            // Si el nivel de precio se vació, eliminar el nivel
            if (queue.empty()) {
                asks_.erase(bestAskIt);
            }
        }
    } else {
        // Emparejar una orden de venta con las mejores ofertas de compra (Bids)
        while (order->remainingQuantity > 0 && !bids_.empty()) {
            auto bestBidIt = bids_.begin();
            Price bestBidPrice = bestBidIt->first;
            
            // Si el comprador ofrece menos de lo que pido, paramos
            if (bestBidPrice < order->price) {
                break;
            }

            auto& queue = bestBidIt->second;
            while (order->remainingQuantity > 0 && !queue.empty()) {
                auto bidOrder = queue.front();
                
                if (bidOrder->remainingQuantity == 0) {
                    queue.erase(queue.begin());
                    continue;
                }

                Quantity tradeQty = std::min(order->remainingQuantity, bidOrder->remainingQuantity);
                
                auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                // El precio de ejecución siempre es el que ya estaba en el libro
                trades_.push_back({
                    bidOrder->id, order->id, bestBidPrice, tradeQty, static_cast<uint64_t>(now)
                });

                order->remainingQuantity -= tradeQty;
                bidOrder->remainingQuantity -= tradeQty;

                if (bidOrder->remainingQuantity == 0) {
                    orders_.erase(bidOrder->id);
                    queue.erase(queue.begin());
                }
            }

            if (queue.empty()) {
                bids_.erase(bestBidIt);
            }
        }
    }
}

} // namespace apextrade
