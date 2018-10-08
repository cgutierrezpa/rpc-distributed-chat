#define MAX_USERNAME 256
#define MAX_IP 16
#define TRUE 	1
#define FALSE 	0

struct user{
    char username[MAX_USERNAME];    /* Username that acts as ID */
    char status;                    /* Status of the client: 0 if "OFF"; 1 if "ON" */
    char ip[MAX_IP];				/* IP of the user from which the connect operation was made */
    uint16_t port;					/* Port number of the user from which the connect operation was made */
    unsigned int last_id;			/* ID assigned to the last sent message */
    struct msg *pend_msgs_head;		/* Pointer to the head of the pending messages queue */
    struct user *next;				/* Pointer to the next user in the list */
} *user_head;

/* ================FUNCTION HEADERS================ */
char isRegistered(char * username);
char registerUser(char * username);
char unregisterUser(char * username);
char connectUser(char * username, char * ip, uint16_t port);
char disconnectUser(char * username, char * used_ip);
int storeMsg(char * username, char* msg, unsigned int msg_id, char * md5, char * sender);
unsigned int updateLastID(char * username);
char isConnected(char * username);
char * getUserIP(char * username);
uint16_t getUserPort(char * username);
struct msg ** getPendMsgHead(char * username);