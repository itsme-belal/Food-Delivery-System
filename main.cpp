#include <bits/stdc++.h>
using namespace std;
using namespace std::chrono;

struct User
{
    string username;
    string password;
    vector<int> orderHistory;
    string address;
    int totalItemsOrdered = 0;
};

const string ADMIN_USERNAME = "admin";
string ADMIN_PASSWORD = "admin123";

map<string, map<string, int>> cityGraph = {
    {"Dhaka", {{"Narayanganj", 20}, {"Gazipur", 30}, {"Chittagong", 250}}},
    {"Narayanganj", {{"Dhaka", 20}, {"Gazipur", 40}, {"Comilla", 80}}},
    {"Gazipur", {{"Dhaka", 30}, {"Narayanganj", 40}, {"Mymensingh", 90}}},
    {"Chittagong", {{"Dhaka", 250}, {"Comilla", 150}, {"Cox's Bazar", 150}}},
    {"Comilla", {{"Narayanganj", 80}, {"Chittagong", 150}, {"Sylhet", 200}}},
    {"Mymensingh", {{"Gazipur", 90}, {"Tangail", 70}, {"Kishoreganj", 60}}},
    {"Sylhet", {{"Comilla", 200}, {"Moulvibazar", 40}}},
    {"Cox's Bazar", {{"Chittagong", 150}}},
    {"Tangail", {{"Mymensingh", 70}, {"Gazipur", 60}}},
    {"Kishoreganj", {{"Mymensingh", 60}, {"Narsingdi", 50}}},
    {"Moulvibazar", {{"Sylhet", 40}}},
    {"Narsingdi", {{"Kishoreganj", 50}, {"Narayanganj", 30}}}};

struct Restaurant
{
    string name;
    string city;
    vector<string> menu;
    map<string, double> itemPrices;
    map<string, int> itemWeights;
};

struct Order
{
    int id;
    string customer;
    string city;
    Restaurant restaurant;
    vector<string> items;
    string status = "Pending";
    int weight = 0;
    int priority = 1; // 1=Normal, 2=Priority, 3=VIP
    time_t orderTime;
    time_t estimatedDeliveryTime;
    double totalPrice = 0;
};

struct DeliveryVan
{
    string currentLocation;
    vector<Order> orders;
    int capacity = 10;
    int weightCapacity = 50;
    vector<string> route;
    bool onRoute = false;
    int totalDistance = 0;
    int currentWeight = 0;
    time_t dispatchTime;
};

// Add these global variables to track performance data
vector<tuple<string, int, double>> tspPerformanceData; // <testCase, inputSize, runtime>
vector<tuple<string, int, double>> knapsackPerformanceData;
vector<tuple<string, int, double>> greedyPerformanceData;
vector<Restaurant> restaurants;
vector<DeliveryVan> vans = {{"Dhaka"}, {"Chittagong"}, {"Mymensingh"}};
vector<Order> allOrders;
vector<Order> completedOrders;
vector<User> users;
unordered_map<string, int> userIndex;
int orderId = 1;
double totalRevenue = 0;
int totalOrdersDelivered = 0;
int totalDeliveryTime = 0;

const double BASE_DELIVERY_FEE = 30.0;
const double PRIORITY_MULTIPLIER = 1.5;
const double VIP_MULTIPLIER = 2.0;
const double DISTANCE_RATE = 0.2;
const double KM_PER_SECOND = 0.0139;
const int MAX_ORDER_WEIGHT = 20;

bool isAlphanumeric(const string &str)
{
    return all_of(str.begin(), str.end(), [](char c)
                  { return isalnum(c) || c == '_'; });
}

bool isValidPassword(const string &password)
{
    if (password.length() < 6)
        return false;
    return true;
}

void saveUserData()
{
    ofstream outFile("user_data.csv");
    outFile << "Username,Password,Address,TotalItemsOrdered\n";
    for (const auto &user : users)
    {
        outFile << user.username << ","
                << user.password << ","
                << user.address << ","
                << user.totalItemsOrdered << "\n";
    }
    outFile.close();
}

void loadUserData()
{
    ifstream inFile("user_data.csv");
    if (!inFile)
        return;

    string line;
    getline(inFile, line);

    while (getline(inFile, line))
    {
        stringstream ss(line);
        User user;
        string field;

        getline(ss, user.username, ',');
        getline(ss, user.password, ',');
        getline(ss, user.address, ',');

        if (getline(ss, field, ','))
        {
            user.totalItemsOrdered = stoi(field);
        }

        users.push_back(user);
        userIndex[user.username] = users.size() - 1;
    }
    inFile.close();
}

void saveCompletedOrders()
{
    ofstream outFile("completed_orders.csv", ios::app);
    if (!outFile)
        return;

    for (const auto &order : completedOrders)
    {
        outFile << order.id << ","
                << order.customer << ","
                << order.city << ","
                << order.restaurant.name << ",";

        for (size_t i = 0; i < order.items.size(); i++)
        {
            outFile << order.items[i];
            if (i != order.items.size() - 1)
                outFile << ";";
        }

        outFile << ","
                << order.status << ","
                << order.priority << ","
                << order.totalPrice << ",";

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.orderTime));
        outFile << buffer << ",";

        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.estimatedDeliveryTime));
        outFile << buffer << "\n";
    }
    outFile.close();
}

void saveActiveOrders()
{
    ofstream outFile("active_orders.csv");
    if (!outFile)
        return;

    outFile << "OrderID,Customer,City,Restaurant,Items,Status,Priority,TotalPrice,OrderTime,Weight\n";
    for (const auto &order : allOrders)
    {
        outFile << order.id << ","
                << order.customer << ","
                << order.city << ","
                << order.restaurant.name << ",";

        for (size_t i = 0; i < order.items.size(); i++)
        {
            outFile << order.items[i];
            if (i != order.items.size() - 1)
                outFile << ";";
        }

        outFile << ","
                << order.status << ","
                << order.priority << ","
                << order.totalPrice << ",";

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.orderTime));
        outFile << buffer << ","
                << order.weight << "\n";
    }
    outFile.close();
}

void loadOrderHistory()
{
    ifstream inFile("active_orders.csv");
    if (inFile)
    {
        string line;
        getline(inFile, line);

        while (getline(inFile, line))
        {
            stringstream ss(line);
            string field;
            Order order;

            getline(ss, field, ',');
            order.id = stoi(field);
            orderId = max(orderId, order.id + 1);

            getline(ss, field, ',');
            order.customer = field;

            getline(ss, field, ',');
            order.city = field;

            getline(ss, field, ',');
            for (auto &r : restaurants)
            {
                if (r.name == field)
                {
                    order.restaurant = r;
                    break;
                }
            }

            getline(ss, field, ',');
            stringstream itemsStream(field);
            string item;
            while (getline(itemsStream, item, ';'))
            {
                order.items.push_back(item);
                if (order.restaurant.itemWeights.count(item))
                {
                    order.weight += order.restaurant.itemWeights[item];
                }
                else
                {
                    order.weight += 1;
                }
                if (order.restaurant.itemPrices.count(item))
                {
                    order.totalPrice += order.restaurant.itemPrices[item];
                }
            }

            getline(ss, field, ',');
            order.status = field;

            getline(ss, field, ',');
            order.priority = stoi(field);

            getline(ss, field, ',');
            order.totalPrice = stod(field);

            getline(ss, field, ',');
            tm tm = {};
            istringstream timeStream(field);
            timeStream >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
            order.orderTime = mktime(&tm);

            getline(ss, field, ',');
            order.weight = field.empty() ? 0 : stoi(field);

            allOrders.push_back(order);
            if (userIndex.count(order.customer))
            {
                users[userIndex[order.customer]].orderHistory.push_back(order.id);
                users[userIndex[order.customer]].totalItemsOrdered += order.items.size();
            }
        }
        inFile.close();
    }

    ifstream completedInFile("completed_orders.csv");
    if (completedInFile)
    {
        string line;
        getline(completedInFile, line);

        while (getline(completedInFile, line))
        {
            stringstream ss(line);
            string field;
            Order order;

            getline(ss, field, ',');
            order.id = stoi(field);
            orderId = max(orderId, order.id + 1);

            getline(ss, field, ',');
            order.customer = field;

            getline(ss, field, ',');
            order.city = field;

            getline(ss, field, ',');
            for (auto &r : restaurants)
            {
                if (r.name == field)
                {
                    order.restaurant = r;
                    break;
                }
            }

            getline(ss, field, ',');
            stringstream itemsStream(field);
            string item;
            while (getline(itemsStream, item, ';'))
            {
                order.items.push_back(item);
                if (order.restaurant.itemWeights.count(item))
                {
                    order.weight += order.restaurant.itemWeights[item];
                }
                else
                {
                    order.weight += 1;
                }
                if (order.restaurant.itemPrices.count(item))
                {
                    order.totalPrice += order.restaurant.itemPrices[item];
                }
            }

            getline(ss, field, ',');
            order.status = field;

            getline(ss, field, ',');
            order.priority = stoi(field);

            getline(ss, field, ',');
            order.totalPrice = stod(field);

            getline(ss, field, ',');
            tm tm = {};
            istringstream timeStream(field);
            timeStream >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
            order.orderTime = mktime(&tm);

            getline(ss, field, ',');
            if (!field.empty())
            {
                tm = {};
                istringstream deliveryStream(field);
                deliveryStream >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
                order.estimatedDeliveryTime = mktime(&tm);
            }

            completedOrders.push_back(order);
            totalOrdersDelivered++;
            int deliveryTime = static_cast<int>(difftime(order.estimatedDeliveryTime, order.orderTime));
            totalDeliveryTime += deliveryTime;
            totalRevenue += order.totalPrice;

            if (userIndex.count(order.customer))
            {
                users[userIndex[order.customer]].orderHistory.push_back(order.id);
                users[userIndex[order.customer]].totalItemsOrdered += order.items.size();
            }
        }
        completedInFile.close();
    }
}

pair<vector<string>, int> solveTSP(const vector<string> &cities)
{
    auto start = high_resolution_clock::now();

    if (cities.empty())
    {
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(stop - start);
        tspPerformanceData.push_back(make_tuple("Empty", 0, duration.count()));
        return make_pair(vector<string>(), 0);
    }

    int n = cities.size();
    vector<vector<int>> dist(n, vector<int>(n, 0));

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            if (i != j)
            {
                if (cityGraph[cities[i]].count(cities[j]))
                {
                    dist[i][j] = cityGraph[cities[i]][cities[j]];
                }
                else
                {
                    dist[i][j] = 1000;
                }
            }
        }
    }

    vector<vector<int>> dp(1 << n, vector<int>(n, INT_MAX));
    vector<vector<int>> parent(1 << n, vector<int>(n, -1));

    dp[1][0] = 0;

    for (int mask = 1; mask < (1 << n); mask++)
    {
        for (int last = 0; last < n; last++)
        {
            if (!(mask & (1 << last)))
                continue;
            if (dp[mask][last] == INT_MAX)
                continue;

            for (int next = 0; next < n; next++)
            {
                if (mask & (1 << next))
                    continue;
                int new_mask = mask | (1 << next);
                if (dp[new_mask][next] > dp[mask][last] + dist[last][next])
                {
                    dp[new_mask][next] = dp[mask][last] + dist[last][next];
                    parent[new_mask][next] = last;
                }
            }
        }
    }

    int final_mask = (1 << n) - 1;
    int min_cost = INT_MAX;
    int last_city = -1;

    for (int i = 1; i < n; i++)
    {
        if (dp[final_mask][i] + dist[i][0] < min_cost)
        {
            min_cost = dp[final_mask][i] + dist[i][0];
            last_city = i;
        }
    }

    vector<string> path;
    int current_city = last_city;
    int current_mask = final_mask;

    while (current_city != -1)
    {
        path.push_back(cities[current_city]);
        int prev_city = parent[current_mask][current_city];
        current_mask ^= (1 << current_city);
        current_city = prev_city;
    }

    reverse(path.begin(), path.end());
    path.push_back(cities[0]);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    tspPerformanceData.push_back(make_tuple("Cities", n, duration.count()));

    return make_pair(path, min_cost);
}

pair<vector<Order>, int> knapsackPriority(const vector<Order> &orders, int capacity, int weightCapacity)
{
    auto start = high_resolution_clock::now();

    int n = orders.size();
    vector<vector<vector<int>>> dp(n + 1, vector<vector<int>>(capacity + 1, vector<int>(weightCapacity + 1, 0)));

    for (int i = 1; i <= n; i++)
    {
        for (int c = 0; c <= capacity; c++)
        {
            for (int w = 0; w <= weightCapacity; w++)
            {
                if (orders[i - 1].items.size() <= c && orders[i - 1].weight <= w)
                {
                    int include = orders[i - 1].priority * 10 + dp[i - 1][c - orders[i - 1].items.size()][w - orders[i - 1].weight];
                    int exclude = dp[i - 1][c][w];
                    dp[i][c][w] = max(include, exclude);
                }
                else
                {
                    dp[i][c][w] = dp[i - 1][c][w];
                }
            }
        }
    }

    vector<Order> selected;
    int c = capacity;
    int w = weightCapacity;
    for (int i = n; i > 0; i--)
    {
        if (dp[i][c][w] != dp[i - 1][c][w])
        {
            selected.push_back(orders[i - 1]);
            c -= orders[i - 1].items.size();
            w -= orders[i - 1].weight;
        }
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    knapsackPerformanceData.push_back(make_tuple("Orders", n, duration.count()));

    return make_pair(selected, dp[n][capacity][weightCapacity]);
}

// Add this new function to track greedy heuristic performance
void trackGreedyPerformance(const vector<Order> &orders)
{
    auto start = high_resolution_clock::now();

    // Simulate a greedy heuristic (sorting by priority)
    vector<Order> sortedOrders = orders;
    sort(sortedOrders.begin(), sortedOrders.end(), [](const Order &a, const Order &b)
         { return a.priority > b.priority; });

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    greedyPerformanceData.push_back(make_tuple("Orders", orders.size(), duration.count()));
}

// Add this function to save performance data to CSV files
void savePerformanceData()
{
    // Save TSP performance data
    ofstream tspFile("tsp_performance.csv");
    tspFile << "Algorithm,InputSize,Runtime(ms)\n";
    for (const auto &data : tspPerformanceData)
    {
        tspFile << get<0>(data) << "," << get<1>(data) << "," << get<2>(data) << "\n";
    }
    tspFile.close();

    // Save Knapsack performance data
    ofstream knapsackFile("knapsack_performance.csv");
    knapsackFile << "Algorithm,InputSize,Runtime(ms)\n";
    for (const auto &data : knapsackPerformanceData)
    {
        knapsackFile << get<0>(data) << "," << get<1>(data) << "," << get<2>(data) << "\n";
    }
    knapsackFile.close();

    // Save Greedy performance data
    ofstream greedyFile("greedy_performance.csv");
    greedyFile << "Algorithm,InputSize,Runtime(ms)\n";
    for (const auto &data : greedyPerformanceData)
    {
        greedyFile << get<0>(data) << "," << get<1>(data) << "," << get<2>(data) << "\n";
    }
    greedyFile.close();
}

// Add this function to display performance analysis
void viewPerformanceAnalysis()
{
    cout << "\nPerformance Analysis:\n";

    // TSP Analysis
    if (!tspPerformanceData.empty())
    {
        cout << "\nTSP Algorithm:\n";
        cout << "Input Size\tRuntime (μs)\n"; // Changed from "Runtime (ms)" to "Runtime (μs)"
        for (const auto &data : tspPerformanceData)
        {
            cout << get<1>(data) << "\t\t" << get<2>(data) << "\n"; // No division needed
        }
    }

    // Knapsack Analysis
    if (!knapsackPerformanceData.empty())
    {
        cout << "\nKnapsack Algorithm:\n";
        cout << "Input Size\tRuntime (ms)\n";
        for (const auto &data : knapsackPerformanceData)
        {
            cout << get<1>(data) << "\t\t" << get<2>(data) << "\n";
        }
    }

    // Greedy Analysis
    if (!greedyPerformanceData.empty())
    {
        cout << "\nGreedy Algorithm:\n";
        cout << "Input Size\tRuntime (ms)\n";
        for (const auto &data : greedyPerformanceData)
        {
            cout << get<1>(data) << "\t\t" << get<2>(data) << "\n";
        }
    }

    // Save data to CSV files for Excel
    savePerformanceData();
    cout << "\nPerformance data saved to CSV files for Excel analysis:\n";
    cout << "- tsp_performance.csv\n";
    cout << "- knapsack_performance.csv\n";
    cout << "- greedy_performance.csv\n";
}

// Add this function to generate test cases for performance analysis
void runPerformanceTests()
{
    cout << "\nRunning performance tests...\n";

    // Clear previous data
    tspPerformanceData.clear();
    knapsackPerformanceData.clear();
    greedyPerformanceData.clear();

    // Test TSP with different city counts
    vector<vector<string>> tspTestCases = {
        {"Dhaka", "Narayanganj"},
        {"Dhaka", "Narayanganj", "Gazipur"},
        {"Dhaka", "Narayanganj", "Gazipur", "Chittagong"},
        {"Dhaka", "Narayanganj", "Gazipur", "Chittagong", "Comilla"},
        {"Dhaka", "Narayanganj", "Gazipur", "Chittagong", "Comilla", "Sylhet"}};

    for (const auto &testCase : tspTestCases)
    {
        solveTSP(testCase);
    }

    // Test Knapsack with different order counts
    vector<vector<Order>> knapsackTestCases;
    for (int i = 1; i <= 5; i++)
    {
        vector<Order> orders;
        for (int j = 0; j < i * 2; j++)
        {
            Order o;
            o.id = j;
            o.priority = (j % 3) + 1;
            o.items = vector<string>(j % 3 + 1, "item");
            o.weight = j % 3 + 1;
            orders.push_back(o);
        }
        knapsackTestCases.push_back(orders);
    }

    for (const auto &testCase : knapsackTestCases)
    {
        knapsackPriority(testCase, 10, 10);
    }

    // Test Greedy with different order counts
    for (const auto &testCase : knapsackTestCases)
    {
        trackGreedyPerformance(testCase);
    }

    cout << "Performance tests completed!\n";
    viewPerformanceAnalysis();
}

double calculateDeliveryFee(const Order &order)
{
    vector<string> cities = {"Dhaka", order.city};
    auto pathInfo = solveTSP(cities);
    int distance = pathInfo.second;
    double baseFee = BASE_DELIVERY_FEE + (distance * DISTANCE_RATE);

    if (order.priority == 2)
        baseFee *= PRIORITY_MULTIPLIER;
    else if (order.priority == 3)
        baseFee *= VIP_MULTIPLIER;

    return baseFee;
}

int calculateEstimatedDeliveryTime(const Order &order)
{
    vector<string> cities = {"Dhaka", order.city};
    auto pathInfo = solveTSP(cities);
    int distance = pathInfo.second;
    return static_cast<int>(distance / KM_PER_SECOND);
}

vector<string> findRoute(string startCity, vector<string> toVisit)
{
    toVisit.insert(toVisit.begin(), startCity);
    auto result = solveTSP(toVisit);
    return result.first;
}

void updateVanPosition(DeliveryVan &van)
{
    if (!van.onRoute || van.route.empty())
        return;

    time_t now = time(nullptr);
    double elapsedSeconds = difftime(now, van.dispatchTime);
    int distanceCovered = static_cast<int>(elapsedSeconds * KM_PER_SECOND);

    int totalDistance = 0;
    string prevCity = van.route[0];

    for (size_t i = 1; i < van.route.size(); i++)
    {
        if (!cityGraph[prevCity].count(van.route[i]))
        {
            van.onRoute = false;
            return;
        }

        int segmentDist = cityGraph[prevCity][van.route[i]];
        if (totalDistance + segmentDist > distanceCovered)
        {
            van.currentLocation = prevCity;
            break;
        }
        totalDistance += segmentDist;
        prevCity = van.route[i];
        van.currentLocation = prevCity;
    }

    for (auto &order : van.orders)
    {
        if (order.status == "In Transit")
        {
            int remainingDistance = 0;
            bool found = false;
            string current = van.currentLocation;

            for (size_t i = 0; i < van.route.size(); i++)
            {
                if (van.route[i] == current)
                {
                    for (size_t j = i; j < van.route.size(); j++)
                    {
                        if (van.route[j] == order.city)
                        {
                            found = true;
                            break;
                        }
                        if (j < van.route.size() - 1)
                        {
                            if (cityGraph[van.route[j]].count(van.route[j + 1]))
                            {
                                remainingDistance += cityGraph[van.route[j]][van.route[j + 1]];
                            }
                        }
                    }
                    break;
                }
            }

            if (found)
            {
                order.estimatedDeliveryTime = now + static_cast<time_t>(remainingDistance / KM_PER_SECOND);
            }
        }
    }

    vector<Order> remainingOrders;
    for (auto &order : van.orders)
    {
        if (van.currentLocation == order.city && order.status == "In Transit")
        {
            order.status = "Delivered";
            order.estimatedDeliveryTime = now;
            completedOrders.push_back(order);
            totalOrdersDelivered++;
            totalDeliveryTime += static_cast<int>(difftime(now, order.orderTime));
            totalRevenue += order.totalPrice;

            if (userIndex.count(order.customer))
            {
                users[userIndex[order.customer]].orderHistory.push_back(order.id);
            }
        }
        else
        {
            remainingOrders.push_back(order);
        }
    }
    van.orders = remainingOrders;

    if (van.currentLocation == van.route.back())
    {
        van.onRoute = false;
    }
}

void startDelivery()
{
    map<string, vector<Order>> cityOrders;
    for (auto &order : allOrders)
    {
        if (order.status == "Pending")
        {
            cityOrders[order.city].push_back(order);
        }
    }

    bool allVansBusy = true;

    for (auto &van : vans)
    {
        if (van.onRoute)
            continue;

        allVansBusy = false;
        vector<Order> candidateOrders;
        for (auto &cityGroup : cityOrders)
        {
            for (auto &order : cityGroup.second)
            {
                if (order.status == "Pending")
                {
                    candidateOrders.push_back(order);
                }
            }
        }

        auto selected = knapsackPriority(candidateOrders, van.capacity, van.weightCapacity);

        if (selected.first.empty())
        {
            continue;
        }

        for (auto &order : selected.first)
        {
            auto it = find_if(allOrders.begin(), allOrders.end(), [&order](const Order &o)
                              { return o.id == order.id; });

            if (it != allOrders.end())
            {
                it->status = "In Transit";
                van.orders.push_back(*it);
                van.currentWeight += it->weight;
            }
        }

        if (!van.orders.empty())
        {
            vector<string> toVisit;
            for (auto &order : van.orders)
            {
                if (find(toVisit.begin(), toVisit.end(), order.city) == toVisit.end())
                {
                    toVisit.push_back(order.city);
                }
            }

            van.route = findRoute(van.currentLocation, toVisit);
            van.onRoute = true;
            van.dispatchTime = time(nullptr);
            van.totalDistance = 0;

            int routeDistance = 0;
            string prevCity = van.route[0];
            for (size_t i = 1; i < van.route.size(); i++)
            {
                routeDistance += cityGraph[prevCity][van.route[i]];
                prevCity = van.route[i];

                for (auto &order : van.orders)
                {
                    if (order.city == van.route[i] && order.status == "In Transit")
                    {
                        order.estimatedDeliveryTime = van.dispatchTime + static_cast<time_t>(routeDistance / KM_PER_SECOND);
                    }
                }
            }

            cout << "\nVan from " << van.currentLocation << " started delivery!\n";
            cout << "Route: ";
            for (size_t i = 0; i < van.route.size(); i++)
            {
                cout << van.route[i];
                if (i != van.route.size() - 1)
                    cout << " -> ";
            }
            cout << "\nCarrying " << van.orders.size() << " orders ("
                 << van.currentWeight << "/" << van.weightCapacity << " kg)\n";
            cout << "Estimated delivery times:\n";
            for (auto &order : van.orders)
            {
                cout << "Order " << order.id << " to " << order.city << ": "
                     << order.items.size() << " items, "
                     << static_cast<int>(difftime(order.estimatedDeliveryTime, time(nullptr)))
                     << " seconds\n";
            }
        }
    }

    if (allVansBusy)
    {
        cout << "\nAll delivery vans are currently busy. Your order will be processed when a van becomes available.\n";
    }

    saveActiveOrders();
}

void completeDelivery()
{
    for (auto &van : vans)
    {
        if (!van.onRoute)
            continue;

        vector<Order> deliveredOrders;
        vector<Order> remainingOrders;

        for (auto &order : van.orders)
        {
            if (van.currentLocation == order.city && order.status == "In Transit")
            {
                order.status = "Delivered";
                order.estimatedDeliveryTime = time(nullptr);
                deliveredOrders.push_back(order);
            }
            else
            {
                remainingOrders.push_back(order);
            }
        }

        if (!deliveredOrders.empty())
        {
            cout << "\nVan from " << van.currentLocation << " delivered orders!\n";
            for (auto &order : deliveredOrders)
            {
                completedOrders.push_back(order);
                totalOrdersDelivered++;
                totalDeliveryTime += static_cast<int>(difftime(order.estimatedDeliveryTime, order.orderTime));
                totalRevenue += order.totalPrice;

                if (userIndex.count(order.customer))
                {
                    users[userIndex[order.customer]].orderHistory.push_back(order.id);
                }

                cout << "- Order #" << order.id << " delivered to " << order.city
                     << " (" << order.items.size() << " items, " << order.weight << " kg)\n";
            }
        }

        van.orders = remainingOrders;

        if (van.currentLocation == van.route.back())
        {
            van.onRoute = false;
            van.route.clear();
            van.currentWeight = 0;
        }
    }

    saveCompletedOrders();
    saveActiveOrders();
}

void registerUser()
{
    User newUser;
    cout << "\nEnter username (alphanumeric only): ";
    cin >> newUser.username;

    if (!isAlphanumeric(newUser.username))
    {
        cout << "Username must be alphanumeric only!\n";
        return;
    }

    if (userIndex.count(newUser.username))
    {
        cout << "Username already exists!\n";
        return;
    }

    cout << "Enter password (minimum 6 characters): ";
    cin >> newUser.password;

    if (!isValidPassword(newUser.password))
    {
        cout << "Password must be at least 6 characters long!\n";
        return;
    }

    cout << "Enter address: ";
    cin.ignore();
    getline(cin, newUser.address);

    users.push_back(newUser);
    userIndex[newUser.username] = users.size() - 1;
    cout << "Registration successful!\n";
    saveUserData();
}

int loginUser()
{
    string username, password;
    cout << "\nEnter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;

    if (userIndex.count(username))
    {
        User &user = users[userIndex[username]];
        if (user.password == password)
        {
            cout << "Login successful!\n";
            cout << "Total items ordered: " << user.totalItemsOrdered << "\n";
            return userIndex[username];
        }
        else
        {
            cout << "Invalid password!\n";
        }
    }
    else
    {
        cout << "Username not found!\n";
    }

    return -1;
}

bool adminLogin()
{
    string username, password;
    cout << "\nEnter admin username: ";
    cin >> username;
    cout << "Enter admin password: ";
    cin >> password;

    if (username == ADMIN_USERNAME && password == ADMIN_PASSWORD)
    {
        cout << "Admin login successful!\n";
        return true;
    }
    else
    {
        cout << "Admin login failed!\n";
        return false;
    }
}

void cancelOrder(int userIndex)
{
    if (userIndex < 0 || userIndex >= static_cast<int>(users.size()))
        return;

    User &user = users[userIndex];
    if (user.orderHistory.empty())
    {
        cout << "No orders to cancel!\n";
        return;
    }

    cout << "\nYour orders:\n";
    vector<int> cancelableOrders;
    for (int orderId : user.orderHistory)
    {
        auto it = find_if(allOrders.begin(), allOrders.end(), [orderId](const Order &o)
                          { return o.id == orderId && o.status == "Pending"; });
        if (it != allOrders.end())
        {
            cout << "ID: " << it->id << " - " << it->restaurant.name << " (" << it->city << ") - ";
            cout << it->items.size() << " items - " << it->totalPrice << " Tk\n";
            cancelableOrders.push_back(it->id);
        }
    }

    if (cancelableOrders.empty())
    {
        cout << "No cancelable orders found!\n";
        return;
    }

    cout << "Enter order ID to cancel (0 to go back): ";
    int orderId;
    while (!(cin >> orderId) || (orderId != 0 && find(cancelableOrders.begin(), cancelableOrders.end(), orderId) == cancelableOrders.end()))
    {
        cout << "Invalid input. Enter a valid order ID or 0 to cancel: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }

    if (orderId == 0)
        return;

    auto it = find_if(allOrders.begin(), allOrders.end(), [orderId](const Order &o)
                      { return o.id == orderId; });
    if (it != allOrders.end())
    {
        it->status = "Cancelled";
        cout << "Order #" << orderId << " has been cancelled.\n";
        saveActiveOrders();
    }
}

void placeOrder(int userIndex)
{
    if (userIndex < 0 || userIndex >= static_cast<int>(users.size()))
        return;

    Order order;
    order.id = orderId++;
    order.customer = users[userIndex].username;
    order.orderTime = time(nullptr);

    cout << "\nSelect priority (1=Normal, 2=Priority, 3=VIP): ";
    while (!(cin >> order.priority) || order.priority < 1 || order.priority > 3)
    {
        cout << "Invalid input. Enter 1, 2 or 3: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }

    cout << "\nAvailable cities:\n";
    int i = 1;
    for (auto &city : cityGraph)
    {
        cout << i++ << ". " << city.first << endl;
    }

    cout << "Select city (1-" << cityGraph.size() << "): ";
    int choice;
    while (!(cin >> choice) || choice < 1 || choice > static_cast<int>(cityGraph.size()))
    {
        cout << "Invalid input: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    order.city = next(cityGraph.begin(), choice - 1)->first;

    vector<Restaurant> cityRestaurants;
    for (auto &r : restaurants)
    {
        if (r.city == order.city)
        {
            cityRestaurants.push_back(r);
        }
    }

    if (cityRestaurants.empty())
    {
        cout << "No restaurants available in this city!\n";
        return;
    }

    cout << "\nRestaurants:\n";
    for (size_t i = 0; i < cityRestaurants.size(); i++)
    {
        cout << i + 1 << ". " << cityRestaurants[i].name << "\n";
    }

    cout << "Select restaurant (1-" << cityRestaurants.size() << "): ";
    while (!(cin >> choice) || choice < 1 || choice > static_cast<int>(cityRestaurants.size()))
    {
        cout << "Invalid input: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    order.restaurant = cityRestaurants[choice - 1];

    cout << "\nMenu:\n";
    for (size_t i = 0; i < order.restaurant.menu.size(); i++)
    {
        cout << i + 1 << ". " << order.restaurant.menu[i]
             << " - " << order.restaurant.itemPrices[order.restaurant.menu[i]] << " Tk"
             << " (" << order.restaurant.itemWeights[order.restaurant.menu[i]] << " kg)\n";
    }

    cout << "\nEnter item numbers (0 to finish):\n";
    while (true)
    {
        int itemChoice;
        while (!(cin >> itemChoice))
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        if (itemChoice == 0)
            break;
        if (itemChoice < 1 || itemChoice > static_cast<int>(order.restaurant.menu.size()))
        {
            cout << "Invalid choice: ";
            continue;
        }

        string selectedItem = order.restaurant.menu[itemChoice - 1];
        order.items.push_back(selectedItem);
        order.weight += order.restaurant.itemWeights[selectedItem];
        order.totalPrice += order.restaurant.itemPrices[selectedItem];
        cout << "Added " << selectedItem << " (Weight: " << order.restaurant.itemWeights[selectedItem] << " kg)\n";

        if (order.weight > MAX_ORDER_WEIGHT)
        {
            cout << "Warning: Order weight exceeds maximum limit (" << MAX_ORDER_WEIGHT << " kg).\n";
            cout << "Current weight: " << order.weight << " kg. Please remove some items.\n";
            order.items.pop_back();
            order.weight -= order.restaurant.itemWeights[selectedItem];
            order.totalPrice -= order.restaurant.itemPrices[selectedItem];
        }
    }

    if (order.items.empty())
    {
        cout << "No items selected. Order cancelled.\n";
        return;
    }

    double deliveryFee = calculateDeliveryFee(order);
    order.totalPrice += deliveryFee;
    int deliveryTime = calculateEstimatedDeliveryTime(order);
    order.estimatedDeliveryTime = order.orderTime + deliveryTime;

    allOrders.push_back(order);
    users[userIndex].orderHistory.push_back(order.id);
    users[userIndex].totalItemsOrdered += order.items.size();

    cout << "\nOrder placed!\n";
    cout << "ID: " << order.id << endl;
    cout << "Items (" << order.items.size() << "): ";
    for (size_t i = 0; i < order.items.size(); i++)
    {
        cout << order.items[i] << " (" << order.restaurant.itemWeights[order.items[i]] << " kg)";
        if (i != order.items.size() - 1)
            cout << ", ";
    }
    cout << "\nTotal weight: " << order.weight << " kg\n";
    cout << "Total price: " << fixed << setprecision(2) << order.totalPrice
         << " Tk (fee: " << deliveryFee << " Tk)\n";
    cout << "ETA: " << deliveryTime << " seconds\n";
    cout << "Your total items ordered: " << users[userIndex].totalItemsOrdered << "\n";

    saveActiveOrders();
    saveUserData();
}

void viewOrderHistory(int userIndex)
{
    if (userIndex < 0 || userIndex >= static_cast<int>(users.size()))
        return;

    User &user = users[userIndex];
    if (user.orderHistory.empty())
    {
        cout << "No orders found!\n";
        return;
    }

    cout << "\nOrder history (" << user.totalItemsOrdered << " total items ordered):\n";
    for (int orderId : user.orderHistory)
    {
        bool found = false;
        for (const auto &order : allOrders)
        {
            if (order.id == orderId)
            {
                cout << "ID: " << order.id << endl;
                cout << "Restaurant: " << order.restaurant.name << " (" << order.city << ")\n";
                cout << "Items (" << order.items.size() << "): ";
                for (size_t i = 0; i < order.items.size(); i++)
                {
                    cout << order.items[i] << " (" << order.restaurant.itemWeights.at(order.items[i]) << " kg)";
                    if (i != order.items.size() - 1)
                        cout << ", ";
                }
                cout << "\nTotal weight: " << order.weight << " kg\n";
                cout << "Status: " << order.status << endl;
                cout << "Total: " << fixed << setprecision(2) << order.totalPrice << " Tk\n";

                char buffer[80];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.orderTime));
                cout << "Order time: " << buffer << endl;

                if (order.status == "Delivered")
                {
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.estimatedDeliveryTime));
                    cout << "Delivered at: " << buffer << endl;
                    cout << "Delivery time: " << static_cast<int>(difftime(order.estimatedDeliveryTime, order.orderTime)) << " seconds\n";
                }
                else if (order.status == "In Transit")
                {
                    int remaining = static_cast<int>(difftime(order.estimatedDeliveryTime, time(nullptr)));
                    if (remaining > 0)
                        cout << "ETA: " << remaining << " seconds\n";
                    else
                        cout << "Coming soon!\n";
                }
                else if (order.status == "Cancelled")
                {
                    cout << "This order has been cancelled.\n";
                }
                cout << "----------------\n";
                found = true;
                break;
            }
        }

        if (!found)
        {
            for (const auto &order : completedOrders)
            {
                if (order.id == orderId)
                {
                    cout << "ID: " << order.id << endl;
                    cout << "Restaurant: " << order.restaurant.name << " (" << order.city << ")\n";
                    cout << "Items (" << order.items.size() << "): ";
                    for (size_t i = 0; i < order.items.size(); i++)
                    {
                        cout << order.items[i] << " (" << order.restaurant.itemWeights.at(order.items[i]) << " kg)";
                        if (i != order.items.size() - 1)
                            cout << ", ";
                    }
                    cout << "\nTotal weight: " << order.weight << " kg\n";
                    cout << "Status: " << order.status << endl;
                    cout << "Total: " << fixed << setprecision(2) << order.totalPrice << " Tk\n";

                    char buffer[80];
                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.orderTime));
                    cout << "Order time: " << buffer << endl;

                    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.estimatedDeliveryTime));
                    cout << "Delivered at: " << buffer << endl;
                    cout << "Delivery time: " << static_cast<int>(difftime(order.estimatedDeliveryTime, order.orderTime)) << " seconds\n";
                    cout << "----------------\n";
                    break;
                }
            }
        }
    }
}

void viewAllPendingOrders()
{
    vector<Order> pendingOrders;
    for (const auto &order : allOrders)
    {
        if (order.status == "Pending")
        {
            pendingOrders.push_back(order);
        }
    }

    if (pendingOrders.empty())
    {
        cout << "\nNo pending orders!\n";
        return;
    }

    cout << "\nAll Pending Orders (" << pendingOrders.size() << "):\n";
    for (const auto &order : pendingOrders)
    {
        cout << "ID: " << order.id << endl;
        cout << "Customer: " << order.customer << endl;
        cout << "Restaurant: " << order.restaurant.name << " (" << order.city << ")\n";
        cout << "Items (" << order.items.size() << "): ";
        for (size_t i = 0; i < order.items.size(); i++)
        {
            cout << order.items[i] << " (" << order.restaurant.itemWeights.at(order.items[i]) << " kg)";
            if (i != order.items.size() - 1)
                cout << ", ";
        }
        cout << "\nTotal weight: " << order.weight << " kg\n";
        cout << "Priority: " << (order.priority == 1 ? "Normal" : (order.priority == 2 ? "Priority" : "VIP")) << endl;
        cout << "Total: " << fixed << setprecision(2) << order.totalPrice << " Tk\n";

        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&order.orderTime));
        cout << "Order time: " << buffer << endl;
        cout << "----------------\n";
    }
}

void viewDeliveryStatus()
{
    for (auto &van : vans)
    {
        cout << "\nVan at " << van.currentLocation << ":\n";
        if (!van.onRoute)
        {
            cout << "Status: Available\n";
            cout << "Capacity: " << van.orders.size() << "/" << van.capacity << " items\n";
            cout << "Weight: " << van.currentWeight << "/" << van.weightCapacity << " kg\n";
            continue;
        }

        cout << "Status: On Route\n";
        cout << "Route: ";
        for (size_t i = 0; i < van.route.size(); i++)
        {
            cout << van.route[i];
            if (i != van.route.size() - 1)
                cout << " -> ";
        }
        cout << "\nCarrying " << van.orders.size() << " orders ("
             << van.currentWeight << "/" << van.weightCapacity << " kg)\n";
        cout << "Orders:\n";
        for (const auto &order : van.orders)
        {
            cout << "- Order #" << order.id << " to " << order.city
                 << " (" << order.items.size() << " items, "
                 << order.weight << " kg)";
            if (order.status == "In Transit")
            {
                int remaining = static_cast<int>(difftime(order.estimatedDeliveryTime, time(nullptr)));
                cout << " - ETA: " << max(0, remaining) << " seconds";
            }
            cout << endl;
        }
    }
}

void viewAnalytics()
{
    cout << "\nAnalytics:\n";
    cout << "Total orders delivered: " << totalOrdersDelivered << endl;
    cout << "Total revenue: " << fixed << setprecision(2) << totalRevenue << " Tk\n";
    if (totalOrdersDelivered > 0)
    {
        cout << "Avg delivery time: " << totalDeliveryTime / totalOrdersDelivered << " seconds\n";
    }
    cout << "Pending orders: " << count_if(allOrders.begin(), allOrders.end(), [](const Order &o)
                                           { return o.status == "Pending"; })
         << endl;
    cout << "In Transit orders: " << count_if(allOrders.begin(), allOrders.end(), [](const Order &o)
                                              { return o.status == "In Transit"; })
         << endl;
}

void addRestaurant()
{
    Restaurant newRestaurant;
    cout << "\nEnter restaurant name: ";
    cin.ignore();
    getline(cin, newRestaurant.name);

    cout << "Available cities:\n";
    int i = 1;
    for (auto &city : cityGraph)
    {
        cout << i++ << ". " << city.first << endl;
    }

    cout << "Select city (1-" << cityGraph.size() << "): ";
    int choice;
    while (!(cin >> choice) || choice < 1 || choice > static_cast<int>(cityGraph.size()))
    {
        cout << "Invalid input: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    newRestaurant.city = next(cityGraph.begin(), choice - 1)->first;

    cout << "Enter number of menu items: ";
    int itemCount;
    while (!(cin >> itemCount) || itemCount < 1)
    {
        cout << "Invalid input: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }

    for (int i = 0; i < itemCount; i++)
    {
        string itemName;
        double price;
        int weight;

        cout << "Item " << i + 1 << " name: ";
        cin.ignore();
        getline(cin, itemName);

        cout << "Item " << i + 1 << " price: ";
        while (!(cin >> price) || price <= 0)
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        cout << "Item " << i + 1 << " weight (kg): ";
        while (!(cin >> weight) || weight <= 0)
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        newRestaurant.menu.push_back(itemName);
        newRestaurant.itemPrices[itemName] = price;
        newRestaurant.itemWeights[itemName] = weight;
    }

    restaurants.push_back(newRestaurant);
    cout << "Restaurant added successfully!\n";
}

void adminMenu()
{
    while (true)
    {
        cout << "\nAdmin Menu:\n";
        cout << "1. View Delivery Status\n";
        cout << "2. Start Delivery\n";
        cout << "3. Complete Delivery\n";
        cout << "4. View Analytics\n";
        cout << "5. Add Restaurant\n";
        cout << "6. View All Pending Orders\n";
        cout << "7. Performance Analysis\n";  // New option
        cout << "8. Run Performance Tests\n"; // New option
        cout << "9. Exit\n";
        cout << "Choice: ";

        int choice;
        while (!(cin >> choice) || choice < 1 || choice > 9)
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        switch (choice)
        {
        case 1:
            viewDeliveryStatus();
            break;
        case 2:
            startDelivery();
            break;
        case 3:
            completeDelivery();
            break;
        case 4:
            viewAnalytics();
            break;
        case 5:
            addRestaurant();
            break;
        case 6:
            viewAllPendingOrders();
            break;
        case 7:
            viewPerformanceAnalysis();
            break;
        case 8:
            runPerformanceTests();
            break;
        case 9:
            return;
        }
    }
}

void userMenu(int userIndex)
{
    while (true)
    {
        cout << "\nUser Menu:\n";
        cout << "1. Place Order\n";
        cout << "2. View Order History\n";
        cout << "3. Cancel Order\n";
        cout << "4. Exit\n";
        cout << "Choice: ";

        int choice;
        while (!(cin >> choice) || choice < 1 || choice > 4)
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        switch (choice)
        {
        case 1:
            placeOrder(userIndex);
            break;
        case 2:
            viewOrderHistory(userIndex);
            break;
        case 3:
            cancelOrder(userIndex);
            break;
        case 4:
            return;
        }
    }
}

void initializeRestaurants()
{
    // Dhaka restaurants
    Restaurant r1 = {"Burger King", "Dhaka", {"Whopper", "Cheeseburger", "Fries", "Chicken Nuggets", "Onion Rings"}, {{"Whopper", 350}, {"Cheeseburger", 200}, {"Fries", 100}, {"Chicken Nuggets", 180}, {"Onion Rings", 120}}, {{"Whopper", 2}, {"Cheeseburger", 1}, {"Fries", 1}, {"Chicken Nuggets", 1}, {"Onion Rings", 1}}};

    Restaurant r2 = {"Pizza Hut", "Dhaka", {"Margherita", "Pepperoni", "Garlic Bread", "Hawaiian", "Vegetarian"}, {{"Margherita", 500}, {"Pepperoni", 600}, {"Garlic Bread", 150}, {"Hawaiian", 650}, {"Vegetarian", 550}}, {{"Margherita", 3}, {"Pepperoni", 3}, {"Garlic Bread", 1}, {"Hawaiian", 3}, {"Vegetarian", 3}}};

    Restaurant r3 = {"KFC", "Dhaka", {"Chicken Bucket", "Zinger Burger", "Coleslaw", "Chicken Wings", "Mashed Potatoes"}, {{"Chicken Bucket", 800}, {"Zinger Burger", 250}, {"Coleslaw", 80}, {"Chicken Wings", 350}, {"Mashed Potatoes", 100}}, {{"Chicken Bucket", 4}, {"Zinger Burger", 2}, {"Coleslaw", 1}, {"Chicken Wings", 2}, {"Mashed Potatoes", 1}}};

    Restaurant r4 = {"Nando's", "Dhaka", {"Peri-Peri Chicken", "Chicken Wrap", "Garlic Bread", "Corn on the Cob", "Rice"}, {{"Peri-Peri Chicken", 750}, {"Chicken Wrap", 300}, {"Garlic Bread", 120}, {"Corn on the Cob", 80}, {"Rice", 60}}, {{"Peri-Peri Chicken", 4}, {"Chicken Wrap", 2}, {"Garlic Bread", 1}, {"Corn on the Cob", 1}, {"Rice", 1}}};

    // Chittagong restaurants
    Restaurant r5 = {"Pizza Hut", "Chittagong", {"Margherita", "Pepperoni", "Garlic Bread", "BBQ Chicken", "Meat Lovers"}, {{"Margherita", 500}, {"Pepperoni", 600}, {"Garlic Bread", 150}, {"BBQ Chicken", 700}, {"Meat Lovers", 750}}, {{"Margherita", 3}, {"Pepperoni", 3}, {"Garlic Bread", 1}, {"BBQ Chicken", 3}, {"Meat Lovers", 4}}};

    Restaurant r6 = {"KFC", "Chittagong", {"Chicken Bucket", "Zinger Burger", "Coleslaw", "Popcorn Chicken", "Potato Wedges"}, {{"Chicken Bucket", 800}, {"Zinger Burger", 250}, {"Coleslaw", 80}, {"Popcorn Chicken", 200}, {"Potato Wedges", 120}}, {{"Chicken Bucket", 4}, {"Zinger Burger", 2}, {"Coleslaw", 1}, {"Popcorn Chicken", 1}, {"Potato Wedges", 1}}};

    // Gazipur restaurants
    Restaurant r7 = {"KFC", "Gazipur", {"Chicken Bucket", "Zinger Burger", "Coleslaw", "Chicken Sandwich", "Chicken Pie"}, {{"Chicken Bucket", 800}, {"Zinger Burger", 250}, {"Coleslaw", 80}, {"Chicken Sandwich", 220}, {"Chicken Pie", 150}}, {{"Chicken Bucket", 4}, {"Zinger Burger", 2}, {"Coleslaw", 1}, {"Chicken Sandwich", 2}, {"Chicken Pie", 1}}};

    // Narayanganj restaurants
    Restaurant r8 = {"Star Kabab", "Narayanganj", {"Beef Kabab", "Chicken Kabab", "Mutton Kabab", "Naan", "Salad"}, {{"Beef Kabab", 300}, {"Chicken Kabab", 250}, {"Mutton Kabab", 350}, {"Naan", 20}, {"Salad", 30}}, {{"Beef Kabab", 2}, {"Chicken Kabab", 2}, {"Mutton Kabab", 2}, {"Naan", 1}, {"Salad", 1}}};

    // Sylhet restaurants
    Restaurant r9 = {"Panshi Restaurant", "Sylhet", {"Beef Tehari", "Chicken Tehari", "Mutton Tehari", "Borhan", "Salad"}, {{"Beef Tehari", 180}, {"Chicken Tehari", 150}, {"Mutton Tehari", 200}, {"Borhan", 30}, {"Salad", 20}}, {{"Beef Tehari", 2}, {"Chicken Tehari", 2}, {"Mutton Tehari", 2}, {"Borhan", 1}, {"Salad", 1}}};

    // Mymensingh restaurants
    Restaurant r10 = {"Mama Halim", "Mymensingh", {"Beef Halim", "Chicken Halim", "Mutton Halim", "Naan", "Salad"}, {{"Beef Halim", 120}, {"Chicken Halim", 100}, {"Mutton Halim", 150}, {"Naan", 15}, {"Salad", 20}}, {{"Beef Halim", 2}, {"Chicken Halim", 2}, {"Mutton Halim", 2}, {"Naan", 1}, {"Salad", 1}}};

    // Comilla restaurants
    Restaurant r11 = {"Rosmalai House", "Comilla", {"Rosmalai", "Chomchom", "Misti Doi", "Pantua", "Jilapi"}, {{"Rosmalai", 150}, {"Chomchom", 120}, {"Misti Doi", 80}, {"Pantua", 100}, {"Jilapi", 60}}, {{"Rosmalai", 1}, {"Chomchom", 1}, {"Misti Doi", 1}, {"Pantua", 1}, {"Jilapi", 1}}};

    // Cox's Bazar restaurants
    Restaurant r12 = {"Sea Pearl", "Cox's Bazar", {"Grilled Fish", "Fried Prawns", "Crab Curry", "Rice", "Salad"}, {{"Grilled Fish", 400}, {"Fried Prawns", 350}, {"Crab Curry", 500}, {"Rice", 50}, {"Salad", 30}}, {{"Grilled Fish", 3}, {"Fried Prawns", 2}, {"Crab Curry", 4}, {"Rice", 1}, {"Salad", 1}}};

    restaurants.push_back(r1);
    restaurants.push_back(r2);
    restaurants.push_back(r3);
    restaurants.push_back(r4);
    restaurants.push_back(r5);
    restaurants.push_back(r6);
    restaurants.push_back(r7);
    restaurants.push_back(r8);
    restaurants.push_back(r9);
    restaurants.push_back(r10);
    restaurants.push_back(r11);
    restaurants.push_back(r12);
}

int main()
{
    initializeRestaurants();
    loadUserData();
    loadOrderHistory();

    while (true)
    {
        cout << "\nMain Menu:\n";
        cout << "1. Register\n";
        cout << "2. Login\n";
        cout << "3. Admin Login\n";
        cout << "4. Exit\n";
        cout << "Choice: ";

        int choice;
        while (!(cin >> choice) || choice < 1 || choice > 4)
        {
            cout << "Invalid input: ";
            cin.clear();
            cin.ignore(10000, '\n');
        }

        switch (choice)
        {
        case 1:
            registerUser();
            break;
        case 2:
        {
            int loggedInUser = loginUser();
            if (loggedInUser >= 0)
            {
                userMenu(loggedInUser);
            }
            break;
        }
        case 3:
            if (adminLogin())
            {
                adminMenu();
            }
            else
            {
                cout << "Admin login failed!\n";
            }
            break;
        case 4:
            saveUserData();
            saveActiveOrders();
            saveCompletedOrders();
            savePerformanceData(); // Save performance data before exiting
            return 0;
        }
    }
}