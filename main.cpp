#include <iostream>
#include "mpi.h"
#include <vector>

class Node 
{
    private:
        int m_rank;
        std::string m_name;
        std::vector<int> m_neighbore;
        std::vector<int> m_buffer;
    public:
        Node(int rank, std::string name, const std::vector<int>& neighbore):    m_rank(rank),
                                                                                m_name(name),
                                                                                m_neighbore(neighbore)
        {
            m_buffer.reserve(m_neighbore.size());
            std::string validate_message = "Node: " + m_name + ", rank: " + std::to_string(m_rank) + ", neighbore: ";
            for(int i=0; i<m_neighbore.size(); i++)
            {
                validate_message += std::to_string(m_neighbore[i]) + " ";
            }
            validate_message += "\n";
            std::cout << validate_message;
        }      
};


int main(int argc, char * argv[]) {
    int rank, size;
    std::vector<int> neighbore1 {4};
    std::vector<int> neighbore2 {3,4,5};
    std::vector<int> neighbore3 {2};
    std::vector<int> neighbore4 {1,2,6};
    std::vector<int> neighbore5 {2};
    std::vector<int> neighbore6 {4};
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(size!=7)
    {
        std::cout<<"Error: Need to run program with 7 processes \n";
        return 1;
    }

    if(rank==1) {Node node1(rank, "q", neighbore1);}
    if(rank==2) {Node node2(rank, "s", neighbore2);}
    if(rank==3) {Node node3(rank, "t", neighbore3);}
    if(rank==4) {Node node4(rank, "r", neighbore4);}
    if(rank==5) {Node node5(rank, "u", neighbore5);}
    if(rank==6) {Node node6(rank, "p", neighbore6);}
    MPI_Finalize();
    return 0;
}