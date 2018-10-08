#define MAX_MSG 256
#define MAX_MD5 33

struct msg{
    char body[MAX_MSG];		/* Content of the message */
    char sender[MAX_MSG];	/* Sender of the message */
    char md5[MAX_MD5];		/* MD5 of the message */
    unsigned int id;		/* ID assigned to the message */
    struct msg *next;		/* Pointer to the next message in the list */
};

/* ================FUNCTION HEADERS================ */
int enqueueMsg(struct msg **head, char * message, char * md5, unsigned int id, char * sender);
struct msg * dequeueMsg(struct msg **head);
void deleteAllMsgs(struct msg ** head);