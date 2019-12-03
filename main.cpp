#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include "mpi.h"
#include "stddef.h"


class Node 
{       
    public:
        struct Message {
            enum Message_Type {
                End = 0,
                WakeUp = 1,
                Control = 2,
                Announce = 3,
                Invalid_Type = 4
            };
            int m_type=0;
            int m_data=0;
            int m_source=0;
            int m_destination=0;

            Message(Message_Type type, int source, int data = 0, int destination = 0) : m_type(static_cast<int>(type)), 
                                                                                        m_source(source), 
                                                                                        m_data(data),
                                                                                        m_destination(destination)
            {
            }
            Message(int type, int source, int data = 0, int destination = 0) :  m_type(type),
                                                                                m_source(source),
                                                                                m_data(data),
                                                                                m_destination(destination)
            {
            }
            Message() {}
            std::string getMessageTypeStr() const
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
            Message_Type getMessageType() const
            {   
                switch(m_type)
                {
                    case 0:
                        return Message_Type::End;
                    case 1:
                        return Message_Type::WakeUp;
                    case 2:
                        return Message_Type::Control;
                    case 3:
                        return Message_Type::Announce;
                    default:
                        return Message_Type::Invalid_Type;
                }
            }
        };

        Node()
        {
            createMessageStruct();    
        }
        Node(int rank): m_rank(rank) {
            createMessageStruct();
        }
        Node(int rank, const std::string& name, const std::vector<int>& neighbore): m_rank(rank), 
                                                                                    m_name(name), 
                                                                                    m_neighbore(neighbore)
        {
            createMessageStruct();
            m_buffer.reserve(m_neighbore.size());
            std::string validate_message = "Node: " + m_name + ", rank: " + std::to_string(m_rank) + ", neighbore: ";
            for(int i=0; i<m_neighbore.size(); i++)
            {
                validate_message += std::to_string(m_neighbore[i]) + " ";
            }
            validate_message += "\n";
            // std::cout << validate_message;
        }
        ~Node()
        {
            delete m_recv_thread; 
            m_recv_thread = NULL;
            delete m_send_thread;
            m_send_thread = NULL;
            delete m_process_thread;
            m_process_thread = NULL;
        }

        
        int getRank() const {return m_rank;}

        void Receive()
        {
            int counter = 0;         
            while((counter <10))
            {
                try 
                {   
                    int amount;
                    Message M;  
                    MPI_Status status;
                    MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                    MPI_Get_count(&status, MPI_INT, &amount);
                    if (amount!=0)
                    {
                        MPI_Recv(&M,amount, message_struct, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        {
                            std::unique_lock<std::mutex> locker(m_mutex);
                            m_recv_queue.push(M);
                            m_conv_recv_q.notify_one();
                        }
                    }
                    
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                } catch (...)
                {
                    std::cout<< "In: error " << m_rank << "\n";
                    throw;
                }
                counter++;
            }
        }

        void Send()
        {
            if (m_rank==3) 
            {
                int destination = 4;
                Message M (Message::WakeUp, m_rank, 9, destination);
                MPI_Send(&M,1, message_struct, destination, 0, MPI_COMM_WORLD);
            }
            if (m_rank==4) 
            {
                int destination = 2;
                Message M (Message::WakeUp, m_rank, 10, destination);
                MPI_Send(&M,1, message_struct, destination, 0, MPI_COMM_WORLD);
            }
            if (m_rank==4) 
            {
                int destination = 3;
                Message M (Message::WakeUp, m_rank, 11, destination);
                MPI_Send(&M,1, message_struct, destination, 0, MPI_COMM_WORLD);
            }
            if (m_rank==5) 
            {
                int destination = 3;
                Message M (Message::WakeUp, m_rank, 12, destination = 3);
                MPI_Send(&M,1, message_struct, destination, 0, MPI_COMM_WORLD);
            }
        }

        void Process()
        {
            bool flag = true;
            while(flag)
            {
                Message M;
                {
                    std::unique_lock<std::mutex> locker(m_mutex);
                    while(m_recv_queue.empty())
                    {
                        m_conv_recv_q.wait(locker);
                    }
                    M = m_recv_queue.front();
                    m_recv_queue.pop();
                }
                std::string tempS = std::to_string(m_rank) + " value : " + std::to_string(M.m_data) + " from: " +  std::to_string(M.m_source) + " type: " + M.getMessageTypeStr() + " dest : " + std::to_string(M.m_destination) + "\n";
                std::cout<< tempS;
                switch(M.getMessageType()){
                    case Message::Message_Type::End:
                        {
                        break;
                        }
                        
                    case Message::Message_Type::WakeUp:
                        {
                        std::cout<<"HERE\n";
                        break;
                        }
                    case Message::Message_Type::Control:
                        break;
                    case Message::Message_Type::Announce:
                        break;
                    default:
                        break;
                }
                
            }
        }
        void startThreads()
        {
            
            if (!m_recv_thread)
            {
                m_recv_thread = new std::thread(&Node::Receive, this);
            }
            if (!m_send_thread)
            {
                m_send_thread = new std::thread(&Node::Send, this);
            }
            if (!m_process_thread)
            {
                m_process_thread = new std::thread(&Node::Process, this);
            }
        }

        void terminateThreads() 
        {
            if ((m_recv_thread)&&(m_recv_thread->joinable()))
            {
                m_recv_thread->join();
            }
            if ((m_send_thread)&&(m_send_thread->joinable()))
            {
                m_send_thread->join();
            }
            if ((m_process_thread)&&(m_process_thread->joinable()))
            {
                m_process_thread->join();
            }
        }
    private:
        int m_rank;
        int test =0;
        std::string m_name="";
        std::vector<int> m_neighbore;
        std::vector<int> m_buffer;
        std::queue<Message> m_recv_queue;
        std::queue<Message> m_send_queue;
        std::thread* m_recv_thread;
        std::thread* m_send_thread;
        std::thread* m_process_thread;
        std::mutex m_mutex;
        std::condition_variable m_conv_recv_q;
        MPI_Datatype message_struct;
        Message m_recv_msg; 
        void createMessageStruct()
        {   
            int struct_member_variable_counter = 4;
            int struct_member_variable_block_lengths[struct_member_variable_counter] = {1,1,1,1};
            MPI_Datatype struct_member_variable_type_array[struct_member_variable_counter] = {MPI_INT, MPI_INT, MPI_INT,MPI_INT};
            MPI_Aint struct_member_variable_displacement[struct_member_variable_counter] {  offsetof(Message,m_type),
                                                                                            offsetof(Message,m_data),
                                                                                            offsetof(Message,m_source),
                                                                                            offsetof(Message,m_destination)};
            MPI_Type_create_struct( struct_member_variable_counter, 
                                struct_member_variable_block_lengths,
                                struct_member_variable_displacement,
                                struct_member_variable_type_array,
                                &message_struct
                                );
            MPI_Type_commit(&message_struct);
        }
};


int main(int argc, char * argv[]) 
{
    // Check to make sure that the program start with 7 processes. 
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(size!=7)
    {
        std::cout<<"Error: Need to run program with 7 processes \n";
        return 1;
    }
    // Initialize the topology of each process base on rank
    std::vector<int> neighbore1 {4};
    std::vector<int> neighbore2 {3,4,5};
    std::vector<int> neighbore3 {2};
    std::vector<int> neighbore4 {1,2,6};
    std::vector<int> neighbore5 {2};
    std::vector<int> neighbore6 {4};

    Node* node_ptr;
    if(rank==0) {node_ptr = new Node(rank);}
    if(rank==1) {node_ptr = new Node(rank, "q", neighbore1);}
    if(rank==2) {node_ptr = new Node(rank, "s", neighbore2);}
    if(rank==3) {node_ptr = new Node(rank, "t", neighbore3);}
    if(rank==4) {node_ptr = new Node(rank, "r", neighbore4);}
    if(rank==5) {node_ptr = new Node(rank, "u", neighbore5);}
    if(rank==6) {node_ptr = new Node(rank, "p", neighbore6);}
    Node& node = (*node_ptr);

    // Node's rank == 0 starts the Wakeup Process when user entered "start".
    // if(node.getRank()==0)
    // {
    //     std::string command;
    //     bool flag = true;
    //     while (flag) 
    //     {
    //         std::cout << "Type \"start\" to start: ";
    //         std::cin >> command;
    //         if (command=="start")
    //         {
    //             Message wake_up_msg(Message::WakeUp, rank, 9);
    //             MPI_Bcast(&wake_up_msg,1, message_struct,0, MPI_COMM_WORLD);
    //             flag=false;
    //         }
    //         else
    //         {
    //             std::cout << "Type \"start\" to start"<< std::endl;
    //         }
    //     }   
    // }

    // // Node's rank !=0 will wait for the WakeUp message.
    // if((node.getRank()!=0))
    // {
    //     bool flag = true;
    //     Message recv_msg;
    //     while (flag)
    //     {   
    //         MPI_Bcast(&recv_msg,1, message_struct,0, MPI_COMM_WORLD);
    //         if (recv_msg.getMessageType() == Message::Message_Type::WakeUp)
    //         {
    //             flag=false;
    //         }
    //     }
    //     // std::cout << "Rank: " << node.getRank() << " Message_Type: " << recv_msg.getMessageTypeStr() << " source: " << recv_msg.m_source << " data: " << recv_msg.m_data << "\n";
        
    // }

    
    MPI_Barrier(MPI_COMM_WORLD);
    try 
    {
        node.startThreads();
        // std::this_thread::sleep_for(std::chrono::seconds(3));
        // node.change();
    } catch (...)
    {
        std::cout<< "Out: error " << rank << "\n";
        // MPI_Barrier(MPI_COMM_WORLD);
        node.terminateThreads();
        // MPI_Barrier(MPI_COMM_WORLD);
        throw;
    }

    // MPI_Barrier(MPI_COMM_WORLD);
    node.terminateThreads();
    // MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    delete node_ptr;
    node_ptr = NULL;
    
    return 0;
}