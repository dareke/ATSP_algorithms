#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>
#include <limits>
#include <iomanip>
#include <numeric>

using namespace std;

// Struktura do przechowywania konfiguracji
struct Config {
    string inputFilePath;
    string outputFilePath;
    int repetitions;
};

// Funkcja do wczytania konfiguracji z pliku
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

// Funkcja licząca koszt ścieżki
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
void randomSolve(const Config &config, const vector<vector<int>>& matrix, int &optimalCost) {
    int matrixSize = matrix.size();
    int currentCost, minCost = numeric_limits<int>::max();
    vector<int> path(matrixSize), minPath;
    iota(path.begin(), path.end(), 0);

    auto start = chrono::high_resolution_clock::now();
    const auto timeLimit = chrono::minutes(30); // Limit czasu - 30 minut

    int count = 0;
    bool timeLimitExceeded = false;
    do{
        ++count;
        auto now = chrono::high_resolution_clock::now();
        if (now - start > timeLimit) {
            timeLimitExceeded = true;
            cout << "Przekroczono limit czasu 30 minut.\n";
        }



        shuffle(path.begin() + 1, path.end(), std::mt19937(std::random_device()()));
        currentCost = calculateCost(path, matrix);

        if(currentCost < minCost) {
            minPath = path;
            minCost = currentCost;
        }

        if(minCost == optimalCost){
            cout << "Wylosowane rozwiazanie nr " << count << " jest rozwiazaniem optymalnym." << endl;
            break;
        }
    }while(!timeLimitExceeded);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> executionTime = end - start;

    if(minCost != optimalCost) {
        cout << "Nie znaleziono rozwiazania optymalnego w " << count << " wylosowanych permutacjach." << endl;
    }
    cout << "Minimalna sciezka: ";
    for (int city : minPath) {
        cout << city << " -> ";
    }

    cout << minPath[0] << endl;
    cout << "Koszt minimalnej sciezki: " << minCost << endl;
    cout << "Czas: " << executionTime.count() << " sekund" << endl;


    saveResults(config.outputFilePath, config.inputFilePath, path, currentCost, executionTime.count(), optimalCost);

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
        randomSolve(config, matrix, optimalCost);
        cout << endl << endl;
    }
    system("pause");
    return 0;
}
