
#include "rpc_store_service/store_service.h"

int
main (int argc, char *argv[])
{
	char *host;
	enum clnt_stat retval; /* Return value for the getmessage call */
	/* Check the parameters of the command */
	if (argc < 4) {
		printf ("usage: %s server_host <client_id> <message_id>\n", argv[0]);
		exit (1);
	}
	/* Get the address of the host from the first paramete */
	host = argv[1];
	/* Get the ID from the third parameter and store it in an unsigned int */
	char *stopstring;
	unsigned int id = strtoul(argv[3], &stopstring, 10);

	CLIENT *clnt;
	/* Create the connection with the service */
	clnt = clnt_create (host, STORE_SERVICE, STORE_VERSION, "tcp");
	if (clnt == NULL) {
		printf("ERROR , SERVICE NOT AVAILABLE\n");
		exit (1);
	}
	/* Allocate resources for the response */
	response *res = malloc(sizeof(response));
	res->msg = calloc(MAX_SIZE, sizeof(char));
	res->md5 = calloc(MAX_MD5, sizeof(char));
	/* Call the get message service with the client ID and message ID passed as parameters */
	retval = getmessage_1(argv[2], id, res, clnt);
	/* If FALSE is returned, there was an internal server error */
	if(retval != RPC_SUCCESS) printf("ERROR , SERVICE NOT AVAILABLE\n");
	/* if the length of the receive message is 0, no message was found */
	if(strlen(res->msg) == 0) printf ("ERROR , MESSAGE DOES NOT EXIST\n");
	/* Otherwise, print the message and its MD5 hash */
	else{
		printf("MESS: %s\n", res->msg);
		printf("MD5: %s\n", res->md5);
	}
	/*******************************************/
	/* Sample code for the getnummessages call */
	/*******************************************/
	/*
	int result;
	getnummessages_1(argv[2], &result, clnt);
	if(result != -1) printf("Total number of messages: %d\n", result);
	else printf("The tuple client-id does not exist.\n");
	*/
	/* Destroy the connection */
	clnt_destroy (clnt);
	exit (0);
}
