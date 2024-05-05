
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

#include <endian.h>
#include <byteswap.h>
#include <memory>
#include <map>
#include <bitset>
#include <cstddef>
#include <bit>
#include <optional>
#include <climits>
#include <numeric>
#include <chrono>

using namespace std;

std::ostream &operator<<(std::ostream &os, std::byte b)
{
    return os << std::bitset<8>(std::to_integer<int>(b));
}

struct Best
{
    int32_t price;
    int32_t qty;
    int32_t ordersCount;
};

struct Tick
{
    int64_t ts;
    uint8_t side;
    char action;
    int64_t orderId;
    int32_t price;
    int32_t qty;
    Best bestBid;
    Best bestAsk;
};

struct Order
{
    int64_t id;
    int32_t price;
    int32_t qty;
};

vector<Tick> readTicks()
{

    string line;
    ifstream file("../data/ticks.raw", std::ios::binary);

    int i = 0;
    int64_t value;
    int32_t val2;

    // 8 1 1 8 4 4
    short line_size = sizeof(int64_t) * 3 + sizeof(int8_t) * 2;
    char buf[line_size];

    vector<Tick> ticks;

    while (file.read(buf, line_size))
    {
        Tick tick;
        tick.bestAsk.qty = -1;
        tick.bestBid.qty = -1;

        memcpy(&value, buf, sizeof(value));
        // TODO: Swap only if endian diffs
        // #if __BYTE_ORDER == __LITTLE_ENDIAN
        tick.ts = bswap_64(value);
        // #endif

        tick.side = buf[8];
        tick.action = buf[9];

        memcpy(&value, buf + 10, sizeof(value));

        tick.orderId = bswap_64(value);

        memcpy(&val2, buf + 18, sizeof(val2));
        tick.price = bswap_32(val2);

        memcpy(&val2, buf + 22, sizeof(val2));
        tick.qty = bswap_32(val2);

        ticks.push_back(tick);

        i++;
    }

    return ticks;
}

void saveTicks(vector<Tick> ticks)
{
    ofstream myfile;
    myfile.open("../data/rad_result.csv");
    myfile << "SourceTime;Side;Action;OrderId;Price;Qty;B0;BQ0;BN0;A0;AQ0;AN0\n";
    for (auto &&tick : ticks)
    {
        bool hasSide = tick.side == '1' || tick.side == '2';
        myfile << tick.ts << ";";
        if (hasSide)
        {
            myfile << tick.side;
        }
        myfile << ";" << tick.action << ";";
        myfile << tick.orderId << ";" << tick.price << ";" << tick.qty << ";";
        if (tick.bestBid.qty > 0)
        {
            myfile << tick.bestBid.price << ";" << tick.bestBid.qty << ";" << tick.bestBid.ordersCount << ";";
        }
        else
            myfile << ";;;";
        if (tick.bestAsk.qty > 0)
        {
            myfile << tick.bestAsk.price << ";" << tick.bestAsk.qty << ";" << tick.bestAsk.ordersCount;
        }
        else
            myfile << ";;";
        myfile << endl;
    }
    myfile.close();
}

class OrderBook
{
    // TODO: Adjust access specifiers
public:
    optional<Best> bestBid,
        bestAsk;

    map<int64_t, map<int64_t, Order>> bidOrders, askOrders;

    void clear()
    {
        bestBid = nullopt;
        bestAsk = nullopt;
        bidOrders.clear();
        askOrders.clear();
    }

    void remove(Tick *tick,
                map<int64_t, map<int64_t, Order>> *orders,
                optional<Best> *best)
    {

        if (!orders->contains(tick->price) || !orders->at(tick->price).contains(tick->orderId))
        {
            return;
        }

        int32_t oldQty = orders->at(tick->price).at(tick->orderId).qty;

        orders->at(tick->price).erase(tick->orderId);
        if (orders->at(tick->price).empty())
        {
            orders->erase(tick->price);

            this->updateBest(tick->side == '1', best, *orders);
            return;
        }

        if (tick->price == best->value().price)
        {

            best->value().qty -= oldQty;
            best->value().ordersCount--;
        }
    }

    void update(Tick *tick,
                map<int64_t, map<int64_t, Order>> *orders,
                optional<Best> *best)
    {

        if (!orders->contains(tick->price) || !orders->at(tick->price).contains(tick->orderId))
        {
            this->add(tick, orders, best);
            return;
        }

        if (tick->price == best->value().price)
        {
            int32_t diff = orders->at(tick->price)[tick->orderId].qty - tick->qty;
            best->value().qty -= diff;
            orders->at(tick->price)[tick->orderId].qty = tick->qty;
        }
    }

    void add(Tick *tick,
             map<int64_t, map<int64_t, Order>> *orders,
             optional<Best> *best)
    {
        Order order = {
            tick->orderId, tick->price, tick->qty};

        if (orders->contains(order.price))
        {
            orders->at(order.price)[order.id] = order;
        }
        else
        {
            map<int64_t, Order> m = {{order.id, order}};
            orders->emplace(order.price, m);
        }

        if (!best->has_value() || ((tick->side == '1' && order.price > best->value().price)) || (tick->side == '2' && order.price < best->value().price))
        {
            Best b = {
                order.price,
                order.qty,
                1};
            best->emplace(b);

            return;
        }

        if (best->has_value() && order.price == best->value().price)
        {

            best->value().ordersCount++;
            best->value().qty += order.qty;
        }
    }

    void updateResult(Tick &tick)
    {
        if (bestBid.has_value())
        {
            tick.bestBid = bestBid.value();
        }
        if (bestAsk.has_value())
        {
            tick.bestAsk = bestAsk.value();
        }
    }

private:
    void updateBest(bool isBid, optional<Best> *best, map<int64_t, map<int64_t, Order>> &orders)
    {
        auto last = isBid ? (--orders.end()) : orders.begin();
        best->value().price = last->first;
        best->value().ordersCount = last->second.size();

        const int32_t result = std::accumulate(
            begin(orders[last->first]),
            end(orders[last->first]),
            0,
            [](const std::size_t prev, const std::pair<const int32_t, Order> &p)
            { return prev + p.second.qty; });

        [](const int32_t prev, const auto &element)
        { return prev + element.second; };

        best->value().qty = result;
    }
};

int main(int, char **)
{
    // Read
    vector<Tick> ticks = readTicks();

    using picoseconds = chrono::duration<long long, std::pico>;
    auto t0 = chrono::steady_clock::now();

    // Calculate
    OrderBook book;

    for (auto &&tick : ticks)
    {

        bool isBid = tick.side == '1';
        auto orders = &(isBid ? book.bidOrders : book.askOrders);
        auto best = &(isBid ? book.bestBid : book.bestAsk);

        switch (tick.action)
        {
        case 'Y':
        case 'F':
            book.clear();
            break;
        case 'A':
            book.add(&tick, orders, best);
            break;
        case 'M':
            book.update(&tick, orders, best);
            break;
        case 'D':
            book.remove(&tick, orders, best);
            break;
        }

        book.updateResult(tick);
    }

    auto t1 = std::chrono::steady_clock::now();
    auto d = picoseconds{t1 - t0} / 1e6; // pico to micro
    cout << d.count() << " microseconds" << endl;
    cout << d.count() / ticks.size() << " microseconds per tick" << endl;

    // Print info
    cout
        << "Best bid / best ask" << endl;
    cout << book.bestBid.value().price << " / "
         << book.bestAsk.value().price << endl;

    cout << book.bestBid.value().qty << " / " << book.bestAsk.value().qty << endl;
    cout << book.bestBid.value().ordersCount << " / " << book.bestAsk.value().ordersCount << endl;

    // Save
    saveTicks(ticks);
}
