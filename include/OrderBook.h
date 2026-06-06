#pragma once

#include "Order.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

namespace apextrade {

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;

    // Añade una orden al libro. Internamente realiza el emparejamiento.
    void addOrder(std::shared_ptr<Order> order);
    
    // Cancela una orden existente
    void cancelOrder(OrderId id);

    // Retorna las operaciones realizadas (trades)
    const std::vector<Trade>& getTrades() const { return trades_; }

private:
    void matchOrder(std::shared_ptr<Order> order);

    // Mapa Hash para acceso O(1) a cualquier orden mediante su ID
    std::unordered_map<OrderId, std::shared_ptr<Order>> orders_;

    // Niveles de Precio (Price Levels): Precio -> Lista de Órdenes (Prioridad de Tiempo)
    // Compras (Bids): Ordenadas de mayor a menor precio (mayor disposición a pagar primero)
    std::map<Price, std::vector<std::shared_ptr<Order>>, std::greater<Price>> bids_;
    
    // Ventas (Asks): Ordenadas de menor a mayor precio (el más barato primero)
    std::map<Price, std::vector<std::shared_ptr<Order>>, std::less<Price>> asks_;

    // Registro de operaciones completadas
    std::vector<Trade> trades_;
};

} // namespace apextrade
