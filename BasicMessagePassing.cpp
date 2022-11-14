#include "BasicMessagePassing.h"

// init all data members
BasicMessagePassing::BasicMessagePassing() {
	//m_msgs.lock();			// TODO delete
	created_msgs_head = NULL;
	created_msgs_tail = NULL;
	//m_msgs.unlock();

	for (int i = 0; i < MAX_THREADS_POSSIBLE; i++) {
		queues_head[i] = queues_tail[i] = NULL;
	}
}

// delete all items in all doubly linked lists
BasicMessagePassing::~BasicMessagePassing() {
	message_t *nxt_msg;
	//m_msgs.lock();		// TODO delete
	message_t *at_msg= created_msgs_head;
	while (at_msg != created_msgs_tail) {
		nxt_msg = at_msg->next;
		delete at_msg;
		at_msg = nxt_msg;
	}
	//m_msgs.unlock();

	message_wrapper* at_wrapper;
	message_wrapper* nxt_wrapper;
	for (int i = 0; i < MAX_THREADS_POSSIBLE; i++){
		//m_queue[i].lock();
		at_wrapper = queues_head[i];
		while (at_wrapper != queues_tail[i]) {
			nxt_wrapper = at_wrapper->next;
			delete at_wrapper;
			at_wrapper = nxt_wrapper;
		}
		//m_queue[i].unlock();
	}
}

/*
  Steps to creating a new message:
   - Create a new message struct object
      - if bad_alloc error is caught, print err message and return NULL
   - intialize the data in the newly created message: 
      - len <= 0
	  - data[MAX_DATA_LENTH] <= 00..
	  - prev & next pointers <= NULL
   - add it to the created_msgs_doubly_linked_list -- will be used to keep track of all created messsages for the destructor to clear them
   - return the new message object address in memory
 */
message_t* BasicMessagePassing::new_message() {
	message_t* new_msg;
	try {
		new_msg = new message_t;
	}
	catch (const std::bad_alloc& e) {
		std::cout << "!!ERR!! error while creating message\n";
		std::cout << e.what() << std::endl;
		return NULL;
	}
	catch (...) {
		std::cout << "!!!ERR!!! other error while creating a message" << std::endl;
		return NULL;
	}

	//  - clear new message content
	new_msg->len = 0;
	for (int i = 0; i < MAX_DATA_LENTH; i++) {
		new_msg->data[i] = 0x00;
	}
	new_msg->next = NULL;
	new_msg->prev = NULL;

	//  - add it to the linked list
	m_msgs.lock();
	if (created_msgs_head == NULL) {
		created_msgs_head = created_msgs_tail = new_msg;
	}
	else {
		created_msgs_tail->next = new_msg;
		new_msg->prev = created_msgs_tail;
		created_msgs_tail = new_msg;
	}
	m_msgs.unlock();

	return new_msg;
}

/*
  Steps to deleting a message:
   - Find all wrapper messages in the MAX_THREADS_POSSIBLE queues pointing to msg and delete the wrappers
   - Find it in the doubly linked list of created message objects
*/
void BasicMessagePassing::delete_message(message_t* msg) {
	if (msg == NULL) {
		std::cout << "!!ERR!! BasicMessagePassing::delete_message received invalid msg address == NULL\n";
		return;
	}

	// Search for the msg in all queues, delete it from all linked lists, 
	// then delete the object
	message_t* at_msg; 

	m_msgs.lock();
	at_msg = created_msgs_head;
	while (at_msg != created_msgs_tail) {
		if ( at_msg == msg ) {
			if (at_msg->prev != NULL) {	// something behind us? then update it
				at_msg->prev->next = at_msg->next;
			}
			else {						// else we were the head!
				created_msgs_head = at_msg->next;
			}
			if (at_msg->next != NULL) { // something ahead? then update it
				at_msg->next->prev = at_msg->prev;
			}
			else {						// else we were tail!
				created_msgs_tail = at_msg->prev;
			}
		}
		at_msg = at_msg->next;
	}
	m_msgs.unlock();

	try {
		delete msg;
	}
	catch (...) {
		std::cout << "!!ERR!! error while deleting a message at address: " << (void*)msg << std::endl;
	}
}

/*
 - Validate inputs
 - Creates a new message wrapper in the queue for thread_id,
     init new wrapper dst and msg
 - Update the corresponding doubly linked list 
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

	m_queue[destination_id].lock();
	if (queues_head[destination_id] == NULL) { // first message in an empty queue
		queues_head[destination_id] = queues_tail[destination_id] = new_wrapper;  // add to empty linked list 
	}
	else {			// add to tail of queue
		queues_tail[destination_id]->next = new_wrapper;
		new_wrapper->prev = queues_tail[destination_id];
		queues_tail[destination_id] = new_wrapper;
	}
	m_queue[destination_id].unlock();

	return SUCCESS;
}

// consume a message wrapper object from the head of the receiver_id queue
// receiving a message doesn't delete the message it just deletes a wrapper that points to it
// return error code, or 0 if success.
int BasicMessagePassing::recv(uint8_t receiver_id, message_t* msg) {
	if (receiver_id < 0 || receiver_id >= MAX_THREADS_POSSIBLE) {
		std::cout << "!!ERR!! BasicMessagePassing::recv received invalid receiver_id id value: " << (int)receiver_id << '\n';
		std::cout << "        valid values of destination id : [0 - " << MAX_THREADS_POSSIBLE << " - 1]\n";
		return INVALID_RECEIVER_ID;
	}
	if (msg == NULL) {
		std::cout << "!!ERR!! BasicMessagePassing::recv received invalid msg address == NULL\n";
		return INVALID_MSG_ADDRESS;
	}

	m_queue[receiver_id].lock();
	if (queues_head[receiver_id] == NULL) { // There's no message for this received, 

	}
	message_wrapper* to_del = queues_head[receiver_id];
	msg = queues_head[receiver_id]->msg;
	queues_head[receiver_id] = queues_head[receiver_id]->next;
	m_queue[receiver_id].unlock();
	delete to_del;

	return SUCCESS;
}