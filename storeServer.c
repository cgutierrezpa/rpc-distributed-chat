
#include "rpc_store_service/store_service.h"

/* Define the structure of the message list nodes */
struct msg{
    char body[MAX_SIZE];		/* Content of the message */
    char md5[MAX_MD5];			/* MD5 of the message */
    char sender[MAX_SIZE];		/* Sender of the message */
    char receiver[MAX_SIZE];	/* Receiver of the message */
    unsigned int id;			/* ID assigned to the message */
    struct msg *next;			/* Pointer to the next message in the list */
};
/* Define the structure of the user list nodes */
struct user{
	char name[MAX_SIZE];		/* Name of the user */
	unsigned long num_msgs;		/* Number of sent (stored) messages for the user */
	struct user *next;			/* Pointer to the next user in the list */
	struct msg *sent_msgs_head;	/* Pointer to the head of the messages sent by the user */
};

/* ========================================================== */
/* ======================== HEADERS ========================= */
/* ========================================================== */

struct user * usr_head;

int addMsg(struct msg **head,char * message, char * md5, unsigned int id, char * receiver);

/* Initializes the user list in the server. If there is an existing user list in memory, this is 
traversed and all the nodes in the list (including both messages and users) will be freed from
memory.
	Returns always TRUE. No internal error can happen */
bool_t
init_1_svc(void *result, struct svc_req *rqstp)
{
	bool_t retval = TRUE;
	/* If the list of users is not empty, traverse the list and free each node */
	if(usr_head != NULL){
		struct user *prev = usr_head;
		/* While the list is greater than 1, advance in the list and eliminate the first node of the list */
		while(usr_head->next != NULL){
			/* If the list of messages associated to the user is not empty, traverse it and free the memory */
			if(usr_head->sent_msgs_head != NULL){
				struct msg *prev_msg = usr_head->sent_msgs_head;
				/* While the list is greater than 1, advance in the list and eliminate the first node */
				while(usr_head->sent_msgs_head->next != NULL){
					usr_head->sent_msgs_head = usr_head->sent_msgs_head->next;
					free(prev_msg);
					prev_msg = usr_head->sent_msgs_head;
				}
				/* Free the resources of the last element in the list */
				free(prev_msg);
			}
			usr_head = usr_head->next;
			free(prev);
			prev = usr_head;
		}
		/* Free the resources of the last element in the list */
		free(prev);
	}
	/* Initialize the list of users to NULL */
	usr_head = NULL;

	return retval;
}

/* Stores the message and the associated information (receiver, ID, MD5) into the list of messages sent by 
the input user passed in the 'sender'.
	Returns TRUE no errors
	Returns FALSE if there is a malloc error (memory full) */
bool_t
store_1_svc(char *sender, char *receiver, u_int msg_id, char *msg, char *md5, int *result,  struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	struct user *temp = usr_head;
	/* Iterate through the list of users that sent at least one message */
	while(temp != NULL){
		if(strcmp(temp->name, sender) == 0){	//User found in the list
			/* Append the message to the list of sent messages by that user */
			*result = addMsg(&(temp->sent_msgs_head), msg, md5, msg_id, receiver);
			/* If -1 is returned, the memory is full and message could not be stored. Return FALSE */
			if(*result == -1) return FALSE;
			/* Update the message counter */
			temp->num_msgs = temp->num_msgs + 1;
			return retval;
		}
		temp = temp->next;
	}
	/* If the code reaches this point, no user was found, so add it to the list and set 
	the message counter to 1 */
	temp = (struct user *) malloc(sizeof(struct user));
	/* If malloc returns error (full memory or other). Return FALSE */
	if(temp == NULL) return FALSE;
	strcpy(temp->name, sender);
	temp->next = NULL;
	temp->sent_msgs_head = NULL;
	/* Add message to the list of messages that the user sent */
	*result = addMsg(&(temp->sent_msgs_head), msg, md5, msg_id, receiver);
	/* If -1 is returned, the memory is full and message could not be stored. Return FALSE */
	if(*result == -1) return FALSE;
	temp->num_msgs = 1;		/* Set the sent-message counter to 1 */

	temp->next = usr_head;
	usr_head = temp;

	return retval;
}

/* Gets the number of messages sent by the input user.
	Returns always TRUE. No internal error can happen */
bool_t
getnummessages_1_svc(char *user, int *result,  struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	struct user *temp = usr_head;
	*result = 0;
	/* Traverse the list of users until the input username is found */
	while(temp != NULL){
		if(strcmp(temp->name, user) == 0){	//Sender is found in the list
			/* Return the number of stored messages for that user */
			*result = temp->num_msgs;
			return retval;
		}
		temp = temp->next;
	}
	*result = -1;	//User was not found

	return retval;
}

/* Gets the message corresponding to the ID and username of the sender of such message.
If the message or the user is not found, then an empty string will be sent back.
	Returns always TRUE. No internal error can happen */
bool_t
getmessage_1_svc(char *user, u_int msg_id, response *result,  struct svc_req *rqstp)
{
	bool_t retval = TRUE;

	/* Initialize to zeroes the message and MD5 strings of the response struct */
	result->msg = calloc(MAX_SIZE, sizeof(char));
	result->md5 = calloc(MAX_MD5, sizeof(char));

	struct user *temp = usr_head;
	struct msg *msg_temp; 

	/* Traverse the list of users looking for the input username */
	while(temp != NULL){
		if(strcmp(temp->name, user) == 0){	//Sender is found in the list
			/* Search for the message with that ID */
			msg_temp = temp->sent_msgs_head;
			/* Iterate through the list of sent messages */
			while(msg_temp != NULL){
				if(msg_temp->id == msg_id){	//Message ID found
					strncpy(result->msg, msg_temp->body, strlen(msg_temp->body)+1);
					strncpy(result->md5, msg_temp->md5, strlen(msg_temp->md5)+1);
					return retval;
				}
				msg_temp = msg_temp->next;
			}
			/* At this point, no message with such ID was found for that user. Stop iterating */
			return retval;
		}
		temp = temp->next;
	}
	/* User was not found, thus message does not exist */

	return retval;
}

int
store_service_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	return 1;
}

/* Creates a new message struct and adds it to the message queue
    Returns 0 if the message is stored
        -1 if malloc error */
int addMsg(struct msg **head, char * message, char * md5, unsigned int id, char * receiver){
    struct msg *temp;
    /* Allocate the space for the new message */
    temp = (struct msg *) malloc(sizeof(struct msg));
    /* If malloc returns error (full memory or other) */
    if(temp == NULL) return -1;
    strcpy(temp->body, message);
    strcpy(temp->md5, md5);
    strcpy(temp->receiver, receiver);
    temp->id = id;
    temp->next = *head; //If msg_head is null, then the list is empty
    *head = temp;
    
    return 0;
}
