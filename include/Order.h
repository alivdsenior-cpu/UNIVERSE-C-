#pragma once

#include <cstdint>
#include <chrono>

namespace apextrade {

enum class Side : uint8_t {
    Buy,
    Sell
};

enum class OrderType : uint8_t {
    Limit,
    Market
};

using OrderId = uint64_t;
using Price = uint64_t; // Represented in ticks (e.g., 100.50 -> 10050)
using Quantity = uint32_t;

struct Order {
    OrderId id;
    Side side;
    OrderType type;
    Price price;
    Quantity initialQuantity;
    Quantity remainingQuantity;
    uint64_t timestamp;

    Order(OrderId id, Side side, OrderType type, Price price, Quantity qty, uint64_t ts)
        : id(id), side(side), type(type), price(price), 
          initialQuantity(qty), remainingQuantity(qty), timestamp(ts) {}
};

struct Trade {
    OrderId buyerOrderId;
    OrderId sellerOrderId;
    Price price;
    Quantity quantity;
    uint64_t timestamp;
};

} // namespace apextrade
