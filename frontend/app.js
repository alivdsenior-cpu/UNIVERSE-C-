document.addEventListener('DOMContentLoaded', () => {
    const wsStatus = document.getElementById('ws-status');
    const statusPill = document.querySelector('.status-pill');
    
    // Contenedores del DOM
    const asksContainer = document.getElementById('asks-container');
    const bidsContainer = document.getElementById('bids-container');
    const tradesContainer = document.getElementById('trades-container');
    
    const marketPriceEl = document.getElementById('current-market-price');
    const spreadValueEl = document.getElementById('spread-value');
    const totalVolEl = document.getElementById('total-vol');
    const totalTradesEl = document.getElementById('total-trades');
    
    // Estado del motor en el frontend
    let orderBook = {
        asks: new Map(), // price -> qty
        bids: new Map()
    };
    
    let stats = {
        vol: 0,
        trades: 0,
        lastPrice: 100.00
    };

    const maxRows = 12; // Número de niveles a mostrar en el order book

    // Conectar WebSocket al backend de Python
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const ws = new WebSocket(`${protocol}//${window.location.host}/ws`);

    ws.onopen = () => {
        wsStatus.textContent = "Motor C++ Conectado";
        statusPill.classList.add('active');
        asksContainer.innerHTML = '';
        bidsContainer.innerHTML = '';
    };

    ws.onclose = () => {
        wsStatus.textContent = "Desconectado";
        statusPill.classList.remove('active');
    };

    ws.onmessage = (event) => {
        try {
            const data = JSON.parse(event.data);
            
            if (data.type === 'order') {
                handleOrder(data);
            } else if (data.type === 'trade') {
                handleTrade(data);
            }
        } catch (e) {
            // Ignorar errores de parseo (mensajes de log del C++)
        }
    };

    // Para evitar redibujar el DOM miles de veces por segundo, usamos requestAnimationFrame
    let updatePending = false;

    function handleOrder(order) {
        // Formato: { type: 'order', id: 1, side: 'BUY'/'SELL', price: 10000, quantity: 50 }
        const price = order.price / 100.0;
        
        // Simulación: Añadir órdenes al libro visual (en un entorno real el C++ envía instantáneas)
        if (order.side === 'BUY') {
            const currentQty = orderBook.bids.get(price) || 0;
            orderBook.bids.set(price, currentQty + order.quantity);
        } else {
            const currentQty = orderBook.asks.get(price) || 0;
            orderBook.asks.set(price, currentQty + order.quantity);
        }
        
        scheduleUIUpdate();
    }

    function handleTrade(trade) {
        // Formato: { type: 'trade', buyer: 1, seller: 2, price: 10000, quantity: 50 }
        stats.trades++;
        stats.vol += trade.quantity;
        
        totalTradesEl.textContent = stats.trades.toLocaleString();
        totalVolEl.textContent = stats.vol.toLocaleString();

        const price = trade.price / 100.0;
        
        // Actualizar precio actual de mercado con indicador de dirección
        if (price > stats.lastPrice) {
            marketPriceEl.className = 'market-price up';
        } else if (price < stats.lastPrice) {
            marketPriceEl.className = 'market-price down';
        }
        marketPriceEl.textContent = price.toFixed(2);
        stats.lastPrice = price;

        // Añadir a la lista de últimos trades
        // Determinamos visualmente si fue una compra o venta asumiendo convención común
        const side = trade.buyer > trade.seller ? 'BUY' : 'SELL';
        addTradeToUI(price, trade.quantity, side);
        
        // Limpiar liquidez consumida del libro de órdenes visual
        if (side === 'BUY') {
            const currentQty = orderBook.asks.get(price);
            if (currentQty) orderBook.asks.set(price, Math.max(0, currentQty - trade.quantity));
        } else {
            const currentQty = orderBook.bids.get(price);
            if (currentQty) orderBook.bids.set(price, Math.max(0, currentQty - trade.quantity));
        }

        scheduleUIUpdate();
    }

    function scheduleUIUpdate() {
        if (!updatePending) {
            updatePending = true;
            requestAnimationFrame(() => {
                updateOrderBookUI();
                updatePending = false;
            });
        }
    }

    function updateOrderBookUI() {
        // Ordenar y limitar niveles
        const asks = Array.from(orderBook.asks.entries())
            .filter(a => a[1] > 0)
            .sort((a, b) => a[0] - b[0])
            .slice(0, maxRows);
            
        const bids = Array.from(orderBook.bids.entries())
            .filter(b => b[1] > 0)
            .sort((a, b) => b[0] - a[0])
            .slice(0, maxRows);
        
        // Calcular volumen total para las barras de profundidad (Depth chart background)
        const maxAskVol = asks.reduce((acc, curr) => acc + curr[1], 0);
        const maxBidVol = bids.reduce((acc, curr) => acc + curr[1], 0);
        const maxVol = Math.max(maxAskVol, maxBidVol) || 1;

        // Render Asks (Ventas)
        let asksHtml = '';
        let currentAskAcc = 0;
        // Se muestran con el precio mayor arriba
        for (let i = asks.length - 1; i >= 0; i--) {
            const [price, qty] = asks[i];
            currentAskAcc += qty;
            const depthPct = (currentAskAcc / maxVol) * 100;
            
            asksHtml += `
                <div class="ob-row">
                    <div class="depth-bar depth-ask" style="width: ${depthPct}%"></div>
                    <span class="ask-price">${price.toFixed(2)}</span>
                    <span>${qty}</span>
                    <span>${currentAskAcc}</span>
                </div>
            `;
        }
        asksContainer.innerHTML = asksHtml;

        // Render Bids (Compras)
        let bidsHtml = '';
        let currentBidAcc = 0;
        for (let i = 0; i < bids.length; i++) {
            const [price, qty] = bids[i];
            currentBidAcc += qty;
            const depthPct = (currentBidAcc / maxVol) * 100;
            
            bidsHtml += `
                <div class="ob-row">
                    <div class="depth-bar depth-bid" style="width: ${depthPct}%"></div>
                    <span class="bid-price">${price.toFixed(2)}</span>
                    <span>${qty}</span>
                    <span>${currentBidAcc}</span>
                </div>
            `;
        }
        bidsContainer.innerHTML = bidsHtml;

        // Actualizar Indicador de Spread
        if (asks.length > 0 && bids.length > 0) {
            const spread = asks[0][0] - bids[0][0];
            spreadValueEl.textContent = `Spread: ${spread.toFixed(2)}`;
        } else {
            spreadValueEl.textContent = `Spread: --`;
        }
    }

    function addTradeToUI(price, qty, side) {
        const time = new Date().toLocaleTimeString('en-US', { hour12: false, hour: "2-digit", minute: "2-digit", second: "2-digit", fractionalSecondDigits: 3 });
        const isBuy = side === 'BUY';
        
        const row = document.createElement('div');
        row.className = `tr-row ${isBuy ? 'flash-up' : 'flash-down'}`;
        
        row.innerHTML = `
            <span>${time}</span>
            <span class="${isBuy ? 'bid-price' : 'ask-price'}">${price.toFixed(2)}</span>
            <span>${qty}</span>
        `;
        
        tradesContainer.prepend(row);
        
        // Mantener un historial corto para que el DOM no explote
        if (tradesContainer.children.length > 30) {
            tradesContainer.removeChild(tradesContainer.lastChild);
        }
    }
});
