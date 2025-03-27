# ATSP_algorithms
## Description
My implementation of chosen algorithms to solve travelling salesman problem.
Each algorithm is written seperately in named file.
## Instruction
To work program needs config file that stores algorithm arguments. Structure of the file varies for different algorithms.
### General elements
Every implementation needs this information:
- inputFilePath - Location of input file with ATSP data. Compatible data has:
  - Dimension of matrix in a first line
  - Cost matrix with every element split with space
  - Optimal value in the last line.
- outputFilePath - Location of output file. If file doesn't exist program will make one. Output file should use `.xlsx` extension.
- repetitions - number of repetitions of the algorithm. Natural numbers work as an argument.

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
- chooseStart - if given `true` value, program takes chosenStart number as a start node, `false` chooses start node randomly.
- chosenStart - number of chosen start node.
>chooseStart=false
>
>chosenStart=1

### Tabu Search
Additional tabu related arguments:
- tabuMaxReps - maximum repetition of neighborhood search without optimising the result.
- tabuLength - lenght of tabu list as well as tenure of a move in tabu list.
- tabuIfTwoOpt - If `true` algorithm searches neighborhood with 2-opt swap, if `false` swap operation is used.
>tabuMaxReps=20
>
>tabuLength=431
>
>tabuIfTwoOpt=true
>
### Ant Colony Optimization
- ACOuseDAS - if `true` density spread of pheromones is used, if 'false' cyclical spread is used.
- ACOmaxReps - maximum repetition of  without optimising the result.

>ACOuseDAS=false
>ACOmaxReps=5

### Author
Jakub Biadalski 2024


