#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <limits>
#include <iomanip>
#include <cmath>
#include <thread>
#include <mutex>
#include <deque>
#include <algorithm>
#include <random>


using namespace std;


struct Config {
    bool isAsymmetric;
    string inputFilePath;
    string outputFilePath;
    bool ACOuseDAS;
    int ACOmaxReps;

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
        if (line.find("asymmetric") != string::npos) {
            config.isAsymmetric = (line.find("true") != string::npos);
        } else if(line.find("inputFilePath") != string::npos) {
            config.inputFilePath = line.substr(line.find("=") + 1);
        } else if (line.find("outputFilePath") != string::npos) {
            config.outputFilePath = line.substr(line.find("=") + 1);
        } else if (line.find("ACOuseDAS") != string::npos) {
            config.ACOuseDAS = (line.find("true") != string::npos);
        } else if (line.find("ACOmaxReps") != string::npos) {
            config.ACOmaxReps = stoi(line.substr(line.find("=") + 1));
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
        outputFile << "Nazwa pliku z danymi\tSciezka\tKoszt sciezki\tOptymalny Koszt\tCzas Wykonania (s)\tBlad bezwzgledny\tBlad wzgledny\n";
        headerWritten = true;
    }

    int absoluteError = abs(minCost - optimalCost);
    double relativeError = (double)absoluteError / optimalCost * 100.0;
    cout << "Blad bezwzgledny: " << absoluteError << " Blad wzgledny: " << relativeError << "%\n";

    // Zapis danych
    outputFile << inputFileName << "\t";

    for (size_t i = 0; i < minPath.size(); ++i) {
        outputFile << minPath[i];
        if (i < minPath.size() - 1) outputFile << "->";
    }
    outputFile << "->" << minPath[0] << "\t"; // Powrót do początkowego miasta
    outputFile << minCost << "\t";
    outputFile << optimalCost << "\t";
    outputFile << fixed << setprecision(6) << executionTime << "\t";
    outputFile << absoluteError << "\t";
    outputFile << relativeError << "\n";


    outputFile.close();
}

const double alpha = 1;  // Wpływ feromonu
const double beta = 3;   // Wpływ heurystyki
const double rho = 0.5;  // Współczynnik parowania feromonów
const double Q = 10;  // Stała do aktualizacji feromonów

void updatePheromones(int ant_count, const vector<vector<int>>& matrix, vector<vector<double>>& pheromones, const vector<vector<int>>& paths, const vector<int>& costs, bool isAsymmetric, bool useDAS) {
    int n = pheromones.size();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            pheromones[i][j] *= (1.0 - rho); // Parowanie feromonów
        }
    }

    for (int k = 0; k < ant_count; k++) {
        if (!useDAS) {
            double deltaTau = Q / costs[k];
            for (size_t i = 0; i < paths[k].size() - 1; i++) {
                int from = paths[k][i], to = paths[k][i + 1];
                pheromones[from][to] += deltaTau;
                if(!isAsymmetric){
                    pheromones[to][from] += deltaTau;
                }
            }
        } else {
            for (size_t i = 0; i < paths[k].size() - 1; i++) {
                int from = paths[k][i], to = paths[k][i + 1];
                double deltaTau = Q / matrix[from][to];
                pheromones[from][to] += deltaTau;
                if(!isAsymmetric){
                    pheromones[to][from] += deltaTau;
                }
            }
        }
    }
}

pair<vector<int>, int> ACOSolve(const vector<vector<int>>& matrix, Config config, int optimalCost) {
    int n = matrix.size();
    int ant_count = n;
    vector<vector<double>> pheromones(n, vector<double>(n, 1.0));
    vector<int> bestPath;
    int bestCost = numeric_limits<int>::max();

    auto startTime = chrono::high_resolution_clock::now();

    for (int rep = 0; rep < config.ACOmaxReps; ++rep) {
        vector<vector<int>> paths(ant_count);
        vector<int> costs(ant_count, 0);

        auto elapsedTime = chrono::duration_cast<chrono::minutes>(
                chrono::high_resolution_clock::now() - startTime
        );
        if (elapsedTime.count() >= 15) {
            cout << "Przekroczono limit 15 minut." << endl;
            break;
        }
        if(bestCost == optimalCost){    //warunek stopu - znaleziono opt
            cout << "Znaleziono rozwiazanie optymalne." << endl;
            break;
        }


        for (int k = 0; k < ant_count; k++) {
            vector<int> path;
            vector<bool> visited(n, false);
            int current = k;
            path.push_back(current);
            visited[current] = true;

            for (int step = 1; step < n; step++) {
                vector<double> probabilities(n, 0.0);
                double sum = 0.0;

                for (int j = 0; j < n; j++) {
                    if (!visited[j]) {
                        probabilities[j] = pow(pheromones[current][j], alpha) * pow(1.0 / matrix[current][j], beta);
                        sum += probabilities[j];
                    }
                }

                double r = (double) rand() / RAND_MAX * sum;
                double cumulative = 0.0;
                int nextCity = -1;

                for (int j = 0; j < n; j++) {
                    if (!visited[j]) {
                        cumulative += probabilities[j];
                        if (cumulative >= r) {
                            nextCity = j;
                            break;
                        }
                    }
                }

                if (nextCity == -1) break;
                path.push_back(nextCity);
                visited[nextCity] = true;
                costs[k] += matrix[current][nextCity];
                current = nextCity;
            }
            costs[k] += matrix[current][path.front()];

            paths[k] = path;

            if (costs[k] < bestCost) {
                bestCost = costs[k];
                bestPath = path;
                rep = 0;
            }
        }

        updatePheromones(ant_count, matrix, pheromones, paths, costs, config.isAsymmetric, config.ACOuseDAS);
    }
    return {bestPath, bestCost};
}

int main() {
    string configFilePath = "config.txt";
    Config config = loadConfig(configFilePath);

    int matrixSize;
    int optimalCost;
    vector<int> bestPath;
    int bestCost;
    double executionTime;
    vector<vector<int>> matrix = loadMatrix(config.inputFilePath, matrixSize, optimalCost);

    cout << "Sciezka optymalna: " << optimalCost << endl;

    auto startTime = chrono::high_resolution_clock::now();

    pair <vector<int>, int> result = ACOSolve(matrix, config, optimalCost);

    auto endTime = chrono::high_resolution_clock::now();
    executionTime = chrono::duration<double>(endTime - startTime).count();
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

    system("pause");
    return 0;
}
