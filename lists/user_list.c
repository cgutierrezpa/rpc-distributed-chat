#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "msg_list.h"
#include "user_list.h"

/* Checks if the input user is already registered
    Return 0 if is registered
        1 if is not registered */
char isRegistered(char * username){
    struct user *temp;
    temp = user_head;

    /* Iterate over the list */
    while (temp != NULL){
        if (strcmp(temp->username, username) == 0){     //Check if the username exists
            return 1;
        }
        temp = temp->next;
    }

    return 0;
 }
 
/* Registers a user (if not previously registered) and appends it to the end of the s list
    Returns 1 if already registered 
        0 if registered correctly */
char registerUser(char * username){
    /* Check if the user already exists */
    if(isRegistered(username)) return 1;

    /* Prepare new user */ 
    struct user *temp;
    temp = (struct user *) malloc(sizeof(struct user));
    /* If memory is full and malloc is not possible, we return code 2 */
    if(temp == NULL) return 2;
    /* Initialize user values */
    strcpy(temp->username, username);
    temp->status = 0;
    strcpy(temp->ip, "-1");
    temp->port = 0;
    temp->pend_msgs_head = NULL;
    temp->next = NULL;
    temp->last_id = 0;

    if (user_head == NULL){      //If list is empty
        temp->next = user_head;
        user_head = temp;
    }
    else{
        struct user *last = user_head;

        /* Iterate over the list */
        while(last->next != NULL){
            last = last->next;
        }
        last->next = temp;
    }

    return 0;
}

/* Changes the status of a user to 1 (ON) and links an IP and port number to it
    Return 0 if connect OK
        1 if user is not registered
        2 if registered but not connected */
char connectUser(char * username, char * ip, uint16_t port){
    struct user *temp = user_head;

    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            if (temp->status == 1) return 2;        //User already connected       
            /* Change status to 1 ("ON") and update IP and Port */
            temp->status = 1;
            strcpy(temp->ip, ip);
            temp->port = port;
            return 0;
        }
        temp = temp->next;
    }
    /* No user was found, so send code 1 */
    return 1;
}

/* Changes the status of a user to 0 (OFF) and cleans the IP and port number
    Return 0 if disconnect OK;
        1 if user is not registered;
        2 if registered but not connected;
        3 if trying to disconnect from a different IP */
char disconnectUser(char * username, char * used_ip){
    struct user *temp = user_head;

    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            if (temp->status == 0) return 2;        //User already disconnected
            if(strcmp(temp->ip, used_ip) != 0) return 3; //Trying to disconnect from a different IP     
            /* Change status to 0 ("OFF") and delete IP and Port */
            temp->status = 0;
            strcpy(temp->ip, "-1");
            temp->port = 0;
            return 0;
        }
        temp = temp->next;
    }

    return 1;
}
 

/* Unregisters a user, deleting it and its pending messages (if any) from the list
    Returns 1 if the user does not exist.
        0 if the user is deleted correctly */
char unregisterUser(char * username){
    struct user *temp, *prev; //temp is the current user, prev is the previous user in the list
    temp = user_head;

    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            if(temp == user_head){       //If the user is at the user_head of the list
                user_head = temp->next;  //change the user_head to the next element
                /* Delete the pending messages if any */
                deleteAllMsgs(&(temp->pend_msgs_head));
                free(temp);         //Free the resources of the user
                return 0;
            }
            else{                   //User is not at the user_head
                prev->next = temp->next;
                /* Delete the pending messages if any */
                deleteAllMsgs(&(temp->pend_msgs_head));
                /* Free the memory resources of the user structure */
                free(temp);
                return 0;
            }
        }
        else{
            prev = temp;
            temp = temp->next;
        }
    }
    //If we reach this point, no user was found
    return 1;
}

/* Returns 0 if store OK. -1 if server error (malloc error because of full memory) */
int storeMsg(char * username, char* msg, unsigned int msg_id, char * md5, char * sender){
    struct user *temp = user_head;

    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            //Enqueue message
            return enqueueMsg(&(temp->pend_msgs_head), msg, md5, msg_id, sender);    
        }
        temp = temp->next;
    }

    return -1; //User was not found
}

/* Increments the last-sent-message ID associated to the input user
    Return the value of the updated ID */
unsigned int updateLastID(char * username){
    struct user *temp = user_head;

    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            //Increment the ID in 1
            temp->last_id = temp->last_id+1;
            //If it results in 0, then the maximum representable number is overflown
            if(temp->last_id == 0) temp->last_id = 1;

            return temp->last_id;     
        }
        temp = temp->next;
    }

    return 0;
}

/* Checks if the input user is connected 
    Return 0 if the user is disconnected
        1 if the user is connected
        2 if error (user was not found) */
char isConnected(char * username){
    struct user *temp = user_head;
    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            return temp->status;     //Returns 0 if OFF, 1 if ON
        }
        temp = temp->next;
    }
    /* Return error 2 if we reach this point. No user was found */
    return 2;
}

/* Retrieves the IP associated to a user in the list
    Returns a char array with the IP of the user
        NULL if the user was not found */
char * getUserIP(char * username){
    struct user *temp = user_head;
    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            return temp->ip;     //Returns the IP of the user
        }
        temp = temp->next;
    }
    return NULL;
}

/* Retrieves the port number associated to a user in the list
    Return the port number
        0 if the user was not found */
uint16_t getUserPort(char * username){
    struct user *temp = user_head;
    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            return temp->port;     //Returns the IP of the user
        }
        temp = temp->next;
    }
    return 0;
}

/* Retrieves a pointer to the head of the pending message list associated to a user
    Return the pointer to the head of the message list 
        NULL if the user was not found */
struct msg ** getPendMsgHead(char * username){
    struct user *temp = user_head;
    /* Iterate over the list */
    while(temp != NULL){
        if(strcmp(temp->username, username) == 0){  //User found
            return &(temp->pend_msgs_head);     //Returns the IP of the user
        }
        temp = temp->next;
    }
    return NULL;
}

