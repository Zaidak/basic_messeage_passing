/*
Embedded Software Engineer Challenge Question 2 - Basic Message Passing Library
Zaid Al-Khatib
nolosian@gmail.com
11-14-2022

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
- Any thread is allowed to read a message from the 
- This library is to be used on a device with maximum 256 HW concurent threads 
- NON-BLCOKIUNG MESSAGE PASSING
- The Basic Messaage Passing Library is to be implemented without using STL or Boost data structures like std::queue or boost::lockfree::queue 
- The library is to be block-free, threads calling recv function for an empty threa_id queue should not be blocked to wait for a message
- The library does not need to be Lock-free; Mutexes, condition variables, and lock guards can be used to protect shared resources during non-atomic access operations.
- Pre-reserving memory capacity for the message queueing is not required; rely on the OS to dynamically allocate memory from the heap instead of using a fixed size circular buffer.
- Because thread IDs are stroed in uint8_t (maximum value of 255), the library is assumed designed for an embedded system with a limited number of thread hardware_concurrency 
    support and a known upper limit imposoed by CPU architecture (e.x. less than 255 thread as for most common Intel X86-64 CPUs) 
- The test app is to fail and throw an exception when a test case fails to produce the expected output; use assert statements
Possible improvements: 
    Library Implementation:
        - Implement a lock-free message passing library using Shared Pointers & Atomic operations
    Test App 
        - Use a thread pool for the test application instead of creating and deleting test threads.
        - Syncrhonize access to std::cout using std::basic_osyncstream in the test app, instead of the global mutex & lock guards
        - Use Joinable threads in the test app to avoid
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

//std::counting_semaphore sem_count_sent_msgs[MAX_THREADS_POSSIBLE];  // array of semaphores to synchronize producer and consumer test threads
//std::counting_semaphore sem_th0_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th1_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th2_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th3_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th4_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th5_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th6_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//std::counting_semaphore sem_th7_count_packets{ 0 };                                            // to count packets queued for thread_id 0
//void InitSemaphores();

void Test1ProducerTh(BasicMessagePassing& bmp); // creates messgae objects, sends to 2 Consumer threads. Doesn't need to receive messgae; doesn't need an ID
void Test1ConsumerTh(uint8_t thread_id, BasicMessagePassing& bmp); // waits for available messages passed to it, deletes received messages after using them
//void Test1ReceiverTh(uint8_t thread_id, BasicMessagePassing& bmp); // waits for available messages passed to it, does not delete the message after receiving it

// Bad user test - bas function parameter values & memory allocation exceptions
void BadUserThread(BasicMessagePassing& bmp);

// Performance Evaluation Test - using semaphores, continously sending and receiving messages

void ProducerTh(uint8_t thread_id, BasicMessagePassing& bmp, unsigned int max_thread_count);
void ConsumerTh(uint8_t thread_id, BasicMessagePassing& bmp);
//void MonitorTh(uint8_t thread_id, BasicMessagePassing& bmp);

std::counting_semaphore sem_th0_count_packets{ 0 };                                            // to count packets queued for thread_id 0
std::counting_semaphore sem_th1_count_packets{ 0 };                                            // to count packets queued for thread_id 1

int main(){
    m_cout.unlock();
    BasicMessagePassing basic_message_passing_uut;
    std::cout << "Testing Basic Message Passing Library." << std::endl;
    /*
    // Test with thread_id 0 creating and sending multiple messages for basic functional testing
    std::cout << "Test 1: a producer thread creates fixed nmber of messages and sends them in a deterministic way to 2 consumer threads" << std::endl;
    std::thread producer_thread(Test1ProducerTh, std::ref(basic_message_passing_uut)); // thread id is 0 but it doesn't really need it for this test
    std::thread consumer_thread1(Test1ConsumerTh, 0, std::ref(basic_message_passing_uut));
    std::thread consumer_thread2(Test1ConsumerTh, 1, std::ref(basic_message_passing_uut));
    void MonitorTh(uint8_t thread_id, BasicMessagePassing & bmp);

    producer_thread.join();
    consumer_thread1.join();
    consumer_thread2.join();
    */
    

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
    std::thread bad_thread(BadUserThread, std::ref(basic_message_passing_uut));
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    bad_thread.join();


    std::cout << "Test program completed execution." << std::endl;
    return 0;
}


void Test1ProducerTh(BasicMessagePassing& bmp) {
    unsigned int status;
    message_t* msg0 = bmp.new_message();
    message_t* msg1 = bmp.new_message();
    message_t* msg2 = bmp.new_message();

    // write something in the new message
    msg0->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    for (int i = 0; i < msg0->len; i++)  msg0->data[i] = 0x13;
    msg1->len = (int)rand() % (MAX_DATA_LENTH + 1);
    for (int i = 0; i < msg1->len; i++)  msg1->data[i] = 0x13;
    msg2->len = (int)rand() % (MAX_DATA_LENTH + 1);
    for (int i = 0; i < msg2->len; i++)  msg2->data[i] = 0x13;

    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread created 3 messages, to send to threads 1-3 respectively" << std::endl;
    }
    status = bmp.send(0, msg1);
    assert(status == BasicMessagePassing::SUCCESS);
    sem_th0_count_packets.release(); // Signal thread 0 that a message is added to its queue

    status = bmp.send(1, msg1);
    assert(status == BasicMessagePassing::SUCCESS);
    sem_th1_count_packets.release(); // Signal thread 1 that a message is added to its queue
    

    status = bmp.send(2, msg2);  // send a message to a non-existing thread for this test
    assert(status == BasicMessagePassing::SUCCESS);
    
    status = bmp.send(0, msg2);
    sem_th0_count_packets.release(); // add 1 to semaphore counting messages sent to thread 1
    assert(status == BasicMessagePassing::SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // Producer thread will sleep for a second, allowing consumers to receive messages
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Producer thread slept and woke up, will delete the 3  messages" << std::endl;
    }
    bmp.delete_message(msg0);
    bmp.delete_message(msg1);
    bmp.delete_message(msg2);
}
void Test1ConsumerTh(uint8_t thread_id, BasicMessagePassing& bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << " Consumer thread ID: " << (unsigned int) thread_id << " will wait to acquire the semaphore of its oqueue, and display the length of any received message" << std::endl;
    }
    message_t* msg_recevied = NULL;
    while (1) {
        switch (thread_id) {
        case 0:
            sem_th0_count_packets.acquire();
            break;
        case 1:
            sem_th1_count_packets.acquire();
        }
        bmp.recv(thread_id, msg_recevied);
        {
            assert(msg_recevied != NULL);
            std::lock_guard<std::mutex> lg_cout(m_cout);
            std::cout << "  Consumer thread ID: " << (unsigned int)thread_id << " received msg of length: " << (unsigned int)msg_recevied->len << std::endl;
        }
    }
}
//void Test1ReceiverTh(uint8_t thread_id, BasicMessagePassing& bmp) {
//
//}

void BadUserThread(BasicMessagePassing& bmp) {
    unsigned int status;
    message_t* msg = NULL;
    
    std::lock_guard<std::mutex> lg_cout(m_cout);    // to be kept for the duration of the bad user test
    std::cout << "Test Bad User - Thread will test error conditions, use of impropper function parameters and overflow queues" << std::endl;

    std::cout << "              - delete(NULL) - should display err message" << std::endl;
    bmp.delete_message(NULL);
    std::cout << "              - delete((message_t*)&status) - Skipped, Known Fails" << std::endl;
    //bmp.delete_message((message_t*)&status); //  TODO -- validate a malicious addresses that doesn't point to like this 

    std::cout << "              - send(-1, NULL) - should return INVALID_DESTINATION_ID" << std::endl;
    status = bmp.send(-1, NULL);
    assert(status == BasicMessagePassing::INVALID_DESTINATION_ID);
    std::cout << "              - send(2542, NULL) - should return INVALID_DESTINATION_ID" << std::endl;
    status = bmp.send(2542, NULL);
    assert(status == BasicMessagePassing::INVALID_DESTINATION_ID);
    std::cout << "              - send(2, NULL) - should return INVALID_MSG_ADDRESS" << std::endl;
    status = bmp.send(2, NULL);
    assert(status == BasicMessagePassing::INVALID_MSG_ADDRESS);
    std::cout << "              - recv(-2, NULL) - should return INVALID_RECEIVER_ID" << std::endl;
    status = bmp.recv(-2, NULL);
    assert(status == BasicMessagePassing::INVALID_RECEIVER_ID);
    std::cout << "              - recv(0, NULL) - should return THREAD_QUEUE_EMPTY" << std::endl;
    status = bmp.recv(0, NULL);
    assert(status == BasicMessagePassing::THREAD_QUEUE_EMPTY);
    std::cout << "              - recv(9876, NULL) - should return INVALID_RECEIVER_ID" << std::endl;
    status = bmp.recv(9876, NULL);
    assert(status == BasicMessagePassing::INVALID_RECEIVER_ID);
    std::cout << "              - recv(0, msg) - should return THREAD_QUEUE_EMPTY" << std::endl;
    status = bmp.recv(0, msg);
    assert(status == BasicMessagePassing::THREAD_QUEUE_EMPTY);
    
    return;
}

void ProducerTh(uint8_t thread_id, BasicMessagePassing& bmp, unsigned int max_thread_count) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[+] Producer thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    }

    /*  for (unsigned int i = 0; i < max_thread_count; i++) {
        if (i == thread_id) continue; // don't send to self -- for now at least
        bmp.send(i, bmp.new_message());
    }
  */

}
void ConsumerTh(uint8_t thread_id, BasicMessagePassing& bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[-] Consumer thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    }
}
void MonitorTh(uint8_t thread_id, BasicMessagePassing& bmp) {
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[o] Monitor thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    }
}

