#pragma once
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex> 

 
#define MAX_THREADS_POSSIBLE 32				// Assuming this library is designed for an embedded system with a limited number of hardware_concurrency support 
#define MAX_DATA_LENTH 255



typedef struct message_t {
	uint8_t len;
	uint8_t data[MAX_DATA_LENTH];
	struct message_t* next;
};



class BasicMessagePassing{
public:
	enum erorrIds {
		SUCCESS = 0,
		INVALID_MSG_ADDRESS,
		INVALID_DESTINATION_ID,
		INVALID_RECEIVER_ID,
		ERROR_ALLOCATING_DYN_MEM,
		THREAD_QUEUE_EMPTY
	};

	/*
	*	BasicMessagePassing()
	*		Constructor: Initialize the class linked list pointers 
	*/
	BasicMessagePassing();

	/*
	*	~BasicMessagePassing()
	*		Destructor:  Delet all dynamically created objects
	*/
	~BasicMessagePassing();

	/*
	* 	new_message() 
	*		creates a new message_t object in Heap
	*   Input: 
	*		None
	*   Retrun:
	* 		address of newly created object in heap if creation was successful
	* 		or NULL when error is encountered
	*/
	message_t* new_message();
	
	/*	
	*	void delete_message(message_t* msg) 
	*		deletes a message object and frees its memory back to the Hap
	*	Input:
	*		message_t*	msg: pointer to the msg to be deleted
	*	Return:
	*		None
	*	Assumptions: 
	*		It's assumed that receiving a message does not destroy it.
	*		This is the only method for the user to delete messages created using new_message
	*		When a message is deleted, all send message requests for that message, that have not been received
	*		are also to be deleted
	*	Known Issues:
	*		The function will fail if the pointer does not point to a valid msg object.
	*/
	void delete_message(message_t* msg);

	/*
	*	int send(uint8_t destination_id, message_t* msg)
	*		Create a send message object with a pointer to the desired message, add it to destination ID FIFO
	*	Input: 
	*		uint8_t		destination_id	
	*		message_t *	msg
	*	Return: 
	*		0 on success
	*		Error code otherwise	{INVALID_DESTINATION_ID, INVALID_MSG_ADDRESS, ERROR_ALLOCATING_DYN_MEM}
	*	Assumptions:
	*		The destination ID has to be within the acceptable range [0 - MAX_THREADS_POSSIBLE],
	*		it's OK to send a message to a destination ID for a thread that doesn't exist yet,
	*		The same message object can be sent to multiple destination threads
	*/
	int send(uint8_t destination_id, message_t* msg);

	/*
	*	int recv(uint8_t receiver_id, message_t* msg);
	*		Create a send message object with a pointer to the desired message
	*	Input:
	*		uint8_t		receiver_id
	*		message_t*&	msg			: Pointer passed by reference to return the address of receuved message to calling thread)
	*	Return:
	*		0 on success			{INVALID_DESTINATION_ID,or  ERROR_ALLOCATING_DYN_MEM, THREAD_QUEUE_EMPTY}
	*		Error code otherwise (non-zero)
	*	Assumptions:
	*		When a message is received, the wrapper_send object in the thread_id fifo is deleted, but
	*		the message it points to is not deleted.
	*/
	int recv(uint8_t receiver_id, message_t*& msg);

private:
	// Linked List wrapper object for Send commands.
	typedef struct  message_wrapper {
		message_t* msg;
		uint8_t dst;
		struct message_wrapper* next;
	};
	

	// Linked List of all created messages, used for deleting all created messages in destructor
	message_t* created_msgs_head;
	message_t* created_msgs_tail;

	// Linked List Fifo Queues for all possible thread_ids in the rang [0-MAX_THREADS_POSSIBLE]
	message_wrapper* queues_head[MAX_THREADS_POSSIBLE];
	message_wrapper* queues_tail[MAX_THREADS_POSSIBLE];

	// Synchronization mutexes 
	std::mutex m_msgs;
	std::mutex m_queue[MAX_THREADS_POSSIBLE];
};
