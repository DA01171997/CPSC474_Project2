#include <iostream>
#include "mpi.h"
#include <vector>

class Node 
{
    private:
        int m_rank;
        std::string m_name="";
        std::vector<int> m_neighbore;
        std::vector<int> m_buffer;
    public:
        Node(){}
        Node(int rank): m_rank(rank) {}
        Node(int rank, const std::string& name, const std::vector<int>& neighbore): m_rank(rank), 
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
            // std::cout << validate_message;
        }
        int getRank() const {return m_rank;}
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
    Node * node_ptr;
    if(rank==0) {node_ptr = new Node(rank);}
    if(rank==1) {node_ptr = new Node(rank, "q", neighbore1);}
    if(rank==2) {node_ptr = new Node(rank, "s", neighbore2);}
    if(rank==3) {node_ptr = new Node(rank, "t", neighbore3);}
    if(rank==4) {node_ptr = new Node(rank, "r", neighbore4);}
    if(rank==5) {node_ptr = new Node(rank, "u", neighbore5);}
    if(rank==6) {node_ptr = new Node(rank, "p", neighbore6);}
    int start = 0;
    int value = 0;
    Node& node = (*node_ptr);
    
    
    if(node.getRank()==0){
        std::string command;
        bool flag = true;
        value =1;
        while (flag) 
        {
            std::cin>>command;
            if (command=="start"){
                MPI_Bcast(&value,1, MPI_INT,0, MPI_COMM_WORLD);
                flag=false;
            }
            else{
                std::cout<<"Type start \n";
            }
        }   
    }

    if((node.getRank()!=0)){
        bool flag = true;
        while (flag)
        {   
            MPI_Bcast(&value,1, MPI_INT,0, MPI_COMM_WORLD);
            if (value!=0){
                flag=false;
            }
        }
        std::cout << "Rank " << node.getRank() << " value " << value << "\n";
    }

    MPI_Finalize();
    return 0;
}