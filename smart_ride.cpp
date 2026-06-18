/*
 * ======================================================================================
 * PROJECT: INTELLIGENT RIDE MANAGEMENT SYSTEM (ISLAMABAD)
 * ARCHITECTURE: Modular Monolith
 * FEATURES: Graph, Hashing, Multithreading, Physics, Ratings, History, Hotspot Monitor
 * ======================================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <chrono>
#include <limits>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <climits>

using namespace std;

// Global Mutex for Thread-Safe Printing
mutex printLock;

/*
 * ======================================================================================
 * MODULE 1: VEHICLE DATABASE & SELECTION LOGIC
 * ======================================================================================
 */

struct VehicleInfo {
    string name;
    string category;
    double ratePerKm;
};

// Static Database
vector<VehicleInfo> vehicleDB = {
    {"Mehran", "Executive", 40},
    {"Alto", "Executive", 45},
    {"WagonR", "Executive", 48},
    {"Cultus", "Executive", 50},
    {"Santro", "Executive", 47},
    {"Honda City", "Business", 65},
    {"Civic", "Business", 80},
    {"BRV", "Business", 75},
    {"Yaris", "Business", 70},
    {"Toyota Corolla", "Business", 72}
};

VehicleInfo getVehicleSelection() {
    string name;
    cout << "\n========== SELECT VEHICLE ==========\n";
    cout << "EXECUTIVE CARS:\n";
    for (int i = 0; i < 5; i++) {
        cout << (i+1) << ". " << vehicleDB[i].name << "\n";
    }
    cout << "\nBUSINESS CARS:\n";
    for (int i = 5; i < 10; i++) {
        cout << (i+1) << ". " << vehicleDB[i].name << "\n";
    }
    cout << "\n11. Other (Will be classified as Business)\n";

    int carChoice;
    cout << "\nEnter choice (1-11): ";
    while (!(cin >> carChoice) || carChoice < 1 || carChoice > 11) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid! Enter 1-11: ";
    }

    if (carChoice == 11) {
        cin.ignore();
        cout << "Enter your vehicle name: ";
        getline(cin, name);
        return {name, "Business", 75.0};
    } else {
        return vehicleDB[carChoice - 1];
    }
}

/*
 * ======================================================================================
 * MODULE 2: COMPLEX COMPUTING - FARE ENGINE
 * ======================================================================================
 */

double calculateTotalFare(int distance, double rate, int priority, string &outMsg) {
    double total = distance * rate;
    outMsg = "";
    if (priority == 1) {
        total = total * 1.5;
        outMsg = " (Emergency Surge 1.5x)";
    }
    return total;
}

/*
 * ======================================================================================
 * MODULE 3: GRAPH DATA STRUCTURES
 * ======================================================================================
 */

struct Edge {
    int to;
    int distance;
    int speedLimit;
};

struct Node {
    int id;
    string name;
    int areaID;
};

/*
 * ======================================================================================
 * MODULE 4: GRAPH ALGORITHMS & PHYSICS ENGINE
 * ======================================================================================
 */

class Graph {
public:
    vector<Node> nodes;
    vector<vector<Edge>> adjList;
    vector<string> areaNames;

    Graph() {
        areaNames = {
            "Comsats Area", "Hostel City", "Chatta Bakhtawar", "Chak Shahzad",
            "Park Road Zone", "Taramri Area", "Bahria Enclave", "Rawal Dam Zone"
        };
    }

    void addNode(int id, string name, int areaID) {
        nodes.push_back({id, name, areaID});
        if (adjList.size() <= id) adjList.resize(id + 1);
    }

    void addEdge(int u, int v, int dist, int speed) {
        adjList[u].push_back({v, dist, speed});
        adjList[v].push_back({u, dist, speed});
    }

    void displayMap() {
        lock_guard<mutex> guard(printLock);
        cout << "\n=========== CITY MAP ===========\n";
        for (auto &n : nodes) {
            cout << "Node " << n.id << " | " << n.name << " | Area: " << areaNames[n.areaID] << "\n";
        }
    }

    int getAreaFromNode(int nodeID) {
        if (nodeID >= 0 && nodeID < nodes.size())
            return nodes[nodeID].areaID;
        return -1;
    }

    pair<int, vector<int>> dijkstra(int source, int destination) {
        int n = nodes.size();
        vector<int> distance(n, INT_MAX);
        vector<int> parent(n, -1);
        vector<bool> visited(n, false);

        distance[source] = 0;

        for (int count = 0; count < n - 1; count++) {
            int minDist = INT_MAX;
            int u = -1;
            for (int v = 0; v < n; v++) {
                if (!visited[v] && distance[v] < minDist) {
                    minDist = distance[v];
                    u = v;
                }
            }
            if (u == -1) break;

            visited[u] = true;

            for (auto &edge : adjList[u]) {
                int v = edge.to;
                int weight = edge.distance;
                if (!visited[v] && distance[u] != INT_MAX && distance[u] + weight < distance[v]) {
                    distance[v] = distance[u] + weight;
                    parent[v] = u;
                }
            }
        }

        vector<int> path;
        if (distance[destination] == INT_MAX)
            return {INT_MAX, path};

        int current = destination;
        while (current != -1) {
            path.push_back(current);
            current = parent[current];
        }
        reverse(path.begin(), path.end());

        return {distance[destination], path};
    }

    int calculatePathTime(const vector<int>& path) {
        double totalTime = 0;
        if (path.empty()) return 0;

        for (size_t i = 0; i < path.size() - 1; ++i) {
            int u = path[i];
            int v = path[i+1];
            for (auto& edge : adjList[u]) {
                if (edge.to == v) {
                    totalTime += ((double)edge.distance / edge.speedLimit) * 120.0;
                    break;
                }
            }
        }
        return max(5, (int)totalTime);
    }

    vector<int> getNodesByArea(int areaID) {
        vector<int> result;
        for (auto &n : nodes) {
            if (n.areaID == areaID) result.push_back(n.id);
        }
        return result;
    }
};

Graph city;

/*
 * ======================================================================================
 * MODULE 5: RIDE HISTORY (AVL TREE)
 * ======================================================================================
 */

struct RideHistory {
    int pickupNode;
    int dropNode;
    double fare;
    time_t rideTime;
};

struct HistoryNode {
    RideHistory data;
    HistoryNode* left;
    HistoryNode* right;
    int height;

    HistoryNode(RideHistory r) {
        data = r;
        left = right = nullptr;
        height = 1;
    }
};

int historyHeight(HistoryNode* node) {
    return node ? node->height : 0;
}

int historyBalance(HistoryNode* node) {
    return node ? historyHeight(node->left) - historyHeight(node->right) : 0;
}

HistoryNode* historyRightRotate(HistoryNode* y) {
    HistoryNode* x = y->left;
    HistoryNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(historyHeight(y->left), historyHeight(y->right)) + 1;
    x->height = max(historyHeight(x->left), historyHeight(x->right)) + 1;
    return x;
}

HistoryNode* historyLeftRotate(HistoryNode* x) {
    HistoryNode* y = x->right;
    HistoryNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(historyHeight(x->left), historyHeight(x->right)) + 1;
    y->height = max(historyHeight(y->left), historyHeight(y->right)) + 1;
    return y;
}

HistoryNode* insertRideHistory(HistoryNode* root, RideHistory r) {
    if (!root) return new HistoryNode(r);

    if (r.rideTime < root->data.rideTime)
        root->left = insertRideHistory(root->left, r);
    else
        root->right = insertRideHistory(root->right, r);

    root->height = 1 + max(historyHeight(root->left), historyHeight(root->right));

    int balance = historyBalance(root);

    // LL Case
    if (balance > 1 && r.rideTime < root->left->data.rideTime)
        return historyRightRotate(root);

    // RR Case
    if (balance < -1 && r.rideTime > root->right->data.rideTime)
        return historyLeftRotate(root);

    // LR Case
    if (balance > 1 && r.rideTime > root->left->data.rideTime) {
        root->left = historyLeftRotate(root->left);
        return historyRightRotate(root);
    }

    // RL Case
    if (balance < -1 && r.rideTime < root->right->data.rideTime) {
        root->right = historyRightRotate(root->right);
        return historyLeftRotate(root);
    }

    return root;
}

void displayRideHistory(HistoryNode* root) {
    if (!root) return;
    displayRideHistory(root->left);

    string pickupName = (root->data.pickupNode < city.nodes.size()) ?
                        city.nodes[root->data.pickupNode].name : "Unknown";
    string dropName = (root->data.dropNode < city.nodes.size()) ?
                      city.nodes[root->data.dropNode].name : "Unknown";

    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&root->data.rideTime));

    cout << left << setw(10) << root->data.pickupNode
         << setw(10) << root->data.dropNode
         << setw(15) << fixed << setprecision(2) << root->data.fare
         << timeStr << "\n";
    cout << "  " << pickupName << " → " << dropName << "\n\n";

    displayRideHistory(root->right);
}

/*
 * ======================================================================================
 * MODULE 6: DRIVER ENTITY
 * ======================================================================================
 */

struct DriverNode {
    int driverID;
    string name;
    string vehicleInfo;
    string vehicleCategory;
    double ratePerKm;
    string status;
    int currentAreaID;
    int lastKnownNode;
    int rideCount;
    HistoryNode* historyRoot;

    // RATING STATS
    double avgRating;
    int totalRatings;
    int badRatingStreak;

    // Auth Lock
    string currentCustomer;

    DriverNode* next;

    DriverNode(int id, string n, string car, string cat, double rate, int area, int node) {
        driverID = id;
        name = n;
        vehicleInfo = car;
        vehicleCategory = cat;
        ratePerKm = rate;
        status = "Available";
        currentAreaID = area;
        lastKnownNode = node;
        rideCount = 0;
        historyRoot = nullptr;
        avgRating = 5.0;
        totalRatings = 0;
        badRatingStreak = 0;
        currentCustomer = "";
        next = nullptr;
    }
};

/*
 * ======================================================================================
 * MODULE 7: DRIVER MANAGEMENT & HASHING LOGIC WITH FILE HANDLING
 * ======================================================================================
 */

class DriverList {
private:
    DriverNode* head;
    unordered_map<int, DriverNode*> driverIndex;

    // >>> MODULE 11 STORAGE: Fleet Supply Hash Tables (Hashed by Area ID) <<<
    unordered_map<int, int> areaDriverCounts;
    unordered_map<int, int> areaRequestCounts;

    const string DRIVER_FILE = "drivers.txt";
    const string HISTORY_FILE = "ride_history.txt";

    // --- NEW HELPER FUNCTION FOR HOTSPOT MONITOR ---
    // Finds the first available driver in a given area.
    DriverNode* getAvailableDriverByArea(int areaID) {
        DriverNode* temp = head;
        while (temp) {
            if (temp->status == "Available" && temp->currentAreaID == areaID) {
                return temp; // Found a target driver to move
            }
            temp = temp->next;
        }
        return nullptr;
    }
    // -----------------------------------------------

    // Helper: Save ride history to file (In-order traversal)
    void saveHistoryToFile(ofstream &out, HistoryNode* node) {
        if (!node) return;

        saveHistoryToFile(out, node->left);

        // Write ride data in readable format
        out << node->data.pickupNode << ","
            << node->data.dropNode << ","
            << node->data.fare << ","
            << node->data.rideTime << "\n";

        saveHistoryToFile(out, node->right);
    }

    // Helper: Load ride history from file
    HistoryNode* loadHistoryFromFile(ifstream &in, int count) {
        HistoryNode* root = nullptr;

        for (int i = 0; i < count; i++) {
            RideHistory h;
            char comma;
            in >> h.pickupNode >> comma
               >> h.dropNode >> comma
               >> h.fare >> comma
               >> h.rideTime;
            in.ignore(); // ignore newline

            root = insertRideHistory(root, h);
        }

        return root;
    }

    // Helper: Count nodes in history tree
    int countHistoryNodes(HistoryNode* node) {
        if (!node) return 0;
        return 1 + countHistoryNodes(node->left) + countHistoryNodes(node->right);
    }

public:
    DriverList() {
        head = nullptr;
        loadFromFile(); // Auto-load on startup
    }

    ~DriverList() {
        saveToFile(); // Auto-save on exit
    }

    /*
     * ======================================================================================
     * MODULE 11: INTELLIGENT FLEET MANAGEMENT (HOTSPOT MONITOR) - FIXED LOGIC
     * ======================================================================================
     */
    void monitorAreaLoad(int requestArea) {
        // Increment Request Count for this area
        areaRequestCounts[requestArea]++;

        int availableDrivers = areaDriverCounts[requestArea];
        int pendingRequests = areaRequestCounts[requestArea];

        // Formula: Load Factor = Requests / (Drivers + 1)
        double loadFactor = (double)pendingRequests / (availableDrivers + 1);

        cout << "\n[SYSTEM MONITOR] Area: " << city.areaNames[requestArea]
             << " | Drivers: " << availableDrivers
             << " | Requests: " << pendingRequests
             << " | Load Factor: " << setprecision(2) << loadFactor << "\n";

        // THRESHOLD CHECK (Hotspot Value = 4.0)
        if (loadFactor >= 4.0) {
            cout << "🚨 [CRITICAL WARNING] HIGH DEMAND DETECTED!\n";
            cout << "   Action: Attempting to rebalance fleet...\n";

            // Logic: Find neighbor with surplus (using cyclic previous area as a simple neighbor check)
            int neighborID = (requestArea == 0) ? city.areaNames.size() - 1 : requestArea - 1;

            // Only attempt to move if the neighbor has at least 2 drivers (to maintain 1 driver minimum)
            if (areaDriverCounts[neighborID] > 1) {
                DriverNode* driverToMove = getAvailableDriverByArea(neighborID);

                if (driverToMove) {
                    // Find a suitable node in the target area for the driver to relocate to
                    auto targetNodes = city.getNodesByArea(requestArea);

                    // Simple logic: move to the first available node in the target area
                    int newTargetNode = targetNodes.empty() ? -1 : targetNodes[0];

                    if (newTargetNode != -1) {
                        // 1. Update Supply Hash Table (Source Area: DECREMENT)
                        areaDriverCounts[neighborID]--;

                        // 2. Update Driver Location in the Linked List
                        driverToMove->lastKnownNode = newTargetNode;
                        driverToMove->currentAreaID = requestArea;

                        // 3. Update Supply Hash Table (Destination Area: INCREMENT)
                        areaDriverCounts[requestArea]++;

                        cout << "   ✓ Rebalanced: Driver **" << driverToMove->name
                             << "** moved from **" << city.areaNames[neighborID]
                             << "** to **" << city.areaNames[requestArea] << "** (Node " << newTargetNode << ").\n";
                    } else {
                        cout << "   ⚠ Cannot find target node for relocation in Area " << city.areaNames[requestArea] << ".\n";
                    }
                } else {
                     cout << "   ⚠ Driver count is > 1 but no 'Available' driver found in neighbor area.\n";
                }
            } else {
                cout << "   ⚠ Neighbor Area (" << city.areaNames[neighborID] << ") also low on supply. Cannot rebalance.\n";
            }
        }

        // Reset request count logic to prevent infinite growth in this simulation
        // (Simulating that we 'processed' the check)
        // areaRequestCounts[requestArea]--;
    }

    // Save all drivers and their history to file (HUMAN READABLE)
    void saveToFile() {
        ofstream driverFile(DRIVER_FILE);
        ofstream historyFile(HISTORY_FILE);

        if (!driverFile.is_open() || !historyFile.is_open()) {
            cout << "⚠ Warning: Could not save data to file.\n";
            return;
        }

        // Count total drivers
        int driverCount = 0;
        DriverNode* temp = head;
        while (temp) {
            driverCount++;
            temp = temp->next;
        }

        // Write driver count
        driverFile << driverCount << "\n";
        driverFile << "========================================\n";

        // Write each driver
        temp = head;
        while (temp) {
            driverFile << "DriverID: " << temp->driverID << "\n";
            driverFile << "Name: " << temp->name << "\n";
            driverFile << "Vehicle: " << temp->vehicleInfo << "\n";
            driverFile << "Category: " << temp->vehicleCategory << "\n";
            driverFile << "RatePerKm: " << temp->ratePerKm << "\n";
            driverFile << "CurrentArea: " << temp->currentAreaID << "\n";
            driverFile << "LastNode: " << temp->lastKnownNode << "\n";
            driverFile << "RideCount: " << temp->rideCount << "\n";
            driverFile << "AvgRating: " << fixed << setprecision(2) << temp->avgRating << "\n";
            driverFile << "TotalRatings: " << temp->totalRatings << "\n";
            driverFile << "BadStreak: " << temp->badRatingStreak << "\n";
            driverFile << "========================================\n";

            // Write history count and data
            int historyCount = countHistoryNodes(temp->historyRoot);
            historyFile << "DriverID: " << temp->driverID << "\n";
            historyFile << "HistoryCount: " << historyCount << "\n";
            if (historyCount > 0) {
                historyFile << "Pickup,Drop,Fare,Timestamp\n";
                saveHistoryToFile(historyFile, temp->historyRoot);
            }
            historyFile << "========================================\n";

            temp = temp->next;
        }

        driverFile.close();
        historyFile.close();

        cout << "✓ Data saved successfully! (" << driverCount << " drivers)\n";
    }

    // Load all drivers and their history from file (HUMAN READABLE)
    void loadFromFile() {
        ifstream driverFile(DRIVER_FILE);
        ifstream historyFile(HISTORY_FILE);

        if (!driverFile.is_open()) {
            cout << "ℹ No previous data found. Starting fresh.\n";
            return;
        }

        int driverCount;
        driverFile >> driverCount;
        driverFile.ignore(); // ignore newline

        string line;
        getline(driverFile, line); // skip separator line

        for (int i = 0; i < driverCount; i++) {
            int id, area, node, rideCount, totalRatings, badStreak;
            double rate, avgRating;
            string name, vehicle, category;

            // Read driver data
            driverFile.ignore(100, ':'); // skip "DriverID:"
            driverFile >> id;
            driverFile.ignore();

            driverFile.ignore(100, ':'); // skip "Name:"
            getline(driverFile, name);

            driverFile.ignore(100, ':'); // skip "Vehicle:"
            getline(driverFile, vehicle);

            driverFile.ignore(100, ':'); // skip "Category:"
            getline(driverFile, category);

            driverFile.ignore(100, ':'); // skip "RatePerKm:"
            driverFile >> rate;

            driverFile.ignore(100, ':'); // skip "CurrentArea:"
            driverFile >> area;

            driverFile.ignore(100, ':'); // skip "LastNode:"
            driverFile >> node;

            driverFile.ignore(100, ':'); // skip "RideCount:"
            driverFile >> rideCount;

            driverFile.ignore(100, ':'); // skip "AvgRating:"
            driverFile >> avgRating;

            driverFile.ignore(100, ':'); // skip "TotalRatings:"
            driverFile >> totalRatings;

            driverFile.ignore(100, ':'); // skip "BadStreak:"
            driverFile >> badStreak;

            driverFile.ignore(); // ignore newline
            getline(driverFile, line); // skip separator line

            // Create driver node
            DriverNode* newDriver = new DriverNode(id, name, vehicle, category, rate, area, node);
            newDriver->rideCount = rideCount;
            newDriver->avgRating = avgRating;
            newDriver->totalRatings = totalRatings;
            newDriver->badRatingStreak = badStreak;

            // Add to list
            if (!head)
                head = newDriver;
            else {
                DriverNode* temp = head;
                while (temp->next) temp = temp->next;
                temp->next = newDriver;
            }

            driverIndex[id] = newDriver;
            // Update Supply Count for Loaded Driver
            areaDriverCounts[area]++;
        }

        driverFile.close();

        // Load history
        if (historyFile.is_open()) {
            while (historyFile.peek() != EOF) {
                int driverID, historyCount;

                historyFile.ignore(100, ':'); // skip "DriverID:"
                if (historyFile >> driverID) {
                    historyFile.ignore(100, ':'); // skip "HistoryCount:"
                    historyFile >> historyCount;
                    historyFile.ignore(); // skip newline

                    if (historyCount > 0) {
                        getline(historyFile, line); // skip header line

                        if (driverIndex.find(driverID) != driverIndex.end()) {
                            driverIndex[driverID]->historyRoot = loadHistoryFromFile(historyFile, historyCount);
                        } else {
                            // Skip this driver's history
                            for (int i = 0; i < historyCount; i++) {
                                getline(historyFile, line);
                            }
                        }
                    }

                    getline(historyFile, line); // skip separator line
                }
            }
            historyFile.close();
        }

        cout << "✓ Loaded " << driverCount << " drivers from previous session.\n";
    }

    void registerDriver() {
        lock_guard<mutex> guard(printLock);
        int id, area, startNode;
        string name;

        cout << "Enter Driver ID: ";
        while (!(cin >> id)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Enter valid integer: ";
        }
        cin.ignore();

        cout << "Enter Name: ";
        getline(cin, name);

        VehicleInfo vInfo = getVehicleSelection();

        cout << "\nAvailable Areas:\n";
        for (int i = 0; i < city.areaNames.size(); i++)
            cout << i << ". " << city.areaNames[i] << "\n";

        cout << "Enter Driver's Current Area (0-7): ";
        while (!(cin >> area) || area < 0 || area > 7) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid! Enter 0-7: ";
        }

        auto nodesInArea = city.getNodesByArea(area);
        cout << "Nodes in this area:\n";
        for (int i = 0; i < nodesInArea.size(); i++)
            cout << i << ". Node " << nodesInArea[i] << " - " << city.nodes[nodesInArea[i]].name << "\n";

        int nodeChoice;
        cout << "Select starting node index (0-" << nodesInArea.size()-1 << "): ";
        while (!(cin >> nodeChoice) || nodeChoice < 0 || nodeChoice >= nodesInArea.size()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid! Select node index (0-" << nodesInArea.size()-1 << "): ";
        }
        startNode = nodesInArea[nodeChoice];

        DriverNode* newDriver = new DriverNode(id, name, vInfo.name, vInfo.category, vInfo.ratePerKm, area, startNode);

        if (!head)
            head = newDriver;
        else {
            DriverNode* temp = head;
            while (temp->next) temp = temp->next;
            temp->next = newDriver;
        }

        driverIndex[id] = newDriver;

        // --- MODULE 11: UPDATE SUPPLY HASH TABLE ---
        areaDriverCounts[area]++;

        cout << "\n✓ Driver Registered Successfully!\n";
        cout << " Vehicle: " << vInfo.name << " (" << vInfo.category << ")\n";
        cout << " Location: Node " << startNode << " (" << city.nodes[startNode].name << ")\n";
        cout << " Area: " << city.areaNames[area] << "\n";
    }

    DriverNode* findDriver(int id) {
        if (driverIndex.find(id) != driverIndex.end())
            return driverIndex[id];
        return nullptr;
    }

    void displayDrivers() {
        lock_guard<mutex> guard(printLock);
        if (!head) {
            cout << "\nNo drivers registered.\n";
            return;
        }

        DriverNode* temp = head;
        cout << "\n========== REGISTERED DRIVERS ==========\n";
        cout << left << setw(5) << "ID" << setw(15) << "Name" << setw(15) << "Vehicle"
             << setw(12) << "Category" << setw(12) << "Status" << setw(6) << "Node"
             << setw(8) << "Rating" << "\n";
        cout << string(80, '-') << "\n";

        while (temp) {
            cout << left << setw(5) << temp->driverID << setw(15) << temp->name
                 << setw(15) << temp->vehicleInfo << setw(12) << temp->vehicleCategory
                 << setw(12) << temp->status << setw(6) << temp->lastKnownNode
                 << setw(8) << fixed << setprecision(2) << temp->avgRating << "\n";
            cout << " Current: " << city.nodes[temp->lastKnownNode].name
                 << " (" << city.areaNames[temp->currentAreaID] << ")\n";
            temp = temp->next;
        }
    }

    DriverNode* getNearestDriver(int pickupNode, string requestedCategory, int &outDistance) {
        DriverNode* temp = head;
        DriverNode* nearest = nullptr;
        int minDistance = INT_MAX;

        cout << "\n[Searching for nearest available " << requestedCategory << " driver...]\n";

        while (temp) {
            if (temp->status == "Available" && temp->vehicleCategory == requestedCategory) {
                auto res = city.dijkstra(temp->lastKnownNode, pickupNode);
                if (res.first != INT_MAX) {
                    cout << " -> Driver " << temp->name << " (Node " << temp->lastKnownNode
                         << "): Distance = " << res.first << " km\n";
                    if (res.first < minDistance) {
                        minDistance = res.first;
                        nearest = temp;
                        outDistance = res.first;
                    }
                }
            }
            temp = temp->next;
        }
        return nearest;
    }

    void updateDriverStatus(int id, string newStatus) {
        if (driverIndex.find(id) != driverIndex.end()) {
            // --- MODULE 11: DYNAMIC SUPPLY UPDATE ---
            // If becoming Busy, reduce supply
            if (newStatus == "Busy" && driverIndex[id]->status == "Available") {
                areaDriverCounts[driverIndex[id]->currentAreaID]--;
            }
            driverIndex[id]->status = newStatus;
        }
    }

    void setRidePendingRating(int id, int dropNode) {
        if (driverIndex.find(id) != driverIndex.end()) {
            DriverNode* driver = driverIndex[id];
            driver->currentAreaID = city.getAreaFromNode(dropNode);
            driver->lastKnownNode = dropNode;
            driver->rideCount++;
            driver->status = "RATING_DUE";
        }
    }

    void setCustomerForDriver(int id, string customerName) {
        if (driverIndex.find(id) != driverIndex.end()) {
            driverIndex[id]->currentCustomer = customerName;
        }
    }

    void deleteDriver(int id) {
        if (!head) return;

        if (driverIndex.find(id) != driverIndex.end()) {
            // Supply Update before delete
            areaDriverCounts[driverIndex[id]->currentAreaID]--;
            driverIndex.erase(id);
        }

        DriverNode* temp = head;

        if (temp->driverID == id) {
            head = head->next;
            delete temp;
            return;
        }

        DriverNode* prev = nullptr;
        while (temp && temp->driverID != id) {
            prev = temp;
            temp = temp->next;
        }

        if (temp) {
            prev->next = temp->next;
            delete temp;
        }
    }

    void processPendingRatings() {
        lock_guard<mutex> guard(printLock);
        string verifyName;
        cin.ignore();
        cout << "\n🔒 SECURITY CHECK: Enter your Passenger Name: ";
        getline(cin, verifyName);

        DriverNode* temp = head;
        bool found = false;
        int driverIDToDelete = -1;

        cout << "\n========== PENDING RATINGS FOR: " << verifyName << " ==========\n";

        while(temp) {
            if (temp->status == "RATING_DUE" && temp->currentCustomer == verifyName) {
                found = true;
                cout << "Driver " << temp->name << " (ID: " << temp->driverID << ") finished a ride.\n";

                int rating;
                cout << "Rate this driver (1-5): ";
                while(!(cin >> rating) || rating < 1 || rating > 5) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid! Enter 1-5: ";
                }

                double currentSum = temp->avgRating * temp->totalRatings;
                temp->totalRatings++;
                temp->avgRating = (currentSum + rating) / temp->totalRatings;

                if (rating <= 2) {
                    temp->badRatingStreak++;
                    cout << "⚠ WARNING: Bad rating recorded! Streak: " << temp->badRatingStreak << "/3\n";
                } else {
                    temp->badRatingStreak = 0;
                }

                if (temp->badRatingStreak >= 3) {
                    cout << "🚫 SYSTEM ALERT: Driver " << temp->name << " has been BANNED and DELETED from the system.\n";
                    driverIDToDelete = temp->driverID;
                } else {
                    temp->status = "Available";
                    // --- MODULE 11: RESTORE SUPPLY ---
                    areaDriverCounts[temp->currentAreaID]++;
                    cout << "✓ Thank you! Driver is now Available.\n";
                }

                temp->currentCustomer = "";
                break;
            }
            temp = temp->next;
        }

        if (driverIDToDelete != -1) {
            deleteDriver(driverIDToDelete);
        }

        if (!found)
            cout << "No pending ratings found for passenger '" << verifyName << "'.\n";
        cout << "========================================================\n";
    }

    void viewDriverHistory() {
        lock_guard<mutex> guard(printLock);
        int driverID;

        cout << "\n========== VIEW DRIVER RIDE HISTORY ==========\n";
        cout << "Enter Driver ID: ";
        while (!(cin >> driverID)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid! Enter Driver ID: ";
        }

        DriverNode* driver = findDriver(driverID);

        if (!driver) {
            cout << "\n✗ ERROR: Driver with ID " << driverID << " not found!\n";
            return;
        }

        cout << "\n========================================\n";
        cout << "Driver: " << driver->name << " (ID: " << driver->driverID << ")\n";
        cout << "Vehicle: " << driver->vehicleInfo << " (" << driver->vehicleCategory << ")\n";
        cout << "Average Rating: " << fixed << setprecision(2) << driver->avgRating << "/5.0\n";
        cout << "Total Ratings: " << driver->totalRatings << "\n";
        cout << "Total Rides: " << driver->rideCount << "\n";
        cout << "========================================\n";

        if (!driver->historyRoot) {
            cout << "\nNo ride history available for this driver.\n";
        } else {
            cout << "\n--- RIDE HISTORY (Chronological Order) ---\n";
            cout << left << setw(10) << "Pickup" << setw(10) << "Drop"
                 << setw(15) << "Fare (Rs)" << "Date & Time\n";
            cout << string(70, '-') << "\n";
            displayRideHistory(driver->historyRoot);
        }
        cout << "========================================\n";
    }

};

/*
 * ======================================================================================
 * MODULE 8: THREAD SIMULATION
 * ======================================================================================
 */

void rideSimulationThread(DriverList* drivers, int driverID, int pickupNode, int dropNode, double fare, int durationSeconds) {
    this_thread::sleep_for(chrono::seconds(durationSeconds));

    DriverNode* driver = drivers->findDriver(driverID);
    if (!driver) return;

    RideHistory h;
    h.pickupNode = pickupNode;
    h.dropNode = dropNode;
    h.fare = fare;
    h.rideTime = time(nullptr);

    driver->historyRoot = insertRideHistory(driver->historyRoot, h);

    {
        lock_guard<mutex> guard(printLock);
        cout << "\n\n*\n";
        cout << " 🔔 RIDE COMPLETED! 🔔\n";
        cout << "*\n";
        cout << "Driver (ID: " << driverID << ") reached Node " << dropNode << ".\n";
        cout << "Fare: Rs " << fare << "\n";
        cout << "Status: WAITING FOR RATING\n";
        cout << "*\n";
    }

    drivers->setRidePendingRating(driverID, dropNode);
}

/*
 * ======================================================================================
 * MODULE 9: PASSENGER & RIDE LOGIC
 * ======================================================================================
 */

struct RideRequest {
    int requestID;
    string customerName;
    int pickupNode;
    int dropoffNode;
    int priority;
    string requestedCategory;
    int assignedDriverID;
    double fare;
};

queue<RideRequest> normalQueue;

struct ComparePriority {
    bool operator()(RideRequest const &r1, RideRequest const &r2) {
        return r1.priority < r2.priority;
    }
};

priority_queue<RideRequest, vector<RideRequest>, ComparePriority> emergencyQueue;

int requestCounter = 1;

void assignDriver(DriverList &drivers);

void customerRequest(DriverList &drivers) {
    RideRequest ride;
    ride.requestID = requestCounter++;

    cin.ignore();
    cout << "\n========== NEW RIDE REQUEST ==========\n";
    cout << "Customer Name: ";
    getline(cin, ride.customerName);

    cout << "\nAvailable Areas:\n";
    for (int i = 0; i < city.areaNames.size(); i++)
        cout << i << ". " << city.areaNames[i] << "\n";

    cout << "\nPickup Area (0-7): ";
    int area;
    while (!(cin >> area) || area < 0 || area > 7) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid! Enter 0-7: ";
    }

    // --- MODULE 11 CALL: MONITOR LOAD ---
    drivers.monitorAreaLoad(area); // Check for Hotspots immediately

    auto nodesInArea = city.getNodesByArea(area);
    cout << "\nAvailable pickup locations in " << city.areaNames[area] << ":\n";
    for (int i = 0; i < nodesInArea.size(); i++)
        cout << i << ". Node " << nodesInArea[i] << " - " << city.nodes[nodesInArea[i]].name << "\n";

    int nodeChoice;
    cout << "Select pickup location (0-" << nodesInArea.size()-1 << "): ";
    while (!(cin >> nodeChoice) || nodeChoice < 0 || nodeChoice >= nodesInArea.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid! Select node index: ";
    }
    ride.pickupNode = nodesInArea[nodeChoice];

    cout << "\nDropoff Node (0-" << city.nodes.size()-1 << "): ";
    while (!(cin >> ride.dropoffNode) || ride.dropoffNode < 0 || ride.dropoffNode >= city.nodes.size()) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid! Enter dropoff node: ";
    }

    cout << "\nSelect Ride Type:\n1. Executive\n2. Business\nChoice: ";
    int categoryChoice;
    while (!(cin >> categoryChoice) || (categoryChoice != 1 && categoryChoice != 2)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid! Enter 1 or 2: ";
    }
    ride.requestedCategory = (categoryChoice == 1) ? "Executive" : "Business";

    cout << "Priority (0=Normal, 1=Emergency): ";
    while (!(cin >> ride.priority) || (ride.priority != 0 && ride.priority != 1)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter 0 or 1: ";
    }

    ride.assignedDriverID = -1;

    if (ride.priority == 0)
        normalQueue.push(ride);
    else
        emergencyQueue.push(ride);

    cout << "\n✓ Ride Request Added to " << (ride.priority == 1 ? "EMERGENCY" : "Normal") << " Queue!\n";
    cout << " Pickup: " << city.nodes[ride.pickupNode].name << " (Node " << ride.pickupNode << ")\n";
    cout << " Dropoff: " << city.nodes[ride.dropoffNode].name << " (Node " << ride.dropoffNode << ")\n";
    cout << " Requested Category: " << ride.requestedCategory << "\n";

    assignDriver(drivers);
}

void assignDriver(DriverList &drivers) {
    lock_guard<mutex> guard(printLock);

    RideRequest ride;
    if (!emergencyQueue.empty()) {
        ride = emergencyQueue.top();
        emergencyQueue.pop();
    } else if (!normalQueue.empty()) {
        ride = normalQueue.front();
        normalQueue.pop();
    } else {
        cout << "\nNo pending rides in queue.\n";
        return;
    }

    int dist = 0;
    DriverNode* driver = drivers.getNearestDriver(ride.pickupNode, ride.requestedCategory, dist);

    if (!driver) {
        cout << "\n✗ ERROR: No available " << ride.requestedCategory << " drivers found!\n";
        return;
    }

    ride.assignedDriverID = driver->driverID;

    auto tripInfo = city.dijkstra(ride.pickupNode, ride.dropoffNode);
    int tripDistance = tripInfo.first;
    vector<int> path = tripInfo.second;

    string surgeMsg;
    ride.fare = calculateTotalFare(tripDistance, driver->ratePerKm, ride.priority, surgeMsg);
    int duration = city.calculatePathTime(path);

    drivers.updateDriverStatus(driver->driverID, "Busy");
    drivers.setCustomerForDriver(driver->driverID, ride.customerName);

    cout << "\n==================================================\n";
    cout << " RIDE ASSIGNMENT SUCCESSFUL\n";
    cout << "==================================================\n";
    cout << "Customer: " << ride.customerName << "\n";
    cout << "Driver: " << driver->name << " (ID: " << driver->driverID << ")\n";
    cout << "Vehicle: " << driver->vehicleInfo << " (" << driver->vehicleCategory << ")\n";
    cout << "Rate: Rs " << driver->ratePerKm << "/km\n";
    cout << "Trip Distance: " << tripDistance << " km\n";
    cout << "Total Fare: Rs " << ride.fare << surgeMsg << "\n";
    cout << "Est Time: " << duration << " seconds (Simulation)\n";
    cout << "--------------------------------------------------\n";

    cout << "\n>>> DRIVER NAVIGATION TO PICKUP <<<\n";
    auto toPickup = city.dijkstra(driver->lastKnownNode, ride.pickupNode);
    if (!toPickup.second.empty()) {
        for (size_t i = 0; i < toPickup.second.size(); i++) {
            cout << " " << (i+1) << ". " << city.nodes[toPickup.second[i]].name;
            if (i < toPickup.second.size() - 1)
                cout << "\n |\n v\n";
            else
                cout << " [PICKUP CUSTOMER]\n";
        }
    }

    cout << "\n>>> CUSTOMER JOURNEY TO DESTINATION <<<\n";
    if (!path.empty()) {
        for (size_t i = 0; i < path.size(); i++) {
            cout << " " << (i+1) << ". " << city.nodes[path[i]].name;
            if (i < path.size() - 1)
                cout << "\n |\n v\n";
            else
                cout << " [DESTINATION]\n";
        }
    }

    cout << "==================================================\n";
    cout << ">>> STARTING RIDE... (Driver is now BUSY)\n";

    thread t(rideSimulationThread, &drivers, driver->driverID, ride.pickupNode, ride.dropoffNode, ride.fare, duration);
    t.detach();
}

/*
 * ======================================================================================
 * MODULE 10: INITIALIZATION & MAIN LOOP
 * ======================================================================================
 */

void buildCityGraph() {
    // Nodes
    city.addNode(0, "COMSATS Main Gate", 0);
    city.addNode(1, "Library Block", 0);
    city.addNode(2, "Parking Lot A", 0);
    city.addNode(3, "Jinnah Hall", 0);

    // AREA 1: HOSTEL CITY
    city.addNode(4, "Hostel Plaza", 1);
    city.addNode(5, "Food Street", 1);
    city.addNode(6, "Boys Hostel 1", 1);
    city.addNode(7, "Girls Hostel 2", 1);

    // AREA 2: CHATTA BAKHTAWAR
    city.addNode(8, "Bakhtawar Market", 2);
    city.addNode(9, "Bakhtawar School", 2);
    city.addNode(10, "Bakhtawar Bus Stop", 2);

    // AREA 3: CHAK SHAHZAD
    city.addNode(11, "NIH Hospital", 3);
    city.addNode(12, "Diplomatic Residences", 3);
    city.addNode(13, "Agriculture Research Center", 3);

    // AREA 4: PARK ROAD
    city.addNode(14, "F-9 Park Gate 1", 4);
    city.addNode(15, "F-9 Jogging Track", 4);
    city.addNode(16, "F-9 Parking Lot", 4);

    // AREA 5: TARAMRI
    city.addNode(17, "Taramri Chowk", 5);
    city.addNode(18, "Taramri Market", 5);
    city.addNode(19, "NARC Road Junction", 5);

    // AREA 6: BAHRIA ENCLAVE
    city.addNode(20, "Bahria Phase 1 Entrance", 6);
    city.addNode(21, "Bahria Civic Center", 6);
    city.addNode(22, "Bahria Zoo", 6);

    // AREA 7: RAWAL DAM
    city.addNode(23, "Rawal Dam Viewpoint", 7);
    city.addNode(24, "Lake Track Point", 7);
    city.addNode(25, "Rawal Dam Parking", 7);

    // INTERNAL (Slow)
    int s_slow = 30;
    city.addEdge(0, 1, 1, s_slow);
    city.addEdge(1, 2, 1, s_slow);
    city.addEdge(2, 3, 1, s_slow);
    city.addEdge(0, 3, 2, s_slow);

    city.addEdge(4, 5, 1, s_slow);
    city.addEdge(5, 6, 1, s_slow);
    city.addEdge(6, 7, 2, s_slow);
    city.addEdge(4, 7, 2, s_slow);

    city.addEdge(8, 9, 1, s_slow);
    city.addEdge(9, 10, 1, s_slow);
    city.addEdge(8, 10, 2, s_slow);

    city.addEdge(11, 12, 2, s_slow);
    city.addEdge(12, 13, 2, s_slow);
    city.addEdge(11, 13, 3, s_slow);

    city.addEdge(14, 15, 1, s_slow);
    city.addEdge(15, 16, 1, s_slow);
    city.addEdge(14, 16, 2, s_slow);

    city.addEdge(17, 18, 1, s_slow);
    city.addEdge(18, 19, 1, s_slow);
    city.addEdge(17, 19, 2, s_slow);

    city.addEdge(20, 21, 2, s_slow);
    city.addEdge(21, 22, 2, s_slow);
    city.addEdge(20, 22, 3, s_slow);

    city.addEdge(23, 24, 2, s_slow);
    city.addEdge(24, 25, 1, s_slow);
    city.addEdge(23, 25, 3, s_slow);

    // MAIN ROADS (Medium)
    int s_med = 60;
    city.addEdge(0, 4, 4, s_med);
    city.addEdge(1, 5, 3, s_med);
    city.addEdge(2, 6, 3, s_med);
    city.addEdge(5, 8, 4, s_med);
    city.addEdge(6, 9, 5, s_med);
    city.addEdge(7, 10, 4, s_med);
    city.addEdge(10, 11, 6, s_med);
    city.addEdge(8, 12, 7, s_med);
    city.addEdge(13, 15, 6, s_med);
    city.addEdge(16, 17, 6, s_med);
    city.addEdge(15, 19, 5, s_med);
    city.addEdge(18, 20, 7, s_med);

    // HIGHWAYS (Fast)
    int s_fast = 100;
    city.addEdge(12, 14, 7, s_fast);
    city.addEdge(11, 16, 8, s_fast);
    city.addEdge(14, 18, 7, s_fast);
    city.addEdge(19, 20, 8, s_fast);
    city.addEdge(17, 21, 9, s_fast);
    city.addEdge(22, 23, 9, s_fast);
    city.addEdge(21, 24, 10, s_fast);
    city.addEdge(20, 25, 11, s_fast);

    // SHORTCUTS (Fast)
    city.addEdge(0, 8, 8, s_fast);
    city.addEdge(4, 11, 10, s_fast);
    city.addEdge(14, 20, 12, s_fast);
    city.addEdge(11, 17, 9, s_fast);
}


int main(){
    buildCityGraph();
    DriverList drivers;

    int choice;

    cout << "\n";
    cout << "============================================\n";
    cout << " RIDE MANAGEMENT SYSTEM - ISLAMABAD\n";
    cout << " Real-time Shortest Path Navigation\n";
    cout << "============================================\n";

    do {
        {
            lock_guard<mutex> guard(printLock);
            cout << "\n=========== MAIN MENU ===========\n";
            cout << "1. Display City Map\n";
            cout << "2. Register Driver\n";
            cout << "3. Display All Drivers\n";
            cout << "4. Request a Ride\n";
            cout << "5. Rate Completed Rides\n";
            cout << "6. View Driver Ride History\n";
            cout << "7. Exit\n";
            cout << "Enter choice: ";
        }

        while (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input! Enter a number (1-7): ";
        }

        switch (choice) {
            case 1:
                city.displayMap();
                break;
            case 2:
                drivers.registerDriver();
                break;
            case 3:
                drivers.displayDrivers();
                break;
            case 4:
                customerRequest(drivers);
                break;
            case 5:
                drivers.processPendingRatings();
                break;
            case 6:
                drivers.viewDriverHistory();
                break;
            case 7:
                cout << "\n💾 Saving data before exit...\n";
                drivers.saveToFile();
                cout << "\nExiting...\n";
                break;
            default:
                cout << "\n✗ Invalid choice! Please select 1-7.\n";
        }
    }
    while (choice != 7);

    return 0;
}