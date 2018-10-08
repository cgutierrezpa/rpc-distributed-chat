#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "msg_list.h"

 
/* Creates a new message struct and enqueues it to the end of the message queue
    Returns 0 if the message is stored
        -1 if malloc error */
int enqueueMsg(struct msg **head, char * message, char * md5, unsigned int id, char * sender){
    struct msg *temp;
    /* Allocate the space for the new message */
    temp = (struct msg *) malloc(sizeof(struct msg));
    /* If malloc returns error (full memory or other) */
    if(temp == NULL) return -1;
    strcpy(temp->body, message);
    strcpy(temp->md5, md5);
    strcpy(temp->sender, sender);
    temp->id = id;
    temp->next = NULL;

    if (*head == NULL){      /* Queue is empty */
        temp->next = *head;
        *head = temp;
    }
    else{
        /* If the queue is not empty, iterate to the end and append the message */
        struct msg *last = *head;
        while(last->next != NULL){
            last = last->next;
        }
        last->next = temp;
    }
    return 0;
}
/* Deletes the message at the head of the queue and returns the new
 head of the list 
    Return a pointer to the next message in the queue
        NULL if the list is left empty */
struct msg * dequeueMsg(struct msg **head){
    struct msg* temp = *head;
    /* Head pointing to the next element */
    *head = temp->next;
    /* Free the resources of the first message */
    free(temp);
    /* Return the new head of the queue */
    return *head;
}

/* Deletes all the messages in the list from the head of the list
passed as paremeter */
void deleteAllMsgs(struct msg ** head){
    struct msg **temp = head;
    while(*temp != NULL){
        *temp = dequeueMsg(&(*temp));
    }
    return;
}