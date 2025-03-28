# ATSP_algorithms
## Description
My implementation of chosen algorithms to solve travelling salesman problem.
Each algorithm is written seperately in named file.
Programs take matrix from output file and works on finding the optimal solution, after finishing stores the result in the output file.
Result given is: found best cost, found best path, work time, relative and absolute error, best cost divided by optimal cost in percentage.
### Algorithms and their implementation
- Brute force: checks every solution looking for optimal one. Uses next_permutation to generate next solution to check.
- Nearest neighbor: greedy algorithm, takes the best local cost of next city.
- Random: generates random solution, repeated until found optimal cost or given time has passed.
- Branch&Bound algorithms: walks throughout solution tree, while pruning worse cost paths using  estimated lower bound. Initial upper bound is calculated by repeated nearest neighbor algorithm for each node.
- Tabu search: Initial solution is generated by repeated nearest neighbor algorithm for each node, later used for local search using swap or 2-opt moves. Stores used moves in tabu list. Uses aspiration criteria to use forbidden move that will give better result.
- Ant colony optimisation: implementation based on ant behavior. Uses pheromones and ant walk to find optimal path. Implementation uses density or quantity pheromone spread.
## Instruction
To work program needs config file that stores algorithm arguments. Structure of the file varies for different algorithms.
### General elements
Every implementation needs this information:
- `inputFilePath` - Location of input file with ATSP data. Compatible data has:
  - Dimension of matrix in a first line
  - Cost matrix with every element split with space
  - Optimal value in the last line.
- `outputFilePath` - Location of output file. If file doesn't exist program will make one. Output file should use `.xlsx` extension.
- `repetitions` - number of repetitions of the algorithm. Natural numbers work as an argument.

>inputFilePath=pathToFile\yourInputFile.txt
>
>outputFilePath=pathToFile\yourOutputFile.txt
>
>repetitions=1

Some implementations also use argument isAsymmetric if algorithm solves asymmetric and symmetric problems different.
`true` or `false` values are considered.

>isAsymmetric=true

### Nearest Neighbor
This implementation works with either chosen start node, or randomly picked one.
Additional config lines:
- `chooseStart` - if given `true` value, program takes chosenStart number as a start node, `false` chooses start node randomly.
- `chosenStart` - number of chosen start node.
>chooseStart=false
>
>chosenStart=1

### Tabu Search
Additional tabu related arguments:
- `tabuMaxReps` - maximum repetition of neighborhood search without optimising the result.
- `tabuLength` - lenght of tabu list as well as tenure of a move in tabu list.
- `tabuIfTwoOpt` - If `true` algorithm searches neighborhood with 2-opt swap, if `false` swap operation is used.
>tabuMaxReps=200
>
>tabuLength=40
>
>tabuIfTwoOpt=true

### Ant Colony Optimization
- `ACOuseDAS` - if `true` density spread of pheromones is used, if 'false' cyclical spread is used.
- `ACOmaxReps` - maximum repetition of  without optimising the result.

>ACOuseDAS=false
>
>ACOmaxReps=5

### Author
Jakub Biadalski 2024


