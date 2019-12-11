# CPSC474_Project2

<b>DUY DO</b><br />
<b>duy.ado@csu.fullerton.edu</b><br />


# Instruction to run and compile 
## 1 Start docker 
./start_docker.sh
## 2 Make
make
## 3 Run
mpirun -n 7 ./main.exe

# Note

Referred for topology.png to understand how the node is created in main.cpp </br>

Node 0 is reserved for starting the algorithm. Node 0 need to have every participating neighbore included in it neightbore vector. So set std::vector<int> neighbore0 {1,2,3,..with all neightbore};</br>

In the current topology there are 6 nodes (node 1-6) participating in the tree election, plus node 0 is 7 nodes.</br>

That's why main will check and fail if size!=7 (mpirun -n 7 ./main.exe), this value need to be change according both inside main.cpp and "mpirun -n size", if the tree topology changes.</br>

# Sources

## MPI struct
https://stackoverflow.com/questions/33618937/trouble-understanding-mpi-type-create-struct?fbclid=IwAR1R8zBF697gpufSkTxSOaEa_FeJpEym7UtnKK9WONOZT2ZOyktSEUfl_W0

## Thread related
https://en.cppreference.com/w/cpp/thread/condition_variable
https://en.cppreference.com/w/cpp/thread/thread
https://www.modernescpp.com/index.php/c-core-guidelines-taking-care-of-your-child-thread
https://www.youtube.com/watch?v=LL8wkskDlbs&list=PL5jc9xFGsL8E12so1wlMS0r0hTQoJL74M&index=1
https://stackoverflow.com/questions/19463602/compiling-multithread-code-with-g
## Message Queue
https://www.codeproject.com/Articles/1169105/Cplusplus-std-thread-Event-Loop-with-Message-Queue
