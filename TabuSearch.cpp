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
    bool ifRandomStart;
    int tabuMaxReps;
    int tabuLength;
    bool tabuIfTwoOpt;
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
        } else if (line.find("ifRandomStart") != string::npos) {
            config.ifRandomStart = (line.find("true") != string::npos);
        } else if (line.find("tabuMaxReps") != string::npos) {
            config.tabuMaxReps = stoi(line.substr(line.find("=") + 1));
        } else if (line.find("tabuLength") != string::npos) {
            config.tabuLength = stoi(line.substr(line.find("=") + 1));
        }else if (line.find("tabuIfTwoOpt") != string::npos) {
            config.tabuIfTwoOpt = (line.find("true") != string::npos);
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



int calculatePathCost(const vector<int>& path, const vector<vector<int>>& matrix) {
    int cost = 0;
    for (size_t i = 0; i < path.size() - 1; ++i) {
        cost += matrix[path[i]][path[i + 1]];
    }
    cost += matrix[path.back()][path.front()]; // powrot do poczatku
    return cost;
}

void performTwoOptSwap(vector<int>& path, int i, int j) {
    reverse(path.begin() + i, path.begin() + j + 1);
}

pair<vector<int>, int> tabuSearchSolve(const vector<vector<int>>& matrix, Config config, int optimalCost) {
    int matrixSize = matrix.size();
    int bestCost;
    vector<int> bestPath;
    if(config.ifRandomStart == true) {
        vector<int> path(matrixSize);
        iota(path.begin(), path.end(), 0);

        shuffle(path.begin() + 1, path.end(), std::mt19937(std::random_device()()));
        bestCost = calculatePathCost(path, matrix);
        bestPath = path;
    }
    else {
        pair <vector<int>, int> resultNN = calculateNNUpperBound(matrix);
        bestPath = resultNN.first;
        bestCost = resultNN.second;
    }


    vector<int> currentPath = bestPath;
    int currentCost = bestCost;

    deque<pair<int, int>> tabuList; // lista ruchów
    vector<vector<int>> tabuMatrix(matrixSize, vector<int>(matrixSize, 0)); //macierz kadencji zakazanych ruchów

    auto startTime = chrono::high_resolution_clock::now();

    for (int rep = 0; rep < config.tabuMaxReps; ++rep) {

        auto elapsedTime = chrono::duration_cast<chrono::minutes>(
                chrono::high_resolution_clock::now() - startTime
        );
        if (elapsedTime.count() >= 15) {
            cout << "Przekroczono limit 15 minut." << endl;
            break;
        }
        if(bestCost == optimalCost){
            break;
        }

        int bestNeighborCost = numeric_limits<int>::max();
        vector<int> bestNeighborPath = currentPath;
        pair<int, int> bestMove = {-1, -1};


        for (int i = 0; i < matrixSize - 1; ++i) {
            for (int j = i + 1; j < matrixSize; ++j) {

                if (!config.isAsymmetric && tabuMatrix[i][j] > 0) continue; // pomijamy ruchy symetryczne


                vector<int> neighborPath = currentPath;

                if(!config.tabuIfTwoOpt){
                    swap(neighborPath[i], neighborPath[j]);

                    int neighborCost = calculatePathCost(neighborPath, matrix);
                    if ((neighborCost < bestNeighborCost && tabuMatrix[i][j] == 0) || neighborCost < bestCost) {
                        bestNeighborCost = neighborCost;
                        bestNeighborPath = neighborPath;
                        bestMove = {i, j};
                    }
                } else {
                    neighborPath = currentPath;
                    performTwoOptSwap(neighborPath, i, j);

                    int neighborCost = calculatePathCost(neighborPath, matrix);
                    if ((neighborCost < bestNeighborCost && tabuMatrix[i][j] == 0) || neighborCost < bestCost) {
                        bestNeighborCost = neighborCost;
                        bestNeighborPath = neighborPath;
                        bestMove = {i, j};
                    }

                }
            }
        }

        //kryterium aspiracji - jezeli rozwiazanie jest lepsze, usun ruch z listy tabu
        if (bestNeighborCost < bestCost) {
            tabuMatrix[bestMove.first][bestMove.second] = 0;
        }

        currentPath = bestNeighborPath;
        currentCost = bestNeighborCost;

        //dodanie ruchu do listy tabu
        if (bestMove.first != -1 && bestMove.second != -1) {
            tabuList.push_back(bestMove);
            tabuMatrix[bestMove.first][bestMove.second] = config.tabuLength;
            if (!config.isAsymmetric) {
                tabuMatrix[bestMove.second][bestMove.first] = config.tabuLength;
            }
        }

        //skrocenie kadencji w macierzy tabu
        for (auto& row : tabuMatrix) {
            for (auto& cell : row) {
                if (cell > 0) {
                    --cell;
                }
            }
        }

        //usuwanie ruchów z listy tabu
        while (tabuList.size() > config.tabuLength) {
            tabuList.pop_front();
        }

        if (currentCost < bestCost) {
            bestCost = currentCost;
            bestPath = currentPath;
            rep = 0;
        }
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

        pair <vector<int>, int> result = tabuSearchSolve(matrix, config, optimalCost);

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
