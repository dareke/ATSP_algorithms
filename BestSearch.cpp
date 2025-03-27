#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <thread>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

using namespace std;


struct Config {
    string inputFilePath;
    string outputFilePath;
    int repetitions;
};


Config loadConfig(const string &configFilePath) {
    Config config;
    ifstream configFile(configFilePath);

    if (!configFile.is_open()) {
        cerr << "Blad wczytywania pliku konfiguracyjnego. " << endl;
        system("pause");
        exit(1);
    }

    string line;
    while (getline(configFile, line)) {
        if (line.find("inputFilePath") != string::npos) {
            config.inputFilePath = line.substr(line.find("=") + 1);
        } else if (line.find("outputFilePath") != string::npos) {
            config.outputFilePath = line.substr(line.find("=") + 1);
        } else if (line.find("repetitions") != string::npos) {
            config.repetitions = stoi(line.substr(line.find("=") + 1));
        }
    }

    return config;
}


vector<vector<int>> loadMatrix(const string &inputFilePath, int &matrixSize, int &optimalCost) {
    ifstream inputFile(inputFilePath);

    if (!inputFile.is_open()) {
        cerr << "Nie udalo sie wczytac danych." << endl;
        system("pause");
        exit(1);
    }

    inputFile >> matrixSize;
    vector<vector<int>> matrix(matrixSize, vector<int>(matrixSize));

    for (int i = 0; i < matrixSize; ++i) {
        for (int j = 0; j < matrixSize; ++j) {
            inputFile >> matrix[i][j];
        }
    }
    inputFile >> optimalCost;
    return matrix;
}


void calculateCandidateCost(const vector<vector<int>>& matrix, vector<bool> visited, int candidate,
                            double& resultCost, int& resultCity, std::mutex& resultMutex) {
    int matrixSize = matrix.size();
    double localCost = 0;  // Koszt całkowity w lokalnej symulacji
    int currentCity = candidate;

    visited[candidate] = true;

    // Symulacja algorytmu nearest neighbor dla kandydata
    for (int step = 1; step < matrixSize; ++step) {
        double minCost = numeric_limits<double>::max();
        int next = -1;

        // Znajdź najbliższego sąsiada
        for (int j = 0; j < matrixSize; ++j) {
            if (!visited[j] && matrix[currentCity][j] < minCost) {
                minCost = matrix[currentCity][j];
                next = j;
            }
        }

        if (next != -1) {
            visited[next] = true;
            localCost += minCost;
            currentCity = next;
        }
    }

    // Powrót do miasta początkowego
    localCost += matrix[currentCity][candidate];

    // Synchronizacja i aktualizacja wyniku
    std::lock_guard<std::mutex> lock(resultMutex);
    if (localCost < resultCost) {
        resultCost = localCost;
        resultCity = candidate;
    }
}


pair <vector<int>, int> nearestNeighborSolve( const vector<vector<int>>& matrix, int startCity) {
    int matrixSize = matrix.size();
    vector<int> path;
    vector<bool> visited(matrixSize, false);
    double totalCost = 0;

    auto start = chrono::high_resolution_clock::now();

    int current = startCity;
    path.push_back(current);
    visited[current] = true;

    for (int i = 1; i < matrixSize; ++i) {
        double minCost = numeric_limits<double>::max();
        vector<int> equalCities;

        // Szukanie miast o minimalnym koszcie
        for (int j = 0; j < matrixSize; ++j) {
            if (!visited[j]) {
                if (matrix[current][j] < minCost) {
                    minCost = matrix[current][j];
                    equalCities = {j};
                } else if (matrix[current][j] == minCost) {
                    equalCities.push_back(j);
                }
            }
        }

        int next = -1;
        if (equalCities.size() > 1) {
            // Przygotowanie do wielowątkowego przetwarzania
            std::vector<std::thread> threads;
            std::mutex resultMutex;
            double bestCost = numeric_limits<double>::max();
            int bestCity = -1;

            // Uruchamianie wątków dla każdego miasta o równym koszcie
            for (int candidate: equalCities) {
                threads.emplace_back(calculateCandidateCost, std::ref(matrix), visited, candidate,
                                     std::ref(bestCost), std::ref(bestCity), std::ref(resultMutex));
            }

            // Czekanie na zakończenie wszystkich wątków
            for (std::thread &t: threads) {
                if (t.joinable()) {
                    t.join();
                }
            }

            next = bestCity;
        } else {
            next = equalCities.front();
        }

        path.push_back(next);
        visited[next] = true;
        totalCost += matrix[current][next];
        current = next;
    }
    totalCost += matrix[current][startCity];

    return {path, totalCost};
}
pair<vector<int>,int> calculateNNUpperBound(const vector<vector<int>>& matrix) {
    int matrixSize = matrix.size();
    int bestUB = numeric_limits<int>::max();
    vector<int> path;
    for (int i = 0; i < matrixSize; ++i) {
        pair<vector<int>,int> resultNN = nearestNeighborSolve(matrix, i);
        if(bestUB > resultNN.second){
            bestUB = resultNN.second;
            path = resultNN.first;

        }

    }
    //cout << "bestUB " << bestUB << endl;
    return {path,bestUB};
}

void saveResults(const string &outputFilePath, const string &inputFileName, const vector<int> &minPath,
                 int minCost, double executionTime, int &optimalCost) {
    ofstream outputFile(outputFilePath, ios::app);

    if (!outputFile.is_open()) {
        cerr << "Blad otwarcia pliku wyjsciowego." << endl;
        system("pause");
        exit(1);
    }

    // Nagłówek tylko w przypadku nowego pliku
    static bool headerWritten = false;
    if (!headerWritten) {
        outputFile << "Nazwa pliku z danymi\tSciezka\tKoszt sciezki\tOptymalny Koszt\tCzas Wykonania (s)\n";
        headerWritten = true;
    }

    // Zapis danych
    outputFile << inputFileName << "\t";

    for (size_t i = 0; i < minPath.size(); ++i) {
        outputFile << minPath[i];
        if (i < minPath.size() - 1) outputFile << "->";
    }
    outputFile << "->" << minPath[0] << "\t"; // Powrót do początkowego miasta
    outputFile << minCost << "\t";
    outputFile << optimalCost << "\t";
    outputFile << fixed << setprecision(6) << executionTime << "\n";

    outputFile.close();
}


// Funkcja haszująca, pomocnicza do MST
struct BoolVectorHash {
    size_t operator()(const vector<bool>& v) const {
        size_t hash = 0;
        for (bool b : v) {
            hash = (hash << 1) ^ b; // Proste haszowanie bitów
        }
        return hash;
    }
};

int calculateMSTCost(const vector<vector<int>> &matrix, const vector<bool> &visited,
                     unordered_map<vector<bool>, int, BoolVectorHash> &mstCache) {
    if (mstCache.find(visited) != mstCache.end()) {
        return mstCache[visited]; // Zwróć wynik z cache
    }

    int matrixSize = matrix.size();
    vector<bool> inMST(matrixSize, false);
    vector<int> minEdge(matrixSize, numeric_limits<int>::max());
    int mstCost = 0;

    int startNode = -1;
    for (int i = 0; i < matrixSize; ++i) {
        if (!visited[i]) {
            startNode = i;
            break;
        }
    }
    if (startNode == -1) return 0;

    minEdge[startNode] = 0;

    for (int i = 0; i < matrixSize; ++i) {
        int u = -1;
        for (int v = 0; v < matrixSize; ++v) {
            if (!inMST[v] && (u == -1 || minEdge[v] < minEdge[u])) {
                u = v;
            }
        }

        if (u == -1) return INT_MAX;
        inMST[u] = true;
        mstCost += minEdge[u];

        for (int v = 0; v < matrixSize; ++v) {
            if (!inMST[v] && !visited[v] && matrix[u][v] < minEdge[v]) {
                minEdge[v] = matrix[u][v];
            }
        }
    }

    mstCache[visited] = mstCost; // Zapisz wynik do cache
    return mstCost;
}


struct Node {
    vector<int> path;           // Ścieżka odwiedzonych miast
    int cost;                   // Koszt dotychczasowej ścieżki
    int lowerBound;             // Dolne ograniczenie kosztu dla tego węzła
    vector<bool> visited;       // Informacja, które miasta zostały odwiedzone

    bool operator>(const Node& other) const {
        return lowerBound > other.lowerBound; // Kolejka priorytetowa: minimalizacja dolnego ograniczenia
    }
};


pair<vector<int>, int> BestSearchSolve(const vector<vector<int>>& matrix, double& executionTime) {
    int matrixSize = matrix.size();

    // Cache dla obliczeń MST
    priority_queue<Node, vector<Node>, greater<Node>> pq;

    unordered_map<vector<bool>, int, BoolVectorHash> mstCache;

    auto startTime = chrono::high_resolution_clock::now();
    auto timeLimit = chrono::minutes(30);
    // Inicjalizacja korzenia

    Node root;
    root.path = {0};                  // Zaczynamy od miasta 0
    root.cost = 0;
    root.visited = vector<bool>(matrixSize, false);
    root.visited[0] = true;           // Miasto początkowe jest odwiedzone
    root.lowerBound = calculateMSTCost(matrix, root.visited,mstCache);


    pq.push(root);
    pair <vector<int>, int> resultNN = calculateNNUpperBound(matrix);
    vector<int> bestPath = resultNN.first;
    int bestCost = resultNN.second;

    while (!pq.empty()) {
        auto currentTime = chrono::high_resolution_clock::now();
        if (currentTime - startTime > timeLimit) {
            cout << "Limit czasu (30 minut) osiagniety." << endl;
            break;
        }

        Node current = pq.top();
        pq.pop();


        if (current.lowerBound >= bestCost) {
            continue;
        }


        if (current.path.size() == matrixSize) {
            int completeCost = current.cost + matrix[current.path.back()][0];
            if (completeCost < bestCost) {
                bestCost = completeCost;
                bestPath = current.path;
            }
            continue;
        }


        for (int nextCity = 0; nextCity < matrixSize; ++nextCity) {
            if (!current.visited[nextCity]) {
                Node child;
                child.path = current.path;
                child.path.push_back(nextCity);
                child.cost = current.cost + matrix[current.path.back()][nextCity];
                child.visited = current.visited;
                child.visited[nextCity] = true;


                child.lowerBound = child.cost + calculateMSTCost(matrix, child.visited, mstCache);


                if (child.lowerBound < bestCost) {
                    pq.push(child);
                }
            }
        }
    }

    // Mierzenie czasu zakończenia algorytmu
    auto endTime = chrono::high_resolution_clock::now();
    executionTime = chrono::duration<double>(endTime - startTime).count();

    return {bestPath, bestCost};
}



int main() {
    // Wczytanie pliku konfiguracyjnego
    string configFilePath = "config.txt";
    Config config = loadConfig(configFilePath);

    int matrixSize;
    int optimalCost;
    vector<int> bestPath;
    int bestCost;
    double executionTime;
    vector<vector<int>> matrix = loadMatrix(config.inputFilePath, matrixSize, optimalCost);

    // Wykonanie algorytmu z powtórzeniami
    for (int i = 0; i < config.repetitions; ++i) {
        cout << "Powtorzenie: " << i + 1 << endl;
        cout << "Sciezka optymalna: " << optimalCost << endl;
        pair <vector<int>, int> result = BestSearchSolve(matrix, executionTime);
        bestPath = result.first;
        bestCost = result.second;
        saveResults(config.outputFilePath, config.inputFilePath, bestPath, bestCost, executionTime, optimalCost);

        cout << "Najlepsza sciezka: ";
        for (int city : bestPath) {
            cout << city << " -> ";
        }
        cout << bestPath[0] << endl;
        cout << "Koszt: " << bestCost << endl;
        cout << "Czas: " << executionTime << " sekund" << endl;
    }
    system("pause");
    return 0;
}
