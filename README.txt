Carlos Gutierrez - 100291121
Ruben Lopez - 100292107


INSTRUCTIONS FOR COMPILING AND EXECUTING THE PROGRAM:

**IMPORTANT NOTE: IF THE PROGRAMS ARE TO BE RUN IN THE SAME MACHINE, IT IS RECOMMENDED TO USE LOOPBACK
IP ADDRESSES (INSTEAD OF THE PUBLIC IP) TO AVOID CONNECTIVITY PROBLEMS**

------------
COMPILATION
------------
- Extract the ZIP folder by running 'unzip ssdd_p2_100291121_100292107.zip -d <destination_folder>'.
This will extract the contents in the <destination_folder> provided.

- Navigate to the <destination_folder>

- Execute the command 'ls' and make sure that the 'Makefile' is located in the directory

- Run the command 'make' and wait until the process is finished. Now all the necessary files
will be compiled and ready to be executed.

----------
EXECUTION
----------
Open 5 terminals and navigate to the root directory where the files are located, and then execute the
following commands:

-Terminal 1: 'java -cp . md5.server.endpoint.MD5Publisher'

-Terminal 2: './storeServer'

-Terminal 3: './server -p <port_number> -s <storage_service_ip>'
where <port_number> is the port number in which the service will be running, and <storage_service_ip> is the IP
address that is shown in Terminal 2 after executing the command shown above

-Terminal 4: 'java -cp . client -s <messaging_service_ip> -p <port_number> -w <md5_service_ip:md5_service_port>'
where <messaging_service_ip> is the IP address of the messaging service that is shown after executing the command
in Terminal 3; <port_number> is the same port number indicated in the command of Terminal 3;
and <md5_service_ip:md5_service_port> is the IP address and the port number that are shown after executing the
command in Terminal 1, in the format IP:port

-Terminal 5: './monitor <storage_service_ip> <client> <message_id>'
where <storage_service_ip> is the IP address that is shown in Terminal 2 after executing the command shown above

Terminal 4 will execute the client and can be replicated more times to create more clients and communicate between them
in real time
