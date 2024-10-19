#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>
#include <ctime>
#include <algorithm>
#include <set>
#include <fstream> 


struct Order {
    std::string clientOrderId;  
    std::string orderId;        
    std::string instrument;
    int side; // 1: Buy, 2: Sell
    int quantity;
    double price;
};


struct ExecutionReport {
    std::string clientOrderId;
    std::string orderId;
    std::string instrument;
    int side;
    double price;
    int quantity;
    std::string status; // New, Rejected, Fill, Partial Fill
    std::string reason;
    std::string transactionTime;
};


std::string generateOrderId() {
    static int idCounter = 0;
    return "ORD" + std::to_string(++idCounter);
}


std::string getCurrentTime() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", std::localtime(&now));
    return std::string(buf);
}


bool validateOrder(const Order &order, std::string &reason) {
    std::set<std::string> validInstruments = {"Rose", "Lavender", "Lotus", "Tulip", "Orchid"};
    if (order.price <= 0) {
        reason = "Invalid Price";
        return false;
    }

    if (order.quantity % 10 != 0 || order.quantity < 10 || order.quantity > 1000) {
        reason = "Invalid Size";
        return false;
    }

    if (validInstruments.find(order.instrument) == validInstruments.end()) {
        reason = "Invalid Instrument";
        return false;
    }

    if (order.side != 1 && order.side != 2) {
        reason = "Invalid Side ";
        return false;
    }

    return true;
}


std::map<std::string, std::vector<Order>> buyOrders;
std::map<std::string, std::vector<Order>> sellOrders;


void insertBuyOrder(const std::string &instrument, const Order &newOrder) {
    auto &orders = buyOrders[instrument];
    auto it = std::upper_bound(orders.begin(), orders.end(), newOrder, [](const Order &newOrder, const Order &existingOrder) {
        return newOrder.price > existingOrder.price;  
    });
    orders.insert(it, newOrder);
}

void insertSellOrder(const std::string &instrument, const Order &newOrder) {
    auto &orders = sellOrders[instrument];
    auto it = std::upper_bound(orders.begin(), orders.end(), newOrder, [](const Order &newOrder, const Order &existingOrder) {
        return newOrder.price < existingOrder.price;  
    });
    orders.insert(it, newOrder);
}

void updateOrderBook(const Order &order) {
    if (order.side == 1) { // Buy side
        insertBuyOrder(order.instrument, order);
    } else if (order.side == 2) { // Sell side
        insertSellOrder(order.instrument, order);
    }
}

/*void displayOrderBook(const std::string &instrument) {
    const auto &buys = buyOrders[instrument];
    const auto &sells = sellOrders[instrument];

    size_t maxSize = std::max(buys.size(), sells.size());

    std::cout << "Order Book for " << instrument << ":\n";
    std::cout << "Buy Side                         | Sell Side\n";
    std::cout << "Order ID  Qty  Price              | Price  Qty  Order ID\n";

    for (size_t i = 0; i < maxSize; ++i) {
        // Display Buy side
        if (i < buys.size()) {
            std::cout << buys[i].orderId << "   " << buys[i].quantity << "   " << buys[i].price;
        } else {
            std::cout << "                            "; // Blank space for unmatched buys
        }

        std::cout << "     |     ";

        // Display Sell side
        if (i < sells.size()) {
            std::cout << sells[i].price << "   " << sells[i].quantity << "   " << sells[i].orderId;
        }

        std::cout << "\n";
    }
}*/


void processOrder(Order &order, std::vector<ExecutionReport> &reports) {
    ExecutionReport report;
    report.clientOrderId = order.clientOrderId;  
    report.orderId = order.orderId;  
    report.instrument = order.instrument;
    report.side = order.side;
    report.transactionTime = getCurrentTime();  
    report.price = order.price;  
    report.quantity = order.quantity;

    std::string reason;
    if (validateOrder(order, reason)) {
        if (order.side == 2) {  
            auto &buyOrdersForInstrument = buyOrders[order.instrument];
            bool matchFound = false;
            while (order.quantity > 0 && !buyOrdersForInstrument.empty()) {
                auto it = buyOrdersForInstrument.begin();
                if (it->price >= order.price) {  
                    int fillQuantity = std::min(order.quantity, it->quantity);

                    report.quantity = fillQuantity;
                    report.price = it->price;  
                    report.status = (fillQuantity == order.quantity) ? "Fill" : "PFill";
                    reports.push_back(report);

                    ExecutionReport matchedBuyReport;
                    matchedBuyReport.clientOrderId = it->clientOrderId;  
                    matchedBuyReport.orderId = it->orderId;              
                    matchedBuyReport.instrument = it->instrument;
                    matchedBuyReport.side = 1;  
                    matchedBuyReport.price = it->price;
                    matchedBuyReport.quantity = fillQuantity;
                    matchedBuyReport.status = (fillQuantity == it->quantity) ? "Fill" : "PFill";
                    matchedBuyReport.transactionTime = getCurrentTime();
                    reports.push_back(matchedBuyReport);
                    order.quantity -= fillQuantity;
                    it->quantity -= fillQuantity;

                    matchFound = true;  

                    if (it->quantity == 0) {
                        buyOrdersForInstrument.erase(it);
                    }
                } else {
                    break;  
                }
            }

            if (order.quantity > 0 && !matchFound) {
                report.status = "New";
                reports.push_back(report);
                insertSellOrder(order.instrument, order);
            } else if (order.quantity > 0) {
                insertSellOrder(order.instrument, order);
            }
        }
        else if (order.side == 1) {  
            auto &sellOrdersForInstrument = sellOrders[order.instrument];
            bool matchFound = false;
            while (order.quantity > 0 && !sellOrdersForInstrument.empty()) {
                auto it = sellOrdersForInstrument.begin();
                if (it->price <= order.price) {  
                    int fillQuantity = std::min(order.quantity, it->quantity);

                    report.quantity = fillQuantity;
                    report.price = it->price;  
                    report.status = (fillQuantity == order.quantity) ? "Fill" : "PFill";
                    reports.push_back(report);

                    ExecutionReport matchedSellReport;
                    matchedSellReport.clientOrderId = it->clientOrderId;  
                    matchedSellReport.orderId = it->orderId;              
                    matchedSellReport.instrument = it->instrument;
                    matchedSellReport.side = 2;  
                    matchedSellReport.price = it->price;
                    matchedSellReport.quantity = fillQuantity;
                    matchedSellReport.status = (fillQuantity == it->quantity) ? "Fill" : "PFill";
                    matchedSellReport.transactionTime = getCurrentTime();
                    reports.push_back(matchedSellReport);
                    order.quantity -= fillQuantity;
                    it->quantity -= fillQuantity;

                    matchFound = true;  

                    if (it->quantity == 0) {
                        sellOrdersForInstrument.erase(it);
                    }
                } else {
                    break;  
                }
            }

            if (order.quantity > 0 && !matchFound) {
                report.status = "New";
                reports.push_back(report);
                insertBuyOrder(order.instrument, order);
            } else if (order.quantity > 0) {
                insertBuyOrder(order.instrument, order);
            }
        }
    } else {
        report.status = "Rejected";
        report.reason = reason;
        reports.push_back(report);
    }
}

void trim(std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");  
    size_t end = s.find_last_not_of(" \t\r\n");

    if (start == std::string::npos) {
        s.clear();
    } else {
        s = s.substr(start, end - start + 1);
    }
}

std::string createFileName() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", std::localtime(&now));
    std::string timestamp(buf);
    return "execution_report_" + timestamp + ".csv";
}

std::vector<Order> readOrdersFromCsv(const std::string &filePath) {
    std::vector<Order> orders;
    std::ifstream file(filePath);
    std::string line;

    char bom[3];
    file.read(bom, 3);
    if (!(bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF')) {
        file.seekg(0);
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Order order;
        std::getline(ss, order.clientOrderId, ',');
        trim(order.clientOrderId);  
        std::getline(ss, order.instrument, ',');
        trim(order.instrument);  
        ss >> order.side;
        ss.ignore();  
        ss >> order.quantity;
        ss.ignore();  
        ss >> order.price;
        order.orderId = generateOrderId();  
        orders.push_back(order);
    }

    return orders;
}

void writeExecutionReportToCsv(const std::string &fileName, const std::vector<ExecutionReport> &reports) {
    std::ofstream file(fileName, std::ofstream::out);
    file << "OrderID,ClientOrderID,Instrument,Side,Status,Reason,Quantity,Price,TransactionTime\n";
    for (const auto &report : reports) {
        file << report.orderId << ","             
             << report.clientOrderId << ","       
             << report.instrument << ","          
             << report.side << ","                
             << report.status << ","              
             << report.reason << ","              
             << report.quantity << ","            
             << report.price << ","               
             << report.transactionTime << "\n";   
    }

    file.close();
    std::cout << "Execution report saved to " << fileName << std::endl;
}


int main() {
    std::string inputFilePath = "Book9.csv"; // Replace with actual input file path
    std::vector<Order> orders = readOrdersFromCsv(inputFilePath);
    std::vector<ExecutionReport> reports;
    for ( auto &order : orders) {
        processOrder(order, reports);
    }
    //displayOrderBook("Rose");
    std::string outputFileName = createFileName();
    writeExecutionReportToCsv(outputFileName, reports);
    return 0;
}