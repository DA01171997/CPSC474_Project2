#include <iostream>
#include <vector>
#include <map>
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

            // Message type.
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

            // Message Constructor.
            Message(Message_Type type, int source, int data = 0, int destination = 0) : m_type(static_cast<int>(type)), 
                                                                                        m_source(source), 
                                                                                        m_data(data),
                                                                                        m_destination(destination)
            {
            }

            // Message Constructor.
            Message(int type, int source, int data = 0, int destination = 0) :  m_type(type),
                                                                                m_source(source),
                                                                                m_data(data),
                                                                                m_destination(destination)
            {
            }

            // Return string Message type.
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

            // Return Message type.
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

        // Default Node constructer. 
        Node()
        {
            createMessageStruct();
            setupNeighboreMap();    
        }

        // Node constructer. 
        Node(int rank): m_rank(rank) {
            m_biggest_rank = m_rank;
            createMessageStruct();
            setupNeighboreMap();
        }

        // Node constructer.
        Node(int rank, const std::string& name, const std::vector<int>& neighbore): m_rank(rank), 
                                                                                    m_name(name), 
                                                                                    m_neighbore(neighbore)
        {
            m_biggest_rank = m_rank;
            createMessageStruct();
            setupNeighboreMap();
        }

        // Clean up pointers.
        ~Node()
        {
            delete m_recv_thread; 
            m_recv_thread = NULL;
            delete m_send_thread;
            m_send_thread = NULL;
            delete m_process_thread;
            m_process_thread = NULL;
        }

        // Return Node rank.
        int getRank() const {return m_rank;}

        void Receive()
        {
            int counter = 0;
            bool flag = true;         
            while(flag)
            {  
                // Prob to see if there are any messages to receive,
                // if amount !=0 mean there are message, the receive.
                int amount;
                Message M;  
                MPI_Status status;
                MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                MPI_Get_count(&status, MPI_INT, &amount);
                if (amount!=0)
                {

                    // Aquire locker to push messages to the recv queue,
                    // notify conditional variable that a new message
                    // has been added.
                    MPI_Recv(&M,amount, m_message_struct, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    {
                        std::unique_lock<std::mutex> locker(m_mutex_recv_q);
                        m_recv_queue.push(M);
                        m_conv_recv_q.notify_one();
                    }
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        void Send()
        {
            bool flag = true;
            while(flag){

                // Wait till condition variable trigger to relock the send queue
                // in order to pop off message for sending.
                Message M;
                {
                    std::unique_lock<std::mutex> locker(m_mutex_send_q);
                    while(m_send_queue.empty())
                    {
                        m_conv_send_q.wait(locker);
                    }
                    M = m_send_queue.front();
                    m_send_queue.pop();
                }

                // Send Message 
                MPI_Send(&M,1, m_message_struct, M.m_destination, 0, MPI_COMM_WORLD);
            }
        }

        void Process()
        {
            bool flag = true;
            bool flag2 = false;
            while(flag)
            {
                // Wait till condition variable trigger to relock the recv queue
                // in order to pop off message for processing.
                Message M;
                {
                    std::unique_lock<std::mutex> locker(m_mutex_recv_q);
                    while(m_recv_queue.empty())
                    {
                        m_conv_recv_q.wait(locker);
                    }
                    M = m_recv_queue.front();
                    m_recv_queue.pop();
                }
                std::string s = "Node::Process: " + m_name + " rank: " + std::to_string(m_rank) + " receive: " + M.getMessageTypeStr() + " from: " + std::to_string(M.m_source) + " data: " + std::to_string(M.m_data) + "\n";
                std::cout<<s;

                // Process the message.
                switch(M.getMessageType()){
                    
                    // Intented to exit thread; (doesn't work atm).
                    case Message::Message_Type::End:
                        {
                        // flag = false;
                        break;
                        }

                    // Trigger flag2 which will trigger the election.
                    case Message::Message_Type::WakeUp:
                        {
                        flag2 = true;
                        break;
                        }

                    // Calculate the biggest data from Control message.
                    // If it receive Control message from it parent,
                    // it will decide and send Announce message to 
                    // all its neighbore except the parent. 
                    case Message::Message_Type::Control:
                        {
                            updateBiggest(M.m_data);
                            m_neighbore_map.erase(M.m_source);
                            if (M.m_source==m_parent_rank){
                                for (int i = 0; i<m_neighbore.size(); i++)
                                {
                                    if(m_neighbore[i]!=m_parent_rank)
                                    {
                                        Message M2(Message::Message_Type::Announce, m_rank, m_biggest_rank,m_neighbore[i]);
                                        addMessageToSendQueue(M2);
                                    }
                                }       
                            }
                        }
                        break;
                    
                    // Elect itself it is receive an Announce message with data == rank.
                    case Message::Message_Type::Announce:
                        if (M.m_data==m_rank)
                        {
                            s = "Node::Process: " + m_name + " rank: " + std::to_string(m_rank) + " *************I am elected!*************\n";
                            std::cout<<s;
                        }
                        break;
                    default:
                        break;
                }

                // Wait until it has received message from all neighbore except one,
                // make it the parent, then send message to it.
                if (flag2)
                {
                    
                    if(m_neighbore_map.size()==1)
                    {
                        std::map<int,int>::iterator it = m_neighbore_map.begin();
                        m_parent_rank = it->first;
                        Message M2 (Message::Message_Type::Control, m_rank, m_biggest_rank,m_parent_rank);
                        addMessageToSendQueue(M2);
                    }
                }
            }
        }

        // Use to add message to the send thread with already constructed Message.
        void addMessageToSendQueue(const Message& M) 
        {
            std::unique_lock<std::mutex> locker(m_mutex_send_q);
            m_send_queue.push(M);
            m_conv_send_q.notify_one();
        }

        // Use to add message to the send thread.
        void addMessageToSendQueue(int type, int source, int data = 0, int destination = 0)
        {
            Message M(type, source, data, destination);
            std::unique_lock<std::mutex> locker(m_mutex_send_q);
            m_send_queue.push(M);
            m_conv_send_q.notify_one();
        }

        // Use to spawn the threads.
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

        // Use to joins all threads.
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

        // Use by process 0 to start a wakeup sequence.
        void startElection(){
            for(int i = 0; i < m_neighbore.size(); i++){
                addMessageToSendQueue(1, m_rank, 0, m_neighbore[i]);
            }
        }
    private:
        int m_rank;
        int m_parent_rank;
        int m_biggest_rank;
        std::string m_name="";
        std::vector<int> m_neighbore;
        std::map<int,int> m_neighbore_map;
        std::queue<Message> m_recv_queue;
        std::queue<Message> m_send_queue;
        std::thread* m_recv_thread;
        std::thread* m_send_thread;
        std::thread* m_process_thread;
        std::mutex m_mutex_recv_q;
        std::mutex m_mutex_send_q;
        std::condition_variable m_conv_recv_q;
        std::condition_variable m_conv_send_q;
        MPI_Datatype m_message_struct;

        // Create a Message struct for MPI to send.
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
                                &m_message_struct
                                );
            MPI_Type_commit(&m_message_struct);
        }

        // Make a map of all neighbore for election process.
        void setupNeighboreMap()
        {
            for(int i =0; i<m_neighbore.size(); i++)
            {
                m_neighbore_map.insert(std::make_pair(m_neighbore[i],m_neighbore[i]));
            }
        }

        // Use to check for the highest ID.
        void updateBiggest(int num){
            m_biggest_rank = (m_biggest_rank > num) ? m_biggest_rank : num;
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

    // Initialize the topology of each process base on rank.
    std::vector<int> neighbore0 {1,2,3,4,5,6};
    std::vector<int> neighbore1 {4};
    std::vector<int> neighbore2 {3,4,5};
    std::vector<int> neighbore3 {2};
    std::vector<int> neighbore4 {1,2,6};
    std::vector<int> neighbore5 {2};
    std::vector<int> neighbore6 {4};

    Node* node_ptr;
    if(rank==0) {node_ptr = new Node(rank, "o", neighbore0);}
    if(rank==1) {node_ptr = new Node(rank, "q", neighbore1);}
    if(rank==2) {node_ptr = new Node(rank, "s", neighbore2);}
    if(rank==3) {node_ptr = new Node(rank, "t", neighbore3);}
    if(rank==4) {node_ptr = new Node(rank, "r", neighbore4);}
    if(rank==5) {node_ptr = new Node(rank, "u", neighbore5);}
    if(rank==6) {node_ptr = new Node(rank, "p", neighbore6);}
    Node& node = (*node_ptr);

    // Synchronize all process to make sure all got initialize correctly.
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Try to start election process with process 0.
    try 
    {
        // Spawn receive, send, process threads for all processes.
        node.startThreads();

        // Process 0 start the election sequence.
        if (rank==0){
            node.startElection();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // node.SendEnd();
        }
    } catch (...)
    {
        std::cout<< "Error " << rank << "\n";
        node.terminateThreads();
        throw;
    }

    // Clean up 
    node.terminateThreads();
    MPI_Finalize();
    delete node_ptr;
    node_ptr = NULL;
    return 0;
}