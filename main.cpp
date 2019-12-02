#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include "mpi.h"
#include "stddef.h"

struct Message {
    enum Message_Type {
        End = 0,
        WakeUp = 1,
        Control = 2,
        Announce = 3,
    };
    int m_id=0;
    int m_type=0;
    int m_data=0;
    int m_source=0;
    std::string getMessageType() const
    {   
        switch(m_type)
        {
            case 0:
                return "End";
            case 1:
                return "WakeUp";
            case 2:
                return "Control";
            case 3:
                return "Announce";
            default:
                return "Invalid Type";
        }
    }
};


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
    
    int struct_member_variable_counter = 4;
    int struct_member_variable_block_lengths[struct_member_variable_counter] = {1,1,1,1};
    MPI_Datatype struct_member_variable_type_array[struct_member_variable_counter] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint struct_member_variable_displacement[struct_member_variable_counter] { offsetof(Message,m_id),offsetof(Message,m_type), offsetof(Message,m_data),offsetof(Message,m_source)};
    MPI_Datatype message_struct;
    MPI_Type_create_struct( struct_member_variable_counter, 
                            struct_member_variable_block_lengths,
                            struct_member_variable_displacement,
                            struct_member_variable_type_array,
                            &message_struct
                            );
    MPI_Type_commit(&message_struct);

    Message wake_up_msg;
    if(node.getRank()==0){
        std::string command;
        bool flag = true;
        value =1;
        while (flag) 
        {
            std::cout << "Type \"start\" to start: ";
            std::cin >> command;
            if (command=="start"){
                wake_up_msg.m_id = rank;
                wake_up_msg.m_type = 1;
                wake_up_msg.m_source = rank;
                MPI_Bcast(&wake_up_msg,1, message_struct,0, MPI_COMM_WORLD);
                flag=false;
            }
            else{
                std::cout << "Type \"start\" to start"<< std::endl;
            }
        }   
    }

    if((node.getRank()!=0)){
        bool flag = true;
        while (flag)
        {   
            MPI_Bcast(&wake_up_msg,1, message_struct,0, MPI_COMM_WORLD);
            if (wake_up_msg.m_type!=0){
                flag=false;
            }
        }
        std::cout << "Rank: " << node.getRank() << " Message_Type: " << wake_up_msg.getMessageType() << " source: " << wake_up_msg.m_source << "\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    delete node_ptr;
    node_ptr = NULL;
    return 0;
}