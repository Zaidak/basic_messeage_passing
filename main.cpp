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


Design approach: 
- Given the assumptions below, a singly linked list datastructure is used to build simple fifo queues for each possible thread.
- This gives very efficient receive function implementation, but adds complexity to the delete_message

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
- Race conditions in the test app implementation are acceptable, its goal is to test the MessagePassingLibrary
Possible improvements: 
    - Use a thread pool for the test application instead of creating and deleting test threads.
    - Syncrhonize access to std::cout using std::basic_osyncstream in the test app, instead of the global mutex & lock guards
    - Use Joinable threads in the test app to avoid
    - Lock guard scopes in the library likely posssible to be optimized, made smaller, seperate mutexes for a queue pointers can be used - espicially the delete_message function
    - Correct bug in semaphore signaling between producer and consumer threads in the test app
Notes:
    - Test app uses asserts to verify the library is executing as expected. Error prompts in the console output are expected.
    - When test cases expected values fail, an assert function call will interrupt the test run
*/

#include <iostream>
#include <thread>
#include <mutex> 
#include <semaphore>
#include <cassert>

#include "BasicMessagePassing.h"


std::mutex m_cout;

std::counting_semaphore sem_th0_count_packets{ 0 };                                            // to count packets queued for thread_id 0
std::counting_semaphore sem_th1_count_packets{ 0 };                                            // to count packets queued for thread_id 1

std::atomic_flag cont_receiving;

// Test 1: simple test of library functions using 3 threads
void Test1ProducerTh(BasicMessagePassing* bmp); // creates messgae objects, sends to 2 Consumer threads. Doesn't need to receive messages; doesn't need an ID
void Test1ConsumerTh0(BasicMessagePassing* bmp); // waits for available messages passed to it, deletes received messages after using them
void Test1ConsumerTh1(BasicMessagePassing* bmp); // waits for available messages passed to it, deletes received messages after using them

// Test 2: Bad user test - bas function parameter values & memory allocation exceptions
void BadUserThread(BasicMessagePassing* bmp);

// Test 3: Performance & thread safety - using semaphores, continously sending and receiving messages
void Test3ProducerTh(BasicMessagePassing* bmp);


int main(){
    BasicMessagePassing * p_basic_message_passing_uut;

    std::cout << "Testing Basic Message Passing Library." << std::endl;
    std::cout << "Hardware concurrency supports " << std::thread::hardware_concurrency() << " threads\n";

    std::cout << "Test group 1: a producer thread creates fixed nmber of messages and sends them in a deterministic way to 2 consumer threads" << std::endl;
    p_basic_message_passing_uut = new BasicMessagePassing();
    cont_receiving.test_and_set(); // enable consumers, will be desabled at the end of Test1ProducerTh
    std::thread producer_thread(Test1ProducerTh, p_basic_message_passing_uut);
    std::thread consumer_thread0(Test1ConsumerTh0, p_basic_message_passing_uut);
    std::thread consumer_thread1(Test1ConsumerTh1, p_basic_message_passing_uut);
    producer_thread.join();
    consumer_thread0.join();
    consumer_thread1.join();
    delete p_basic_message_passing_uut;


    std::cout << "Test group 2: Bad user thread will attempt to use invalid library API input values" << std::endl;
    p_basic_message_passing_uut = new BasicMessagePassing();
    std::thread bad_thread(BadUserThread, std::ref(p_basic_message_passing_uut));
    bad_thread.join();
    delete p_basic_message_passing_uut;


    std::cout << "Test group 3: 3 producer threads continuously sending messages to the 2 consumer threads from test 1" << std::endl ;
    p_basic_message_passing_uut = new BasicMessagePassing();
    std::thread cont_producers[3];
    for(int i=0; i<3 ;i++){
        cont_producers[i] = std::thread(Test3ProducerTh, p_basic_message_passing_uut);
    }
    std::thread cont_consumer0(Test1ConsumerTh0, p_basic_message_passing_uut);
    std::thread cont_consumer1(Test1ConsumerTh1, p_basic_message_passing_uut);
    for (int i = 0; i < 3; i++) {
        cont_producers[i].join();
    }
    cont_consumer0.join();
    cont_consumer1.join();


    std::cout << "Test program completed execution sucessfully." << std::endl;
    std::cout << "Unless a failed assertion is encountered, the test results matched expectations." << std::endl;
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
        std::cout << " Producer thread sent msg 2 to thread 2 times, 3 sends of msg 2 are now in queue." << std::endl;
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
        std::cout << " Producer thread will sleep for 5 seconds, then delete the remaining messages and exit" << std::endl;
    }
    cont_receiving.clear();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // To provide a break in the console output for manual testing
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
    while (1){
        if (cont_receiving.test()) break;
        sem_th0_count_packets.acquire();
        status = bmp->recv(0, msg_recevied);
        if (status == BasicMessagePassing::SUCCESS) {
            assert(msg_recevied != NULL);
            std::lock_guard<std::mutex> lg_cout(m_cout);
            std::cout << "  Consumer thread 0 received msg of length: " << (unsigned int)msg_recevied->len << std::endl;
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
    while (1){
        if (cont_receiving.test()) break;
        sem_th1_count_packets.acquire();
        status = bmp->recv(1, msg_recevied);
        if (status == BasicMessagePassing::SUCCESS) {
            assert(msg_recevied != NULL);
            std::lock_guard<std::mutex> lg_cout(m_cout);
            std::cout << "  Consumer thread 1 received msg of length: " << (unsigned int)msg_recevied->len << std::endl;
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
    std::cout << "              - Sleep for 5 seconds before next test stage" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(4000)); // To provide a break in the console output for manual testing

    return;
}
#define SEND_BURST_COUNT 1000
void Test3ProducerTh(BasicMessagePassing* bmp){
    {
        std::lock_guard<std::mutex> lg_cout(m_cout);
        std::cout << "[+] Producer created. Will continuously send messages to 2 consumer threads." << std::endl;
    }

    message_t* msg0 = bmp->new_message();
    message_t* msg1 = bmp->new_message();
    msg0->len = 1;
    msg1->len = 5;
    for(int i=0;i<SEND_BURST_COUNT; i++){
        bmp->send(0, msg0);
        sem_th0_count_packets.release();
        bmp->send(1, msg0);
        sem_th1_count_packets.release();
        bmp->send(0, msg1);
        sem_th0_count_packets.release();
        bmp->send(1, msg1);
        sem_th1_count_packets.release();
    }
    cont_receiving.clear();

}
