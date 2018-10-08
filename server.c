#include <netinet/in.h>		/* For addresses in PF_INET */
#include <netdb.h>			/* Address-->Network and Network-->Address library; gethostbyname; gethostbyaddr */
#include <sys/types.h>
#include <net/if.h>			/* To use ifreq */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "lists/read_line.h"
#include "lists/user_list.h"
#include "lists/msg_list.h"
#include "server.h"
#include "rpc_store_service/store_service.h"

/* Handler for interrupts */
void interruptHandler(int sig){
	printf("[SERVER]: Handling interrupt. Closing server socket...\n");
	/* Close the server socket and exit with the resulting return value. 0 if OK, -1 if error */
	exit(close(s_server));
}

int main(int argc, char * argv[]){
	struct sockaddr_in server_addr, client_addr;
	int sc;
	int val;
	int server_port;

	/* Check command */
	if(argc != 5 || strcmp(argv[1],"-p") != 0){
		printf("Usage: ./server -p <port> -s <store_service>\n");
		exit(-1);
	} 

	/* Check if the port number passed as parameter is valid */
	server_port = atoi(argv[2]);
	if ((server_port < 1024) || (server_port > 65535)) {
			printf("Error: Port must be in the range 1024 <= port <= 65535\n");
			exit(-1);
	}

	/* Store the IP of the storage service */
	store_service_ip = argv[4];

	/* Initialize mutexes */
    if(pthread_mutex_init(&socket_mtx, NULL) != 0) {
        printf("[SERVER]: Error when initializing the mutex");
        exit(-1);
    }
    if(pthread_mutex_init(&list_mtx, NULL) != 0) {
        printf("[SERVER]: Error when initializing the mutex");
        exit(-1);
    }
    /* Initialize condition variable for copying the socket descriptor in the thread */
    if(pthread_cond_init(&free_socket, NULL) != 0) {
        printf("[SERVER]: Error when initializing the mutex");
        exit(-1);
    }

	/* Prepare thread conditions */
	thread = (pthread_t) malloc((sizeof(thread)));
	pthread_attr_init(&thread_att);
    pthread_attr_setdetachstate(&thread_att, PTHREAD_CREATE_DETACHED);

	s_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	/* This socket has no address assigned */
	if(s_server == -1){
		printf("Error when creating the socket");
		exit(-1);
	}

	/* Obtain the IP address attached to interface eth0 */
	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(s_server, SIOCGIFADDR, &ifr); 

	val = 1;
	setsockopt(s_server, SOL_SOCKET, SO_REUSEADDR, (char*) &val, sizeof(int)); /* Makes the address of the socket reusable */
	
	/* Initialize the address that will be attached to the listening socket */
	bzero((char*) &server_addr, sizeof(server_addr));	/* Initialize the socket address of the server to 0 */
	server_addr.sin_family			= AF_INET;
	server_addr.sin_addr			= ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;	/* Listens to IP address in eth0 interface*/
	server_addr.sin_port			= htons(server_port);	/* Port number */

	/* Bind the address to the listening socket */
	if((bind(s_server, (struct sockaddr*) &server_addr, sizeof(server_addr))) == -1){
		printf("Error when binding the address to the socket");
		exit(-1);
	}

	/* Set the socket to listen incoming requests */
	if(listen(s_server, 5) == -1){
		printf("Error when listening to the socket");
		exit(-1);
	} /* Backlog is 5, maximum number of queued requests is 5 */

	/* Once the server is listening, print inicial prompt */
	printf("s> init server %s:%d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
		 ntohs(server_addr.sin_port));

	/* Define the variable for the client address size. It should be declared
	as variable because the size depends on the incoming request and is an
	output parameter of the 'accept' function */
	socklen_t cl_addr_size = sizeof(client_addr);

	/* Set the control variable to TRUE so that the listening thread waits
	until the thread stores a local copy of the socket descriptor */
	busy_socket = TRUE;

	/**********************************/
	/* Initialize the storage service */
	CLIENT *clnt;
	/* Create connection with the storage service */
	clnt = clnt_create (store_service_ip, STORE_SERVICE, STORE_VERSION, "tcp");
	/* If error, the service is unavailable. Show error and exit */
	if (clnt == NULL) {
		fprintf(stderr, "s> ERROR, STORAGE SERVICE UNAVAILABLE\n");
	}
	else{
		init_1(NULL, clnt);
		clnt_destroy (clnt);
	}
	/**********************************/
	signal(SIGINT, interruptHandler); /* Handles the ctrl+c signal to interrupt the server */
	fprintf(stderr, "%s", "s> ");	/* Prompt */


	/* Loop for accepting and creating threads for each incoming request */
	while(1){
		/* Accept client connections. If error, shut down the server */
		sc = accept(s_server, (struct sockaddr *) &client_addr, &cl_addr_size);
		if(sc == -1){
			printf("Error when accepting the connection");
			/* Close listening server socket */
			close(s_server);
			exit(-1);
		}
		/* Once accepted, create a thread to handle the request. If error, shut down the server */
		if(pthread_create(&thread, &thread_att, (void *) manageRequest, &sc) != 0) {
        	printf("[SERVER]: Error when creating the thread");
        	/* Close both listening socket and the one resulting from the accept operation */
        	close(s_server);
        	close(sc);
        	exit(-1);
    	}

		/* Wait for the thread to copy the socket descriptor locally */
		pthread_mutex_lock(&socket_mtx);
		while(busy_socket == TRUE) 
			pthread_cond_wait(&free_socket, &socket_mtx);
		busy_socket = TRUE;
		pthread_mutex_unlock(&socket_mtx);

	}

	exit(0);
}

void * manageRequest(int *sd){
	int s_local;
	char operation_buff[MAX_COMMAND];
	char user_buff[MAX_USERNAME];
	char msg_buff[MAX_MSG];
	char md5_buff[MAX_MD5];
	int n;
	int m;
	char out;

	/* Copy locally the socket descriptor */
	pthread_mutex_lock(&socket_mtx);
	s_local = *sd;
	busy_socket = FALSE;
	pthread_cond_signal(&free_socket);
	pthread_mutex_unlock(&socket_mtx);

	/* Read the operation. If error, close the socket and terminate the thread */
	n = readLine(s_local, operation_buff, MAX_COMMAND);
	if(n == -1){
		printf("[SERVER_THREAD]: Error when reading from the socket");
		if(close(s_local) == -1){
			/* If there is an error when closing the socket, shut down the server */
			interruptHandler(SIGINT);
		}
		pthread_exit((void *)-1); //Terminate thread with -1 return value
	}
	/* Read the username and convert to uppercase. If error, close the socket 
	and terminate the thread */
	m = readLine(s_local, user_buff, MAX_USERNAME);
	if(m == -1){
		printf("[SERVER_THREAD]: Error when reading from the socket\n");
		if(close(s_local) == -1){
			/* If there is an error when closing the socket, shut down the server */
			interruptHandler(SIGINT);
		}
		pthread_exit((void *)-1); //Terminate thread with -1 return value
	}
	/* For convention of the server, convert every username to uppercase */
	toUpperCase(user_buff);

	/* Check the operation */
	if (strcmp(operation_buff, "REGISTER") == 0){
		/* Register the user */
		pthread_mutex_lock(&list_mtx);
		out = registerUser(user_buff);
		pthread_mutex_unlock(&list_mtx);

	}
	else if (strcmp(operation_buff, "UNREGISTER") == 0){
		/* Unregister the user */
		pthread_mutex_lock(&list_mtx);
		out = unregisterUser(user_buff);
		pthread_mutex_unlock(&list_mtx);

	}
	else if(strcmp(operation_buff, "CONNECT") == 0){

		struct sockaddr_in client_addr_local;
		socklen_t addr_len = sizeof(client_addr_local);
		uint16_t client_port;

		n = readLine(s_local, msg_buff, MAX_MSG);
		if(n == -1){
			printf("[SERVER_THREAD]: Error when reading from the socket");
			if(close(s_local) == -1){
				/* If there is an error when closing the socket, shut down the server */
				interruptHandler(SIGINT);
			}
			pthread_exit((void *)-1); //Terminate thread with -1 return value
		}
		/* Get the port number from the socket */
		client_port = (uint16_t) atoi(msg_buff);
		/* Get the client IP address attached to the socket */
		int err = getpeername(s_local, (struct sockaddr *) &client_addr_local, &addr_len);
		if (err == -1){
			printf("[SERVER_THREAD]: Error when getting client address");
			/* Send error 3 to client and close socket */
			out = 3;
		}
		/* Connect the user to the server if no error */
		if(out != 3){
			pthread_mutex_lock(&list_mtx);
			out = connectUser(user_buff, inet_ntoa(client_addr_local.sin_addr), client_port);
			pthread_mutex_unlock(&list_mtx);
		}

		/* If result is 0, then check for the pending messages and send them */
		if(out == 0){
			/* Send code 0 for the client to open the listening thread */
			if((send_msg(s_local, &out, sizeof(out))) == -1){
			/* If error when sending the message, close the socket and exit */
			if(close(s_local) == -1){
					/* If there is an error when closing the socket, shut down the server */
					interruptHandler(SIGINT);
				}
				pthread_exit((void *)-1); //Terminate thread with -1 return value
			}
			fprintf(stderr, "%s %s %s", operation_buff, user_buff, "OK");
			fprintf(stderr, "\n%s", "s> ");	/* Prompt */

			/* Send Pending Messages */
			pthread_mutex_lock(&list_mtx);
			struct msg **pend_msg = getPendMsgHead(user_buff);
			while(*pend_msg != NULL){
				pthread_mutex_unlock(&list_mtx);
				char sender[MAX_USERNAME];
				char msg_body[MAX_MSG];
				char msg_md5[MAX_MD5];

				/* Get the name of the sender, the id and the body associated to the message to be sent */
				pthread_mutex_lock(&list_mtx);
				strcpy(sender, (*pend_msg)->sender);
				int msg_id = (*pend_msg)->id;
				strcpy(msg_body, (*pend_msg)->body);
				strcpy(msg_md5, (*pend_msg)->md5);
				pthread_mutex_unlock(&list_mtx);

				/* Try to send the message. The 'stored' flag is set to 1 because the message
				is already stored in the server */
				int err = sendMessage(sender, user_buff, msg_body, msg_md5, msg_id, 1);

				/* If the message could not be delivered/stored, then exit the loop */
				if(err != 0) goto destroy_thread;

				/* Send acknowledge to the sender. No return value is checked */
				sendAck(sender, msg_id);

				/* Remove the message from the pending message queue and iterate with the next message */
				pthread_mutex_lock(&list_mtx);
				*pend_msg = dequeueMsg(&(*pend_msg));
			}
			pthread_mutex_unlock(&list_mtx);
			goto destroy_thread;
		}
	}
	else if(strcmp(operation_buff, "DISCONNECT") == 0){
		/* Get the IP from which the command is being executed */
		struct sockaddr_in client_addr_local;
		socklen_t addr_len = sizeof(client_addr_local);

		int err = getpeername(s_local, (struct sockaddr *) &client_addr_local, &addr_len);
		if (err == -1){
			printf("Error when getting client address");
			/* Send error 3 to client and close socket */
			out = 3;
			goto respond_to_client;
		}
		/* Try to disconnect the user passing the IP from which the request is being made
		as parameter to the function */
		pthread_mutex_lock(&list_mtx);
		out = disconnectUser(user_buff, inet_ntoa(client_addr_local.sin_addr));
		pthread_mutex_unlock(&list_mtx);

	}
	else if(strcmp(operation_buff, "SEND") == 0){
		/* Reserve a buffer for the username of the receiver */
		char dest_user_buff[MAX_USERNAME];

		/* Read the destination user from the socket */
		m = readLine(s_local, dest_user_buff, MAX_USERNAME);
		if(m == -1){
			printf("[SERVER_THREAD]: Error when reading from the socket\n");
			if(close(s_local) == -1){
				/* If there is an error when closing the socket, shut down the server */
				interruptHandler(SIGINT);
			}
			pthread_exit((void *)-1); //Terminate thread with -1 return value
		}
		/* Convert username to uppercase by convention */
		toUpperCase(dest_user_buff);

		/* Read the message from the socket */
		n = readLine(s_local, msg_buff, MAX_MSG);
		if(n == -1){
			printf("[SERVER_THREAD]: Error when reading from the socket\n");
			if(close(s_local) == -1){
				/* If there is an error when closing the socket, shut down the server */
				interruptHandler(SIGINT);
			}
			pthread_exit((void *)-1); //Terminate thread with -1 return value
		}

		/* Read the MD5 hash from the socket */
		m = readLine(s_local, md5_buff, MAX_MD5);
		if(m == -1){
			printf("[SERVER_THREAD]: Error when reading from the socket\n");
			if(close(s_local) == -1){
				/* If there is an error when closing the socket, shut down the server */
				interruptHandler(SIGINT);
			}
			pthread_exit((void *)-1); //Terminate thread with -1 return value
		}
		
		/* Check if one of the two users is not registered */
		pthread_mutex_lock(&list_mtx);
		if(!isRegistered(user_buff) || !isRegistered(dest_user_buff)){
			pthread_mutex_unlock(&list_mtx);
			/* Send code 1 to the client and close the socket */
			out = 1; 
			goto respond_to_client;
		}
		pthread_mutex_unlock(&list_mtx);

		/* Check the status of the destination user */
		pthread_mutex_lock(&list_mtx);
		char status = isConnected(dest_user_buff);
		unsigned int last_id = updateLastID(user_buff); //Update the last id of the sender message
		pthread_mutex_unlock(&list_mtx);

		if(status == 0){ //Not connected
			/* Store the message */
			if (storeMessage(user_buff, dest_user_buff, msg_buff, md5_buff, last_id) != 0){
				/* Message could not be stored so send code 2 to the client and close the socket */
				out = 2;
				goto respond_to_client;
			}
			/* Message was stored successfully, send code 0 and message ID to the client */
			out = 0;
			/////////////////////////////////////////////
			/* Store the message in the storage server */
			storeMessage_svc(user_buff, dest_user_buff, last_id, msg_buff, md5_buff);

			if((send_msg(s_local, &out, sizeof(out))) == -1){
				/* If error when sending the message, close the socket and exit */
				if(close(s_local) == -1){
					/* If there is an error when closing the socket, shut down the server */
					interruptHandler(SIGINT);
				}
				pthread_exit((void *)-1); //Terminate thread with -1 return value
			}
			/* Send string with the message ID back to the sender */
			char id_string[11];
			sprintf(id_string, "%d", last_id);
			if((send_msg(s_local, id_string, strlen(id_string)+1)) == -1){
				/* If error when sending the message, close the socket and exit */
				if(close(s_local) == -1){
					/* If there is an error when closing the socket, shut down the server */
					interruptHandler(SIGINT);
				}
				pthread_exit((void *)-1); //Terminate thread with -1 return value
			}

		}else if(status == 1){ //Connected
			/* Try to send the message to the receiver. We set the 'stored' flag to 0 because the message
			is being sent for the first time and was not previously stored int he server */
			int err = sendMessage(user_buff, dest_user_buff, msg_buff, md5_buff, last_id, 0);
			/* If while trying to store the message, the user unregisters, value 1 will be returned */
			if(err == 1){
				out = 1;
				goto respond_to_client;
			}else if(err == -1){
				/* If any server error occurred and the message was not stored or sent, then send code 2
			back to the client */
				out = 2;
				goto respond_to_client;
			}

			/* If no server error occured, then the message was either sent or stored, so we send back
			the code 0 (OK) to the client */
			out = 0;
			/////////////////////////////////////////////
			/* Store the message in the storage server */
			storeMessage_svc(user_buff, dest_user_buff, last_id, msg_buff, md5_buff);

			if ((send_msg(s_local, &out, sizeof(out))) == -1){
				/* If error when sending the message, close the socket and exit */
				if(close(s_local) == -1){
					/* If there is an error when closing the socket, shut down the server */
					interruptHandler(SIGINT);
				}
				pthread_exit((void *)-1); //Terminate thread with -1 return value
			}
			/* Send string with the message ID back to the sender */
			char id_string[11];
			sprintf(id_string, "%d", last_id);
			if((send_msg(s_local, id_string, strlen(id_string)+1)) == -1){
				/* If error when sending the message, close the socket and exit */
				if(close(s_local) == -1){
					/* If there is an error when closing the socket, shut down the server */
					interruptHandler(SIGINT);
				}
				pthread_exit((void *)-1); //Terminate thread with -1 return value
			}

			/* At this point, the message is assumed to  */
			sendAck(user_buff, last_id);
		}
		/* The response to the client is handled within this else-if statement, so the
		'respond_to_client' label is skipped and proceed to close the socket */
		goto destroy_thread; 
	}

	/* Default print template */
	switch(out){
		case 0:
			fprintf(stderr, "%s %s %s", operation_buff, user_buff, "OK");
			fprintf(stderr, "\n%s", "s> ");	/* Prompt */
			break;
		default:
			fprintf(stderr, "%s %s %s", operation_buff, user_buff, "FAIL");
			fprintf(stderr, "\n%s", "s> ");	/* Prompt */
	}

	/* Label to jump previous code to respond the client and skip the default print right above,
	in case other commands (as SEND) do not use a template print */
	respond_to_client: 
		if((send_msg(s_local, &out, sizeof(out))) == -1){
			/* If error when sending the message, close the socket and exit */
			if(close(s_local) == -1){
				/* If there is an error when closing the socket, shut down the server */
				interruptHandler(SIGINT);
			}
			pthread_exit((void *)-1); //Terminate thread with -1 return value
		}

	destroy_thread:
		if(close(s_local) == -1){
			printf("[SERVER_THREAD]: Error when closing the socket in the thread");
			exit(-1);
		}
		pthread_exit(NULL);
}

/* Capitalizes the input string. String is both an input and output parameter */
void toUpperCase(char * string){
	/* Convert to uppercase */
	int i;
	for(i = 0; string[i]; i++){
  		string[i] = toupper(string[i]);
	}
}

/* Return 0: Message is stored OK
   Return -1: Server error (Memory space error) */
int storeMessage(char * sender, char * receiver, char * msg, char * md5, unsigned int msg_id){
	/* Store the message to the receiver pending list */
	pthread_mutex_lock(&list_mtx);
	int err = storeMsg(receiver, msg, msg_id, md5, sender);
	pthread_mutex_unlock(&list_mtx);

	/* Error when trying to store the message */
	if(err == -1) return -1;

	fprintf(stderr, "MESSAGE %d FROM %s TO %s STORED", msg_id,
									 sender, receiver);
	fprintf(stderr, "\n%s", "s> ");	/* Prompt */

	/* Return store OK */
	return 0;
}

/* Return 0: Message is sent OK
   Return 1: User did not exist when trying to store/send the message. Message not stored
   Return 2: Message is stored, or not stored if was already stored 
   Return -1: Server error */
int sendMessage(char * sender, char * receiver, char * msg, char * md5, unsigned int msg_id, char stored){
	int s_receiver; //Socket for the receiver of the message
	struct sockaddr_in recv_addr; //Receiver address
	struct hostent *recv_hp; //Host entity structure for the receiver

	s_receiver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_receiver == -1){
		return -1; //Send error message. -1 is internally encoded as server error
	}

	bzero((char *) &recv_addr, sizeof(recv_addr)); //Reserve space for the address of the receiver

	pthread_mutex_lock(&list_mtx);
	recv_hp = gethostbyname(getUserIP(receiver)); //Get the IP of the receiver
	pthread_mutex_unlock(&list_mtx);
	/* If error when getting the host, return -1 */
	if(recv_hp == NULL) return -1;

	memcpy(&(recv_addr.sin_addr), recv_hp->h_addr, recv_hp->h_length); //Get the IP addres in network format
	recv_addr.sin_family = AF_INET;
	pthread_mutex_lock(&list_mtx);
	recv_addr.sin_port = htons(getUserPort(receiver)); //Get the port number of the receiver listening thread
	pthread_mutex_unlock(&list_mtx);

	/* Try to connect to the listening thread of the receiver to send the message */
	if (connect(s_receiver, (struct sockaddr *) &recv_addr, sizeof(recv_addr)) == -1){
		/* If the connection with the receiver fails, assume the client 
		to be disconnected, disconnect it and store the message */
		pthread_mutex_lock(&list_mtx);
		/* As we are internally disconnecting the user from the server, we need to bypass the 
		IP check, so we pass the IP of the receiver as parameter to always fulfill the condition */
		char reg = disconnectUser(receiver, getUserIP(receiver)); // No need to check for output
		pthread_mutex_unlock(&list_mtx);
		/* If the disconnect method returns 1, it means that the user was not found so is not
		registered (it unregister while trying to store the message, so we return 1 */
		if(reg == 1){
			return 1;
		}
		/* If the stored parameter is set to 0, it means that the message was not prevoiusly stored by the
		server so we need to push it to the end of the queue. If it was stored, then nothing is done */
		if(!stored){
			if(storeMessage(sender, receiver, msg, md5, msg_id) == -1) return -1; //Return -1 if store error
		}

		if(close(s_receiver) == -1){ //Close the socket
			/* If there is an error when closing the socket, shut down the server */
			interruptHandler(SIGINT);
		}
		/* Return 2 to indicate the message is stored but not sent */
		return 2;
	}
	/* Send the SEND_MESSAGE string to the receiver to detect an incoming message */
	char op[13];
	strcpy(op, "SEND_MESSAGE");
	send_msg(s_receiver, op, strlen(op)+1);
	/* Send the sender name */
	send_msg(s_receiver, sender, strlen(sender)+1);
	/* Send the identifier of the message */
	char id_string[11];
	sprintf(id_string, "%d", msg_id);
	send_msg(s_receiver, id_string, strlen(id_string)+1);
	/* Send the MD5 of the message */
	send_msg(s_receiver, md5, strlen(md5)+1);
	/* Send the message */
	send_msg(s_receiver, msg, strlen(msg)+1);

	if(close(s_receiver) == -1){ //Close the socket
		/* If there is an error when closing the socket, shut down the server */
		interruptHandler(SIGINT);
	}

	fprintf(stderr, "SEND MESSAGE %d FROM %s TO %s", msg_id,
										 sender, receiver);
	fprintf(stderr, "\n%s", "s> ");	/* Prompt */

	return 0;
}

/* Tries to send acknowledge to the sender. No return value */
void sendAck(char * sender, unsigned int msg_id){
	int s_sender; //Socket for the receiver of the message
	struct sockaddr_in sender_addr; //Sender address
	struct hostent *sender_hp; //Host entity structure for the sender

	s_sender = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_sender == -1){
		/* If error when allocating resources for the socket, then exit */
		return;
	}

	char ack_op[14];
	strcpy(ack_op, "SEND_MESS_ACK");

	bzero((char *) &sender_addr, sizeof(sender_addr));

	pthread_mutex_lock(&list_mtx);
	sender_hp = gethostbyname(getUserIP(sender));
	pthread_mutex_unlock(&list_mtx);
	/* If any error when getting the hoset, exit the function */
	if(sender_hp == NULL) return;

	memcpy(&(sender_addr.sin_addr), sender_hp->h_addr, sender_hp->h_length);
	sender_addr.sin_family = AF_INET;

	pthread_mutex_lock(&list_mtx);
	sender_addr.sin_port = htons(getUserPort(sender));
	pthread_mutex_unlock(&list_mtx);

	if((connect(s_sender, (struct sockaddr *) &sender_addr, sizeof(sender_addr))) == -1){
		/* If error when connecting, exit the function */
		return;
	}


	char id_string[11];
	sprintf(id_string, "%d", msg_id);
	if((send_msg(s_sender, ack_op, strlen(ack_op)+1)) == -1){
		/* If error when sending the ACK, close the socket and exit the function */
		if(close(s_sender) == -1){
			/* If there is an error when closing the socket, shut down the server */
			interruptHandler(SIGINT);
		}
		return;
	}
	if((send_msg(s_sender, id_string, strlen(id_string)+1)) == -1){
		/* If error when sending the ACK, close the socket and exit the function */
		if(close(s_sender) == -1){
			/* If there is an error when closing the socket, shut down the server */
			interruptHandler(SIGINT);
		}
		return;
	}

	if(close(s_sender) == -1){ //Close the socket
		/* If there is an error when closing the socket, shut down the server */
		interruptHandler(SIGINT);
	}
	return;
}
/* Connects to the storage service (if available) and stores the message with the corresponding information
passed as parameters */
void storeMessage_svc(char * sender, char * receiver, unsigned int id, char * msg, char * md5){
	CLIENT *clnt;
	/* Create connection with the storage service */
	clnt = clnt_create (store_service_ip, STORE_SERVICE, STORE_VERSION, "tcp");
	/* If error, the service is unavailable. Show error and exit */
	if (clnt == NULL) {
		fprintf(stderr, "ERROR, STORAGE SERVICE UNAVAILABLE");
		fprintf(stderr, "\n%s", "s> ");	/* Prompt */
		return;
	}
	int result;
	/* Call the storage service */
	store_1(sender, receiver, id, msg, md5, &result, clnt);
	/* Check for internal server error in the process */
	if(result == -1) fprintf(stderr, "ERROR IN THE STORAGE SERVICE");
	/* If everything went OK, prompt a message in the console */
	else fprintf(stderr, "MESSAGE %d STORED OK IN STORAGE SERVICE", id);
	fprintf(stderr, "\n%s", "s> ");	/* Prompt */
	/* Destroy the client and return */
	clnt_destroy (clnt);
	return;
}