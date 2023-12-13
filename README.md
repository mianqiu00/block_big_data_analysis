# Blockchain Big Data Analysis

## 1. Background
Blockchain technology has widespread applications in fields such as financial technology, legal evidence, and data sharing. This experiment aims to analyze a large volume of transaction records within a blockchain, understanding data structures and algorithms, fostering skills in problem modeling, abstraction, design, and the use of new tools. Additionally, it seeks to enhance self-learning and communication abilities.

## 2. Objectives
The main objectives of this experiment include:
- Mastery of basic knowledge and usage techniques of structures such as linked lists, binary trees, and graphs.
- Development of problem modeling and abstraction skills.
- Cultivation of the ability to design and use new tools.
- Enhancement of self-learning capabilities.

## 3. Problem Description
In this experiment, the following functionalities should be implemented:

### Data Initialization:
- Read data from a specified file to initialize the blockchain.
- Minimize storage overhead and complete data initialization in the shortest time possible.
- The blockchain data structure should record the generation time of each block, with all transactions organized in a binary tree or B-tree.

### Data Query:
- Find all incoming or outgoing records for a specified account within a given time period:
  - Return the total number of records and the top k records with the highest transaction amounts (where k is determined by user input).
- Query the amount of a specific account at a given moment:
  - Allow negative values.
- Forbes Billionaires list at a specific moment:
  - Output the top k richest users at that moment (default k value is 50, user-modifiable).

### Data Analysis:
- Build a transaction relationship graph:
  - If account A has transferred funds to B, there should be an arc from A to B with a weight representing the cumulative transfer amount.
- Calculate the average outdegree and indegree of the transaction relationship graph:
  - Display the top k accounts with the highest outdegree and indegree.
- Check for cycles in the transaction relationship graph:
  - Output YES or NO.
- Given an account A, find the shortest paths to all other accounts:
  - Path length is the sum of weights of all arcs in the path. If no path exists from A to B, do not output anything; provide a prompt.

### Data Insertion:
- Read new transaction records from a file and add them to the existing transaction graph.
- Re-run functionalities 2 and 3.

**Note:** Due to the large number of nodes in this experiment, specific input and output examples should include usernames for account A and the target account.

## 4. Instructions for Running the Code
- Detailed instructions for running the code has been inserted into the C file, input your operation strictly following the instruction, or the code would crack.


Feel free to customize and elaborate on each section as needed for your specific implementation.
