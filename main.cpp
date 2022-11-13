/*


Compiler: 

Assumptions: 
- Data length valid range: [0-MAX_DATA_LENTH] inclusive. I.e. 0 length messages are legal.
- Any thread is allowed to read a message from the 
- This library is to be used on a device with maximum 256 HW concurent threads 
- NON-BLCOKIUNG MESSAGE PASSING
Test edge cases, 
producers continue to generate messages beyong legal limits


*/


#include <iostream>

#include <thread>
#include <mutex> 

#include <algorithm>
#include <vector>

#include "BasicMessagePassing.h"

#define MSGS_PER_PRODUCER 100


std::mutex m_cout;  // TODO, dont use global object?

/*

To test the BasicMessagePassing new_message, send, and receive, a producer thread will write a message for each other thread,
/// == dont send to self for now .. and listen for messages destined to it (it will send itsenf a messge too)

*/
void ProducerTh(uint8_t thread_id,  BasicMessagePassing& bmp, unsigned int max_thread_count) {
    std::lock_guard<std::mutex> lock(m_cout);
    std::cout << "[+] Producer thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
    //message_t *msg = bmp.new_message();
    //message_t *msg2 = bmp.new_message();
    //message_t *msg3 = bmp.new_message();

    //// write something in the new message
    //msg->len = (int)rand() % (MAX_DATA_LENTH+1); // legal length is 0 - MAX_DATA_LENTH. 
    //for (int i = 0; i < msg->len; i++) {
    //    msg->data[i] = 0x13;		// init the data with repeated value
    //}
    //msg2->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    //for (int i = 0; i < msg2->len; i++) {
    //    msg2->data[i] = 0x13;		// init the data with repeated value
    //}
    //msg3->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    //for (int i = 0; i < msg3->len; i++) {
    //    msg3->data[i] = 0x13;		// init the data with repeated value
    //}
    //
    //bmp.send(1, msg);
    //bmp.send(2, msg2);
    //bmp.send(3, msg3);


  /*  for (unsigned int i = 0; i < max_thread_count; i++) {
        if (i == thread_id) continue; // don't send to self -- for now at least
        bmp.send(i, bmp.new_message());
    }
  */

}


void ConsumerTh(uint8_t thread_id, BasicMessagePassing& bmp) {
    std::lock_guard<std::mutex> lock(m_cout);
    std::cout << "[-] Consumer thread " << (unsigned int) thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;

}

void MonitorTh(uint8_t thread_id, BasicMessagePassing& bmp) {
    std::lock_guard<std::mutex> lock(m_cout);
    std::cout << "[o] Monitor thread " << (unsigned int)thread_id << " created. Thread ID: " << std::this_thread::get_id() << std::endl;
}

void ProducerTest1Th(BasicMessagePassing& bmp) {
    message_t* msg1 = bmp.new_message();
    message_t* msg2 = bmp.new_message();
    message_t* msg3 = bmp.new_message();

    // write something in the new message
    msg1->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    for (int i = 0; i < msg1->len; i++) {
        msg1->data[i] = 0x13;		// init the data with repeated value
    }
    msg2->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    for (int i = 0; i < msg2->len; i++) {
        msg2->data[i] = 0x13;		// init the data with repeated value
    }
    msg3->len = (int)rand() % (MAX_DATA_LENTH + 1); // legal length is 0 - MAX_DATA_LENTH. 
    for (int i = 0; i < msg3->len; i++) {
        msg3->data[i] = 0x13;		// init the data with repeated value
    }

    bmp.send(1, msg1);
    bmp.send(2, msg2);
    bmp.send(3, msg3);
    bmp.delete_message(msg1);
    bmp.delete_message(msg2);
    bmp.delete_message(msg3);
}


void BadUserThread(BasicMessagePassing& bmp) {
    bmp.delete_message(NULL);
    //bmp.delete_message(); // TODO -- add malicious addresses? 
    bmp.send(-1, NULL);
    bmp.send(2542, NULL);
    bmp.send(2, NULL);
    bmp.recv(-2, NULL);
    bmp.recv(0, NULL);
    bmp.recv(9876, NULL);
}

int main()
{
    BasicMessagePassing basic_message_passing_uut;
    std::cout << "Testing Basic Message Passing Library.\n";

    // Test with thread_id 0 creating multiple messages and using send and receive for basic functional testing
    std::cout << "Test 1: Thread1 creates several messages and sends them around\n";
    std::thread producer_thread(ProducerTest1Th, std::ref(basic_message_passing_uut)); // thread id is 0 but it doesn't really need it for this test
    std::thread consumer_thread1(ConsumerTh, 1, std::ref(basic_message_passing_uut));
    std::thread consumer_thread2(ConsumerTh, 2, std::ref(basic_message_passing_uut));


    producer_thread.join();
    consumer_thread1.join();
    consumer_thread2.join();



    /*
    Test with 10 producer threads sending data to 8 consumer threads
    */

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
    /*unsigned int*/ created_thread_count = 0;
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

    thread_pool.at(created_thread_count) = std::thread(MonitorTh, created_thread_count, std::ref(basic_message_passing_uut));
    created_thread_count++;

    for (; created_thread_count < max_thread_count; created_thread_count++) {
        thread_pool.at(created_thread_count) = std::thread(ConsumerTh, created_thread_count, std::ref(basic_message_passing_uut)); // TODO: randomize th function
    }


    for (unsigned int i = 0; i < created_thread_count; i++) {
        thread_pool.at(i).join();
    }
    /*
    */






    /*
    Tset with a bad user thread that will use invalid inputs to test library input validation
    */

    std::cout << "Test 3: Bad user thread will attempt to use invalid library API input values\n";
    std::thread bad_thread(BadUserThread, std::ref(basic_message_passing_uut));
    bad_thread.join();


    std::cout << "Test program completed execution." << std::endl;
    return 0;
}
