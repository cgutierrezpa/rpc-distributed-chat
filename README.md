# RPC Distributed Chat System

This project was developed during the _Distributed Systems_ course taken in Universidad Carlos III de Madrid during my Computer Science and Engineering Bachelor's degree. It consists of a distributed chat system that allowed multiple distributed Java clients to be connected by means of a Server coded in C that handles the client-server connections using RPCs.

## Instructions

__IMPORTANT NOTE: IF THE CLIENTS ARE TO BE EXECUTED IN THE SAME MACHINE, IT IS RECOMMENDED TO USE LOOPBACK IP ADDRESSES (INSTEAD OF THE PUBLIC IP) TO AVOID CONNECTIVITY PROBLEMS__

### Compilation

Navigate to the __root__ directory and run ``make`` in the terminal. 


### Execution

In the __root__ directory, open __5 Terminals__ and then execute the following commands:

* Terminal 1:
```
java -cp . md5.server.endpoint.MD5Publisher
```

* Terminal 2: 
```
./storeServer
```

* Terminal 3: 
```
./server -p <port_number> -s <storage_service_ip>
```
where __port_number__ is the port number in which the Server Service will be running, and __storage_service_ip__ is the IP address of the Storage Service __that is shown in Terminal 2__

* Terminal 4: 
```
java -cp . client -s <messaging_service_ip> -p <port_number> -w <md5_service_ip:md5_service_port>
```
where __messaging_service_ip__ is the IP address of the messaging service that is shown in Terminal 3; __port_number__ is the same port number indicated in the output of Terminal 3; and __md5_service_ip:md5_service_port__ is the IP address and the port number that are shown in Terminal 1, in the format _IP:port_

* Terminal 5:
```
./monitor <storage_service_ip> <client> <message_id>
```
where __storage_service_ip__ is the IP address that is shown in Terminal 2

Terminal 4 will execute the client and can be replicated as many times as desired to create other clients and communicate between them in real time
