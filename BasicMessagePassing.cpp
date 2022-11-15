#include "BasicMessagePassing.h"

// init all data members
BasicMessagePassing::BasicMessagePassing() {
	created_msgs_head = NULL;
	created_msgs_tail = NULL;

	for (int i = 0; i < MAX_THREADS_POSSIBLE; i++) {
		queues_head[i] = NULL;
		queues_tail[i] = NULL;
	}
}

// delete all items in all linked lists
BasicMessagePassing::~BasicMessagePassing() {
	message_t* nxt_msg;
	message_t* at_msg = created_msgs_head;
	while (at_msg!=NULL /*&& at_msg != created_msgs_tail*/) {
		nxt_msg = at_msg->next;
		delete at_msg;
		at_msg = nxt_msg;
	}

	message_wrapper* at_wrapper;
	message_wrapper* nxt_wrapper;

	for (int i = 0; i < MAX_THREADS_POSSIBLE; i++){
		at_wrapper = queues_head[i];
		while (at_wrapper != NULL /*&& at_wrapper != queues_tail[i]*/) {
			nxt_wrapper = at_wrapper->next;
			delete at_wrapper;
			at_wrapper = nxt_wrapper;
		}
	}
}

/*
  Steps to creating a new message:
   - Create a new message object
      - if bad_alloc error is caught, print err message and return NULL
   - intialize the data in the newly created message: 
      - len <= 0
	  - data[MAX_DATA_LENTH] <= 00..
   - add it to the created msgs linked list, will be used to keep track of all created messsages for the destructor to clear them
   - return the new message object address in memory
 */
message_t* BasicMessagePassing::new_message() {
	message_t* new_msg;
	try {
		new_msg = new message_t;
		int x = 0;
	}
	catch (const std::bad_alloc& e) {
		std::cout << "!!ERR!! error while creating message\n";
		std::cout << e.what() << std::endl;
		return NULL;
	}


	//  - clear new message content
	new_msg->len = 0;
	for (int i = 0; i < MAX_DATA_LENTH; i++) {
		new_msg->data[i] = 0;
	}
	new_msg->next = NULL;

	//  - add it to the linked list
	{
		std::lock_guard<std::mutex> lg_msgs(m_msgs);

		if (created_msgs_head == NULL) {		// Linked list of created messages is empty
			created_msgs_head = new_msg;
			created_msgs_tail = new_msg;
		}
		else {
			created_msgs_tail->next = new_msg;
			created_msgs_tail = new_msg;
		}
	}

	return new_msg;
}

/*
  Steps to deleting a message:
   - Find all wrapper messages in the MAX_THREADS_POSSIBLE queues pointing to the msg and delete the wrappers from the Linekd List
   - Find the msg in the linked list of created message objects, and delete it from the Linked List
*/
void BasicMessagePassing::delete_message(message_t* msg) {
	if (msg == NULL) {
		std::cout << "!!ERR!! BasicMessagePassing::delete_message received invalid msg address == NULL\n";
		return;
	}

	// Search for the msg in all queues, delete it from all linked lists, 
	message_wrapper* at_wrapper;
	message_wrapper* prev_wrapper = NULL;
	message_wrapper* to_delete_wrapper = NULL;
	for (int i = 0; i < MAX_THREADS_POSSIBLE; i++) {
		std::lock_guard<std::mutex> lg_queue(m_queue[i]);
		at_wrapper = queues_head[i];
		prev_wrapper = NULL;
		to_delete_wrapper = NULL;
		while (at_wrapper!= NULL /*&& at_wrapper != queues_tail[i]*/) {
			if (at_wrapper->msg == msg) {
				to_delete_wrapper = at_wrapper;
				if (prev_wrapper == NULL) {	// delete head node, and continue searching for other wrappers pointing to the same message
					queues_head[i] = at_wrapper->next;
					if (queues_head[i] == NULL) queues_tail[i] = NULL;
				}
				else if (at_wrapper->next == NULL) { // delete tail node
					queues_tail[i] = prev_wrapper;
					prev_wrapper->next = NULL;
				}
				else { // delete a middle node
					prev_wrapper->next = at_wrapper->next;
				}
				at_wrapper = at_wrapper->next;
				delete to_delete_wrapper;
			}
			else {
				prev_wrapper = at_wrapper;
				at_wrapper = at_wrapper->next;
			}
		}
	}
	// then delete the msg object from its linked list
	message_t* at_msg; 
	message_t* prev_msg = NULL;
	{
		std::lock_guard<std::mutex> lg_msgs(m_msgs);
		at_msg = created_msgs_head;
		while (at_msg != NULL){//created_msgs_tail) {
			if (at_msg == msg) { // There will only be one object in this linked list 
				if (prev_msg == NULL) { // delete the first element of the linked list
					created_msgs_head = at_msg->next;
					//if(created_msgs_head )
				}
				else if (at_msg->next == NULL) { // delete the last element of the linked list
					created_msgs_tail = prev_msg;
					prev_msg->next = NULL;
				}
				else	// delete a middle node in the linked list,
					prev_msg->next = at_msg->next;
				break;

			}
			prev_msg = at_msg;
			at_msg = at_msg->next;
		}
	}
	delete msg;
}

/*
  Steps to send a message:
 - Validate inputs
 - Creates a new message wrapper in the queue for thread_id,
     init new wrapper members
 - Update the corresponding linked list 
*/
int BasicMessagePassing::send(uint8_t destination_id, message_t* msg) {
	if (destination_id < 0 || destination_id >= MAX_THREADS_POSSIBLE) {
		std::cout << "!!ERR!! BasicMessagePassing::send received invalid destination id value: " << (int)destination_id << '\n';
		std::cout << "        valid values of destination id : [0 - " << MAX_THREADS_POSSIBLE << " - 1]\n";
		return INVALID_DESTINATION_ID;
	}

	if (msg == NULL) {
		std::cout << "!!ERR!! BasicMessagePassing::send received invalid msg address == NULL\n";
		return INVALID_MSG_ADDRESS;
	}

	message_wrapper* new_wrapper;
	try {
		new_wrapper = new message_wrapper;
	}
	catch (const std::bad_alloc& e) {
		std::cout << "!!ERR!! BasicMessagePassing::send while creating a wrapper for a message\n";
		std::cout << e.what() << std::endl;
		return ERROR_ALLOCATING_DYN_MEM;
	}
	new_wrapper->dst = destination_id;
	new_wrapper->msg = msg;
	new_wrapper->next = NULL;

	{
		std::lock_guard<std::mutex> lg_queue(m_queue[destination_id]);
		if (queues_head[destination_id] == NULL) { // first message in an empty queue
			queues_head[destination_id] = new_wrapper;
			queues_tail[destination_id] = new_wrapper;  // add to empty linked list 
		}
		else {			// add to tail of queue
			queues_tail[destination_id]->next = new_wrapper;
			//new_wrapper->prev = queues_tail[destination_id];
			queues_tail[destination_id] = new_wrapper;
		}
	}

	return SUCCESS;
}

/*
  Steps to receive a message:
 - Validate inputs
 - verify a message exists in the thread_id queue,
	then copy the address of its message for return in input pointer (passed by reference)
 - delete wrapper object, free the memory assigned in the send function
*/
int BasicMessagePassing::recv(uint8_t receiver_id, message_t*& msg) {
	if (receiver_id < 0 || receiver_id >= MAX_THREADS_POSSIBLE) {
		std::cout << "!!ERR!! BasicMessagePassing::recv received invalid receiver_id id value: " << (int)receiver_id << std::endl;
		std::cout << "        valid values of destination id : [0 - " << MAX_THREADS_POSSIBLE << " - 1]\n";
		return INVALID_RECEIVER_ID;
	}

	message_wrapper* to_del;
	{
		std::lock_guard<std::mutex> lg_queue(m_queue[receiver_id]);
		if (queues_head[receiver_id] == NULL) { // There's no message for this received, 
			std::cout << "!!ERR!! BasicMessagePassing::recv attempted to read from an empty queue for received_id: " << (int)receiver_id << std::endl;
			return THREAD_QUEUE_EMPTY;
		}
		
		to_del = queues_head[receiver_id];
		queues_head[receiver_id] = queues_head[receiver_id]->next;
		if (queues_head[receiver_id] == NULL) {
			queues_tail[receiver_id] = NULL;
		}
	}

	msg = to_del->msg;
	delete to_del;

	return SUCCESS;
}