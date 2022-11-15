/*
Embedded Software Engineer Challenge Question 2 - Basic Message Passing Library
Author: Zaid Al-Khatib
Contact: nolosian@gmail.com
Date delivered: 11-15-2022

IDE & Compiler used to develop the library and test application:
    Microsoft Visual Studio Community 2022
    Version 17.3.6
    VisualStudio.17.Release/17.3.6+32929.385
    Visual C++ 2022   00482-90000-00000-AA536

    C++ Language Standard option set to: ISO C++20 Standard (/std:c++20)
    Project C++ Build All Options:
        /JMC /permissive- /ifcOutput "x64\Debug\" /GS /W3 /Zc:wchar_t /ZI /Gm- /Od /sdl /Fd"x64\Debug\vc143.pdb" 
        /Zc:inline /fp:precise /D "_DEBUG" /D "_CONSOLE" /D "_UNICODE" /D "UNICODE" /errorReport:prompt /WX- 
        /Zc:forScope /RTC1 /Gd /MDd /std:c++20 /FC /Fa"x64\Debug\" /EHsc /nologo /Fo"x64\Debug\" 
        /Fp"x64\Debug\kepler_embedded_sw_challenge_1.pch" /diagnostics:column

Assumptions: 
- Messages are expected to be received in FIFO order
- Data length valid range: [0-MAX_DATA_LENTH] inclusive. I.e. 0 length messages are legal.
- Any thread is allowed to read a message from the queue of any other thread
- The action of sending a message can be received once. The message itself is not destroyed on receive
- This library is to be used on a device with maximum 256 HW concurent threads 
- The library is to be block-free, threads calling recv function for an empty threa_id queue should not be blocked to wait for a message
    if the queue to a thread is empty on receive, return an error code and don't block calling thread
- The Basic Messaage Passing Library is to be implemented without using STL or Boost data structures like std::queue or boost::lockfree::queue 
- The library does not need to be lock-free; Mutexes, condition variables, and lock guards can be used to protect shared resources during non-atomic access operations.
- Felxible memory allocation is prefered to pre-reserving memory capacity for the message queueing
    rely on the OS to dynamically allocate memory from the heap instead of using a fixed size circular buffer.
- The library is assumed designed for an embedded system with limited number of hardware_concurrency support and a known upper limit imposoed by CPU architecture 
    (common Intel X86-64 CPUs are less than 16) 
- The test app is to fail and throw an exception when a test case fails to produce the expected output; use assert statements
Possible improvements: 
    - Use a thread pool for the test application instead of creating and deleting test threads.
    - Syncrhonize access to std::cout using std::basic_osyncstream in the test app, instead of the global mutex & lock guards
    - Use Joinable threads in the test app to avoid
    - Lock guard scopes in the library likely posssible to be optimized, made smaller, seperate mutexes for a queue pointers can be used - espicially the delete_message function
*/

#include <iostream>
#include <thread>
#include <mutex> 
#include <semaphore>
#include <algorithm>
#include <vector>
#include <cassert>

#include "BasicMessagePassing.h"


std::mutex m_cout;

std::counting_semaphore sem_th0_count_packets{ 0 };                                            // to count packets queued for thread_id 0
std::counting_semaphore sem_th1_count_packets{ 0 };                                            // to count packets queued for thread_id 1

// simple test of library functions using 3 threads
void Test1ProducerTh(BasicMessagePassing* bmp); // creates messgae objects, sends to 2 Consumer threads. Doesn't need to receive messages; doesn't need an ID
void Test1ConsumerTh0(BasicMessagePassing* bmp); // waits for available messages passed to it, deletes received messages after using them
void Test1ConsumerTh1(BasicMessagePassing* bmp); // waits for available messages passed to it, deletes received messages after using them

// Bad user test - bas function parameter values & memory allocation exceptions
void BadUserThread(BasicMessagePassing* bmp);

// Performance Evaluation Test - using semaphores, continously sending and receiving messages
void ProducerTh(uint8_t thread_id, BasicMessagePassing* bmp, unsigned int max_thread_count);
void ConsumerTh(uint8_t thread_id, BasicMessagePassing* bmp);


int main(){
    //BasicMessagePassing basic_message_passing_uut;
    BasicMessagePassing * p_basic_message_passing_uut;


    std::cout << "Testing Basic Message Passing Library." << std::endl;
    std::cout << "Test 1: a producer thread creates fixed nmber of messages and sends them in a deterministic way to 2 consumer threads" << std::endl;
    p_basic_message_passing_uut = new BasicMessagePassing();
    std::thread producer_thread(Test1ProducerTh, p_basic_message_passing_uut); // thread id is 0 but it doesn't really need it for this test
    std::thread consumer_thread1(Test1ConsumerTh0, p_basic_message_passing_uut);
    std::thread consumer_thread2(Test1ConsumerTh1, p_basic_message_passing_uut);

    producer_thread.join();
    consumer_thread1.detach();
    consumer_thread2.detach();
    delete p_basic_message_passing_uut;

    

    /*
    // Test with 10 producer threads sending data to 8 consumer threads
    std::thread producers[10];
    std::thread consumers[8];
    unsigned int created_thread_count = 0;
    unsigned int max_thread_count = std::min((unsigned int)MAX_THREADS_POSSIBLE, std::max((unsigned int)3, std::thread::hardware_concurrency()));


//    for (auto p : producers) {
//        p = std::thread(ProducerTh, created_thread_count++, std::ref(basic_message_passing_uut), max_thread_count);
    for(int i=0; i<10 ;i++){
        producers[i] = std::thread(ProducerTh, created_thread_count++, std::ref(basic_message_passing_uut), max_thread_count);
    }

    for (int i = 0; i < 10; i++) {
        producers[i].join();
    }

    // Test with at least 3 threads, up to the maximum the hardware can support (dev & testing with 16 thread supper) or 256
   // unsigned int 
    created_thread_count = 0;
    std::cout << "Test 2: Testing with maximum hardware utilization\n";

    max_thread_count = std::min((unsigned int)MAX_THREADS_POSSIBLE, std::max((unsigned int)3, std::thread::hardware_concurrency()));
    std::vector<std::thread> thread_pool(max_thread_count);
    srand((unsigned)time(NULL));

    std::cout << "Hardware concurrency supports " << max_thread_count << " threads\n";
    std::cout << "Initializingn " << max_thread_count << " threads to test the BasicMessagePassing library\n";

    // initializing at least 3 threads (1 writer and 2 consumers)
    // for threads 4-hardware_concurrency randomly choose a thread type

    thread_pool.at(created_thread_count) = std::thread(ProducerTh, created_thread_count, std::ref(basic_message_passing_uut), max_thread_count);
    created_thread_count++;

    thread_pool.at(created_thread_count) = std::thread(ConsumerTh, created_thread_count, std::ref(basic_message_passing_uut));
    created_thread_count++;

    thread_pool.at(created_thread_count) = std::thread(ConsumerTh, created_thread_count, std::ref(basic_message_passing_uut));
    created_thread_count++;

    for (; created_thread_count < max_thread_count; created_thread_count++) {
        thread_pool.at(created_thread_count) = std::thread(ConsumerTh, created_thread_count, std::ref(basic_message_passing_uut)); // TODO: randomize th function
    }


    for (unsigned int i = 0; i < created_thread_count; i++) {
        thread_pool.at(i).join();
    }

    
    //Tset with a bad user thread that will use invalid inputs to test library input validation
    
    */

    std::cout << "Test 3: Bad user thread will attempt to use invalid library API input values\n";
    p_basic_message_passing_uut = new BasicMessagePassing();
    std::thread bad_thread(BadUserThread, std::ref(p_basic_message_passing_uut));
//    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    bad_thread.join();
    delete p_basic_message_passing_uut;

    std::cout << "Test program completed execution sucessfully." << std::endl;
    return 0;
}


void Test1ProducerTh(BasicMessagePassing* bmp) {
    unsigned int status;
    message_t* msg0 = bmp->new_message();
    message_t* msg1 = bmp->new_message();
    message_t* msg2 = bmp->new_message();

    // write something in the new message
    msg0->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    for (int i = 0; i < msg0->len; i++)  msg0->data[i] = 0x13;
    msg1->len = (int)rand() % (MAX_DATA_LENTH + 1);
    for (int i = 0; i < msg1->len; i++)  msg1->data[i] = 0x13;
    msg2->len = (int)rand() % (MAX_DATA_LENTH + 1);
    for (int i = 0; i < msg2->len; i++)  msg2->data[i] = 0x13;

    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread created 3 messages, lengths: " << (int)msg0->len << ' ' << (int)msg1->len << ' ' << (int)msg2->len <<  " to send to threads 0 - 2" << std::endl;
    }

    status = bmp->send(0, msg0);
    assert(status == BasicMessagePassing::SUCCESS);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread sent msg 0 to thread 0" << std::endl;
    }
    sem_th0_count_packets.release(); // Signal thread 0 that a message is added to its queue

    status = bmp->send(1, msg1);
    assert(status == BasicMessagePassing::SUCCESS);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread sent msg 1 to thread 1" << std::endl;
    }
    sem_th1_count_packets.release(); // Signal thread 1 that a message is added to its queue

    status = bmp->send(2, msg2);
    assert(status == BasicMessagePassing::SUCCESS);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread sent msg 2 to thread 2 (doesn't exist, should be OK)" << std::endl;
    }
    
    status = bmp->send(0, msg2);
    sem_th0_count_packets.release(); // add 1 to semaphore counting messages sent to thread 1
    assert(status == BasicMessagePassing::SUCCESS);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread sent msg 2 to thread 0" << std::endl;
    }

    status = bmp->send(2, msg2);
    status = bmp->send(2, msg2);
    assert(status == BasicMessagePassing::SUCCESS);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread sent msg 2 to thread two times, 3 sends of msg 2 are now in queue." << std::endl;
    }

    bmp->delete_message(msg2);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread deleted the msg2 object. (should empty the queue to thread 2)" << std::endl;
    }

    message_t* recv_msg;
    status = bmp->recv(2, recv_msg);
    assert(status == BasicMessagePassing::THREAD_QUEUE_EMPTY);
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread attempted to read a msg in the queue to thread 2 (which should be empty now)." << std::endl;
    }


    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread will sleep for 4 seconds, then delete the remaining messages and exit" << std::endl;
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(4000)); // Producer thread will sleep for a second, allowing consumers to receive messages
    bmp->delete_message(msg0);
    bmp->delete_message(msg1);
}

void Test1ConsumerTh0(BasicMessagePassing* bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Consumer thread 0 will wait to acquire the semaphore of its oqueue, and display the length of any received message" << std::endl;
    }
    message_t* msg_recevied = NULL;
    int status;
    while (1) {
        sem_th0_count_packets.acquire();
        status = bmp->recv(0, msg_recevied);
        {
            if(status == BasicMessagePassing::SUCCESS)
            {
                assert(msg_recevied != NULL);
                std::lock_guard<std::mutex> lg_cout(m_cout);
                std::cout << "  Consumer thread 0 received msg of length: " << (unsigned int)msg_recevied->len << std::endl;
            }
            else {
                std::lock_guard<std::mutex> lg_cout(m_cout);
                std::cout << "ERR  Consumer thread 0 encountered an error trying to receive a message." << std::endl;
            }
        }
    }
}
void Test1ConsumerTh1(BasicMessagePassing* bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Consumer thread 1 will wait to acquire the semaphore of its oqueue, and display the length of any received message" << std::endl;
    }
    message_t* msg_recevied = NULL;
    int status;
    while (1) {
        sem_th1_count_packets.acquire();
        status = bmp->recv(1, msg_recevied);
        {
            if (status == BasicMessagePassing::SUCCESS) {
                assert(msg_recevied != NULL);
                std::lock_guard<std::mutex> lg_cout(m_cout);
                std::cout << "  Consumer thread 1 received msg of length: " << (unsigned int)msg_recevied->len << std::endl;
            }
        }
    }
}


void BadUserThread(BasicMessagePassing* bmp) {
    unsigned int status;
    message_t* msg = NULL;
    
    std::lock_guard<std::mutex> lg_cout(m_cout);    // to be kept for the duration of the bad user test
    std::cout << "Test Bad User - Thread will test error conditions, use of impropper function parameters and overflow queues" << std::endl;

    std::cout << "              - delete(NULL) - should display err message" << std::endl;
    bmp->delete_message(NULL);
    std::cout << "              - delete((message_t*)&status) - Skipped, Known Fails" << std::endl;
    //bmp->delete_message((message_t*)&status); //  TODO -- validate a malicious addresses that doesn't point to like this 
    std::cout << "              - send(-1, NULL) - should return INVALID_DESTINATION_ID" << std::endl;
    status = bmp->send(-1, NULL);
    assert(status == BasicMessagePassing::INVALID_DESTINATION_ID);
    std::cout << "              - send(2542, NULL) - should return INVALID_DESTINATION_ID" << std::endl;
    status = bmp->send(2542, NULL);
    assert(status == BasicMessagePassing::INVALID_DESTINATION_ID);
    std::cout << "              - send(2, NULL) - should return INVALID_MSG_ADDRESS" << std::endl;
    status = bmp->send(2, NULL);
    assert(status == BasicMessagePassing::INVALID_MSG_ADDRESS);
    std::cout << "              - recv(-2, NULL) - should return INVALID_RECEIVER_ID" << std::endl;
    status = bmp->recv(-2, msg);
    assert(status == BasicMessagePassing::INVALID_RECEIVER_ID);
    std::cout << "              - recv(0, NULL) - should return THREAD_QUEUE_EMPTY" << std::endl;
    status = bmp->recv(0, msg);
    assert(status == BasicMessagePassing::THREAD_QUEUE_EMPTY);
    std::cout << "              - recv(9876, NULL) - should return INVALID_RECEIVER_ID" << std::endl;
    status = bmp->recv(9876, msg);
    assert(status == BasicMessagePassing::INVALID_RECEIVER_ID);
    std::cout << "              - recv(0, msg) - should return THREAD_QUEUE_EMPTY" << std::endl;
    status = bmp->recv(0, msg);
    assert(status == BasicMessagePassing::THREAD_QUEUE_EMPTY);
    
    return;
}

void ProducerTh(uint8_t thread_id, BasicMessagePassing* bmp, unsigned int max_thread_count) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[+] Producer thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    }

    /*  for (unsigned int i = 0; i < max_thread_count; i++) {
        if (i == thread_id) continue; // don't send to self -- for now at least
        bmp->send(i, bmp->new_message());
    }
  */

}
void ConsumerTh(uint8_t thread_id, BasicMessagePassing* bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[-] Consumer thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    }
}
