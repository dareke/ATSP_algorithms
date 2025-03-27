#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits>
#include <iomanip>
#include <numeric>
#include <cmath>

using namespace std;


struct Config {
    bool isAsymmetric;
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
        if (line.find("asymmetric") != string::npos) {
            config.isAsymmetric = (line.find("true") != string::npos);
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


int calculateCost(const vector<int> &path, const vector<vector<int>> &matrix) {
    int cost = 0;
        for (size_t i = 0; i < path.size() - 1; ++i) {
            cost += matrix[path[i]][path[i + 1]];
        }
        cost += matrix[path.back()][path[0]];  // powrot do miasta poczatkowego

    return cost;
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


vector<int> reversePath(const vector<int>& path) {
    vector<int> reversed(path.rbegin(), path.rend());
    return reversed;
}

long factorial(int n)
{
    long f = 1;
    for (int i=1; i<=n; ++i)
        f *= i;
    return f;
}
void bruteForceSolve(const Config &config, const vector<vector<int>> &matrix, int &optimalCost) {
    int matrixSize = matrix.size();
    vector<int> path(matrixSize);
    iota(path.begin(), path.end(), 0);

    int minCost = numeric_limits<int>::max();
    vector<int> minPath;
    auto start = chrono::high_resolution_clock::now();

    const auto timeLimit = chrono::minutes(30);

    if (!config.isAsymmetric) {
        int fact = factorial(matrixSize - 1) / 2;
        int counter = 0;
        do {

            auto now = chrono::high_resolution_clock::now();
            if (now - start > timeLimit) {
                cout << "Przekroczono limit czasu.\n\n";
                break;
            }

            // Dla symetrycznych rozważamy tylko połowę permutacji,
            // jeśli ścieżka jest mniejsza leksykograficznie
            // od swojej odwrotności
            if (path < reversePath(path)) {
                int currentCost = calculateCost(path, matrix);
                counter++;
                if (currentCost < minCost) {
                    minCost = currentCost;
                    minPath = path;
                }
            }

        } while (next_permutation(path.begin() + 1, path.end()) && ++counter < fact);
    } else {
        do {

            auto now = chrono::high_resolution_clock::now();
            if (now - start > timeLimit) {
                cout << "Przekroczono limit czasu 30 minut.\n";
                break;
            }

            int currentCost = calculateCost(path, matrix);
            // Dla asymetrycznych rozważamy wszystkie permutacje
            if (currentCost < minCost) {
                minCost = currentCost;
                minPath = path;
            }
        } while (next_permutation(path.begin() + 1, path.end()));
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> executionTime = end - start;

    // Wyświetlanie wyników
    cout << "Minimalna ścieżka: ";
    for (int city : minPath) {
        cout << city << " -> ";
    }
    cout << minPath[0] << endl;
    cout << "Koszt minimalnej ścieżki: " << minCost << endl;
    cout << "Czas: " << executionTime.count() << " sekund" << endl;


    saveResults(config.outputFilePath, config.inputFilePath, minPath, minCost, executionTime.count(), optimalCost);
}


int main() {
    // Wczytanie pliku konfiguracyjnego
    string configFilePath = "config.txt";
    Config config = loadConfig(configFilePath);

    int matrixSize;
    int optimalCost;
    vector<vector<int>> matrix = loadMatrix(config.inputFilePath, matrixSize, optimalCost);


    for (int i = 0; i < config.repetitions; ++i) {
        cout << "Powtorzenie: " << i + 1 << endl;
        cout << "Sciezka optymalna: " << optimalCost << endl;
        bruteForceSolve(config, matrix, optimalCost);
        cout << endl << endl;
    }
    system("pause");
    return 0;
}
