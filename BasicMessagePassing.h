#pragma once
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex> 

 
#define MAX_THREADS_POSSIBLE 32 // 256
#define MAX_DATA_LENTH 255

//typedef struct {
//	uint8_t len;
//	uint8_t data[255]
//} message_t;
typedef struct {
	uint8_t len;
	uint8_t data[255];
} message_t_orig;

typedef struct message_t {
	uint8_t len;
	uint8_t data[MAX_DATA_LENTH];
	struct message_t* next;
	struct message_t* prev;		// doubly linked list to make deletions in send() fast
}message_t;


class BasicMessagePassing{
public:
	enum erorrIds {
		SUCCESS = 0,
		INVALID_MSG_ADDRESS,
		INVALID_DESTINATION_ID,
		INVALID_RECEIVER_ID,
		ERROR_ALLOCATING_DYN_MEM

	};

	BasicMessagePassing();
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
	*	delete_message(message_t* msg) 
	*		deletes a message object and frees its memory back to the Hap
	*	Input:
	*		message_t*	msg: pointer to the msg to be deleted
	*	Return:
	*		None
	*	Assumptions: 
	*		It's assumed that receiving a message does not destroy it.
	*		This is the only method for the user to delete messages created using new_message
	*		When a message is deleted, all send message requests for that message, that have not been received
			are also to be deleted
	*/
	void delete_message(message_t* msg);

	/*
	*	send(uint8_t destination_id, message_t* msg)
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
		/*d) int recv(uint8_t receiver_id, message_t * msg) : Threads may use this
		function to receive any pending incoming messages.The receiver_id is the ID of the
		thread that wishes to receive a message.The msg argument is an output, and the
		implementation of this function should set it to point to a message destined to
		receiver_id if one exists.A response code should be returned where 0 indicates
		success and non - 0 indicates error.*/
	// assumptions: 
	// dont delete the received message

	/*
	*	recv(uint8_t receiver_id, message_t* msg);
	*		Create a send message object with a pointer to the desired message
	*	Input:
	*		uint8_t		receiver_id
	*		message_t*	msg
	*	Return:
	*		0 on success
	*		Error code otherwise (non-zero)
	*	Assumptions:
	*		When a message is received, the wrapper_send object in the thread_id fifo is deleted, but
	*		the message it points to is not deleted.
	*/
	int recv(uint8_t receiver_id, message_t* msg);

private:
	// Doubly Linked List wrapper object for Send commands.
	typedef struct  message_wrapper {
		message_t* msg;
		uint8_t dst;
		struct message_wrapper* next;
		struct message_wrapper* prev;
	}message_wrapper;

	// Doubly Linked List of all created messages, used for deleting all created messages in destructor
	message_t* created_msgs_head;
	message_t* created_msgs_tail;

	// Doubly Linked List Fifo Queues for all possible thread_ids in the rang [0-MAX_THREADS_POSSIBLE]
	message_wrapper* queues_head[MAX_THREADS_POSSIBLE];
	message_wrapper* queues_tail[MAX_THREADS_POSSIBLE];

	// Synchronization mutexes 
	std::mutex m_msgs;
	std::mutex m_queue[MAX_THREADS_POSSIBLE];
};

class BasicMessagePassingLockFree {
public:

	BasicMessagePassingLockFree();
	~BasicMessagePassingLockFree();

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
	*	delete_message(message_t* msg)
	*		deletes a message object and frees its memory back to the Hap
	*	Input:
	*		message_t*	msg: pointer to the msg to be deleted
	*	Return:
	*		None
	*	Assumptions:
	*		It's assumed that receiving a message does not destroy it.
	*		This is the only method for the user to delete messages created using new_message
	*		When a message is deleted, all send message requests for that message, that have not been received
			are also to be deleted
	*/
	void delete_message(message_t* msg);


	/*c) int send(uint8_t destination_id, message_t * msg) : Threads may use this
	function to send a message to another thread.The destination_id is the ID of the
	thread the message should be delivered to, and msg is the message to be delivered. A
	response code should be returned where 0 indicates success and non - 0 indicates an
	error.*/
	// assumptions:
	// it's ok to send a message to a destination ID for a thread that doesn't exist yet, it may in the future.
	// The same message object can be sent to multiple destination threads 
		// to do this with a new wrapper pointing to the same message
	/*
	*	send(uint8_t destination_id, message_t* msg)
	*		Create a send message object with a pointer to the desired message
	*	Input:
	*		uint8_t		destination_id
	*		message_t*	msg
	*	Return:
	*		0 on success
	*		Error code otherwise
	*/
	int send(uint8_t destination_id, message_t* msg);
	/*d) int recv(uint8_t receiver_id, message_t * msg) : Threads may use this
	function to receive any pending incoming messages.The receiver_id is the ID of the
	thread that wishes to receive a message.The msg argument is an output, and the
	implementation of this function should set it to point to a message destined to
	receiver_id if one exists.A response code should be returned where 0 indicates
	success and non - 0 indicates error.*/
	// assumptions: 
	// dont delete the received message

	/*
	*	recv(uint8_t receiver_id, message_t* msg);
	*		Create a send message object with a pointer to the desired message
	*	Input:
	*		uint8_t		receiver_id
	*		message_t*	msg
	*	Return:
	*		0 on success
	*		Error code otherwise (non-zero)
	*/
	int recv(uint8_t receiver_id, message_t* msg);

private:

	typedef struct  message_wrapper {
		message_t* msg;
		uint8_t dst;
		struct message_wrapper* next;
		struct message_wrapper* prev;	// doubly linked list to optimize delete_message
	}message_wrapper;
	message_t* created_msgs_head;	/// doubly linked list of all created messgaes..
	message_t* created_msgs_tail;
	message_wrapper* queues_head[MAX_THREADS_POSSIBLE];	// heads and tails for MAX_THREADS_POSSIBLE (256) potential queues 
	message_wrapper* queues_tail[MAX_THREADS_POSSIBLE];

	// Synchronization members 
	std::mutex m_msgs;
	std::mutex m_queue[MAX_THREADS_POSSIBLE];
};