#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits>
#include <iomanip>
#include <random>
#include <thread>
#include <mutex>

using namespace std;



struct Config {
    bool chooseStart;
    int chosenStart;
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
        if (line.find("chooseStart") != string::npos) {
            config.chooseStart = (line.find("true") != string::npos);
        } else if (line.find("chosenStart") != string::npos) {
            config.chosenStart = stoi(line.substr(line.find("=") + 1));
        } else if (line.find("inputFilePath") != string::npos) {
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


void saveResults(const string &outputFilePath, const string &inputFileName, const vector<int> &minPath, int minCost, double executionTime, int &optimalCost) {
    ofstream outputFile;
    bool fileExists = ifstream(outputFilePath).good();  // sprawdzanie, czy plik istnieje

    outputFile.open(outputFilePath, ios::app);  // otwieranie pliku w trybie dopisywania

    if (!outputFile.is_open()) {
        cerr << "Blad pliku wyjsciowego." << endl;
        system("pause");
        exit(1);
    }
    if (!fileExists) {
        outputFile << "Nazwa pliku z danymi\tSciezka\tKoszt sciezki\tOptymalny Koszt\tCzas Wykonania (s)\n";
    }


    outputFile << inputFileName << "\t";

    // Zapis ścieżki
    for (size_t i = 0; i < minPath.size(); ++i) {
        outputFile << minPath[i];
        if (i < minPath.size() - 1) outputFile << "->";
    }
    outputFile << "->" << minPath[0] << "\t";
    outputFile << minCost << "\t";
    outputFile << optimalCost << "\t";
    outputFile << fixed << setprecision(6) << executionTime << "\n";

    outputFile.close();
}

void calculateCandidateCost(const vector<vector<int>>& matrix, vector<bool> visited, int candidate,
                            double& resultCost, int& resultCity, std::mutex& resultMutex) {
    int matrixSize = matrix.size();
    double localCost = 0;
    int currentCity = candidate;

    visited[candidate] = true;

    //wykonanie NN dla kandydata
    for (int step = 1; step < matrixSize; ++step) {
        double minCost = numeric_limits<double>::max();
        int next = -1;


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
//zmienna globalna do zatrzymania programu
auto globalStartTime = chrono::high_resolution_clock::now();

void nearestNeighborSolve(const Config &config, const vector<vector<int>>& matrix, int &optimalCost, int rep) {
    int matrixSize = matrix.size();
    vector<int> path;
    vector<bool> visited(matrixSize, false);
    int startCity = config.chooseStart ? config.chosenStart : rep % matrixSize;
    double totalCost = 0;

    auto start = chrono::high_resolution_clock::now();

    int current = startCity;
    path.push_back(current);
    visited[current] = true;

    for (int i = 1; i < matrixSize; ++i) {
        double minCost = numeric_limits<double>::max();
        vector<int> equalCities;


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
        if (equalCities.size() > 1) {// Uruchamianie wątków dla każdego miasta o równym koszcie
            std::vector<std::thread> threads;
            std::mutex resultMutex;
            double bestCost = numeric_limits<double>::max();
            int bestCity = -1;


            for (int candidate : equalCities) {
                threads.emplace_back(calculateCandidateCost, std::ref(matrix), visited, candidate,
                                     std::ref(bestCost), std::ref(bestCity), std::ref(resultMutex));
            }


            for (std::thread& t : threads) {
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
        auto currentTime = chrono::high_resolution_clock::now();
        chrono::duration<double> elapsedTime = currentTime - globalStartTime;
        if (elapsedTime.count() >= 1800.0) {
            cout << "Przekroczono limit czasu" << endl;
            return;
        }
    }


    // Powrót do miasta startowego
    totalCost += matrix[current][startCity];
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> executionTime = end - start;

    cout << "Sciezka: ";
    for (int city : path) {
        cout << city << " -> ";
    }
    cout << path[0] << endl;
    cout << "Koszt sciezki: " << totalCost << endl;
    cout << "Czas: " << executionTime.count() << " sekund" << endl;

    saveResults(config.outputFilePath, config.inputFilePath, path, totalCost, executionTime.count(), optimalCost);
}
int main() {
    string configFilePath = "config.txt";
    Config config = loadConfig(configFilePath);

    int matrixSize;
    int optimalCost;
    vector<vector<int>> matrix = loadMatrix(config.inputFilePath, matrixSize, optimalCost);

    for (int i = 0; i < config.repetitions; ++i) {
        cout << "Powtorzenie: " << i + 1 << endl;
        cout << "Sciezka optymalna: " << optimalCost << endl;
        nearestNeighborSolve(config, matrix, optimalCost, i);
        cout << endl << endl;
    }
    system("pause");
    return 0;
}
