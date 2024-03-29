#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>
#include <ctype.h>
#include "utilities.h"

int killFlag = 0;
int connections = 0;
messageBox *boxHead = NULL;
Node *threadHead = NULL;
pthread_mutex_t lock;

void sighandler(int signum){
	exit(1);
}

void closeBox(int clientSocket, char *name){
	messageBox *boxPtr;
	for(boxPtr = boxHead; boxPtr != NULL; boxPtr = boxPtr->next_box){
		if(strcmp(boxPtr->box_name, name) == 0){
			boxPtr->open = -1;
			break;
		}
	}
	return;
}

int delete(int clientSocket, char *name){
	if(strcmp(boxHead->box_name, name) == 0){
		if(boxHead->message != NULL){
			return -1;
		}else{
			messageBox *temp = boxHead;
			boxHead = boxHead->next_box;
			free(temp->box_name);
			free(temp);
			return 1;
		}
	}
	messageBox *prevPtr = NULL;
	messageBox *ptr = boxHead;
	while(strcmp(ptr->box_name, name) != 0){
		prevPtr = ptr;
		ptr = ptr->next_box;
	}
	prevPtr->next_box = ptr->next_box;
	free(ptr);
	return 1;
}

void put(int clientSocket, int length, char *text, char *name){
	messageBox *boxPtr;
	for(boxPtr = boxHead; boxPtr != NULL; boxPtr = boxPtr->next_box){
		if((strcmp(boxPtr->box_name, name) == 0) && (boxPtr->clientSocket == clientSocket)){
			Message *newMessage = (Message*)malloc(sizeof(Message));
			newMessage->text = malloc(strlen(text)+1);
			strcpy(newMessage->text, text);
			newMessage->length = length;
			newMessage->next_msg = NULL;
			if(boxPtr->message == NULL){
				boxPtr->message = newMessage;
			}else{
				Message *msgPtr;
				for(msgPtr = boxPtr->message; msgPtr->next_msg != NULL; msgPtr = msgPtr->next_msg){
				}
				msgPtr->next_msg = newMessage;
			}
			break;
		}
	}
	return;
}
//(-1): incorrect format
int putFormatCheck(char *arg0, char *arg1){
	int i;
	for(i=0; i < strlen(arg0); i++){
		if(isdigit(arg0[i]) == 0){
			return -1;
		}
	}
	if(atoi(arg0) != strlen(arg1)){
		return -1;
	}
	
	return 1;
}

Message* next(int clientSocket, char *name){
	messageBox *boxPtr;
	for(boxPtr = boxHead; boxPtr != NULL; boxPtr = boxPtr->next_box){
		if(strcmp(boxPtr->box_name, name) == 0){
			if(boxPtr->message != NULL){
				Message *msgPtr = boxPtr->message;
				boxPtr->message = boxPtr->message->next_msg;
				return msgPtr;
			}else{
				break;
			}
		}
	}
	return NULL;
}

void openBox(int clientSocket, char *name){
	messageBox *boxPtr;
	for(boxPtr = boxHead; boxPtr != NULL; boxPtr = boxPtr->next_box){
		if(strcmp(boxPtr->box_name, name) == 0){
			boxPtr->open = 1;
			boxPtr->clientSocket = clientSocket;
			break;
		}
	}
	return;
}

//(-2): invalid name, (-1): message box exists but its open, (0): message box exists and its closed, (1): message box does not exist
int validName(char *name, int length){
	if(length < 5 || length > 25) return -2;
	if(!((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) return -2;
	messageBox *boxPtr;
	for(boxPtr = boxHead; boxPtr != NULL; boxPtr = boxPtr->next_box){	//Checks if message box already exists
		if(strcmp(boxPtr->box_name, name) == 0){
			if(boxPtr->open == 1){
				return -1;	
			}else{
				return 0;
			}
		}
	}
	return 1;
}

void create(int clientSocket, char *name){
	messageBox *newBox = (messageBox*)malloc(sizeof(messageBox));
	newBox->box_name = malloc(strlen(name)+1);
	strcpy(newBox->box_name, name);
	newBox->open = -1;
	newBox->message = NULL;
	newBox->next_box = boxHead;
	boxHead = newBox;
}

//Server forms the listener socket while client reaches out to the server
void commandHandler(void* args){
	commandArgs *arguments = (commandArgs*)args;
	
	int msgLength = 0;
	char buffer[1024] = {0};
	char clientID[1024] = {0};
	char* confirmation = "HELLO DUMBv0 ready!\n";
	
	time_t timed = time(NULL);
	struct tm *ptm = localtime(&timed);

	msgLength = recv(arguments->clientSocket, clientID, 1024, 0);
	send(arguments->clientSocket, confirmation, strlen(confirmation), 0);
	clientID[msgLength] = '\0';
	printf("%02d%02d %d Dec %s connected\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID);
	printf("%02d%02d %d Dec %s HELLO\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID);
	connections++;

	int open = -1;	//keeps track of box open(1) or closed(-1)
	char *currentOpenBox;
	char *response = "";
	while(killFlag == 0){

		char cmd[256] = {0};
		char arg0[256] = {0};
		char arg1[256] = {0};
		//receives command from client and puts it in buffer
		msgLength = recv(arguments->clientSocket, buffer, 1024, 0);
		//removes \n included in msgLength
		msgLength = msgLength - 1;
		buffer[msgLength] = '\0';

		int i = 0;
		for(i=0; i < msgLength; i++){
			if(buffer[i] == ' ' || buffer[i] == '!'){
				break;
			}
			cmd[i] = buffer[i];
		}
		int k = 0;
		if(i != msgLength){
			int j;
			for(j=i+1; j < msgLength; j++){	
				if(buffer[j] == '!'){
					break;
				}
				arg0[k] = buffer[j];
				k++;
			}
			arg0[k] = '\0';
			if(j != msgLength){
				int l;
				int m = 0;
				for(l = j+1; l < msgLength; l++){	//msgLength-1 to exclude trailing ! in put cmd
					arg1[m] = buffer[l];
					m++;
				}
				arg1[m] = '\0';
			}
		}
		if(strcmp(cmd, "GDBYE") == 0){	//E.1
			//client expects no response text from server, since the server should close the connection
			//The server should close the client's open message box, if the user had one and did not close it before disconnecting
			if(open == 1){
				closeBox(arguments->clientSocket, currentOpenBox);
				free(currentOpenBox);
			} 
			printf("%02d%02d %d Dec %s GDBYE\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID);
			printf("%02d%02d %d Dec %s disconnected\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID);
			free(arguments);
			return;
		}else if(strcmp(cmd, "CREAT") == 0){	//E.2
			pthread_mutex_lock(&lock);
			int valid = validName(arg0, k);
			if(valid == -2){	//Incorrect format
				response = "ER:WHAT?";
				send(arguments->clientSocket, response, strlen(response), 0);
			}else if(valid == -1 || valid == 0){	//Message Box already exists
				response = "ER:EXIST";
				send(arguments->clientSocket, response, strlen(response), 0);
			}else{
				create(arguments->clientSocket, arg0);

				response = "CREAT\n";
				send(arguments->clientSocket, "OK!\n", 4, 0);
			}
			pthread_mutex_unlock(&lock);
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else if(strcmp(cmd, "OPNBX") == 0){	//E.3
			pthread_mutex_lock(&lock);
			int valid = validName(arg0, k);
			if(valid == -2){
				response = "ER:WHAT?";
				send(arguments->clientSocket, response, strlen(response), 0);	
			}else if(valid == -1){	//this means message box exists but it is open by another client
				response = "ER:OPEND";
				send(arguments->clientSocket, response, strlen(response), 0);
			}else if(valid == 0){	//Good, this means message box exists and its closed 
				if(open == 1){
					closeBox(arguments->clientSocket, currentOpenBox);
					free(currentOpenBox);
					response = "CLSBX\n";
					printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
				}
				openBox(arguments->clientSocket, arg0);
				response = "OPNBX\n";
				open = 1;
				currentOpenBox = malloc(strlen(arg0)+1);
				strcpy(currentOpenBox, arg0);
				send(arguments->clientSocket, "OK!\n", 4, 0);
			}else{	//this means message box does not exist
				response = "ER:NEXST";
				send(arguments->clientSocket, response, strlen(response), 0);
			}
			pthread_mutex_unlock(&lock);
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else if(strcmp(cmd, "NXTMG") == 0){	//E.4
			if(open == 1){
				Message *message;
				if((message = next(arguments->clientSocket, currentOpenBox)) == NULL){
					response = "ER:EMPTY";
					send(arguments->clientSocket, response, strlen(response), 0);
				}else{
					char reply[1024] = {0};
					char number[15] = {0};
					sprintf(number, "%d", message->length);
					int n = 0;
					while(number[n] != '\0'){
						n++;
					}
					number[n] = '!';
					strcpy(reply, "OK!");
					strcpy(&reply[3], number);
					strcpy(&reply[3+strlen(number)], message->text);
					send(arguments->clientSocket, reply, strlen(reply), 0);
					free(message->text);
					free(message);
					response = reply;
				}
			}else{
				response = "ER:NOOPN";
				send(arguments->clientSocket, response, strlen(response), 0);
			}
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else if(strcmp(cmd, "PUTMG") == 0){	//E.5
			if(open == 1){
				int format = putFormatCheck(arg0, arg1);
				if(format == -1){
					response = "ER:WHAT?";
					send(arguments->clientSocket, response, strlen(response), 0);
				}else{
					put(arguments->clientSocket, atoi(arg0), arg1, currentOpenBox);
					char reply[1024] = {0};
					strcpy(reply, "OK!");
					strcpy(&reply[3], arg0);
					send(arguments->clientSocket, reply, strlen(reply), 0);
					response = reply;
				}
			}else{
				response = "ER:NOOPN";
				send(arguments->clientSocket, response, strlen(response), 0);
			}
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else if(strcmp(cmd, "DELBX") == 0){	//E.6
			pthread_mutex_lock(&lock);
			int valid = validName(arg0, k);
			if(valid == -2){
				response = "ER:WHAT?";
				send(arguments->clientSocket, response, strlen(response), 0);
			}else if(valid == -1){
				response = "ER:OPEND";
				send(arguments->clientSocket, response, strlen(response), 0);
			}else if(valid == 0){
				if((delete(arguments->clientSocket, arg0)) == -1){
					response = "ER:NOTMT";
					send(arguments->clientSocket, response, strlen(response), 0);
				}else{
					response = "DELBX";
					send(arguments->clientSocket, "OK!\n", 4, 0);
				}
			}else{
				response = "ER:NEXST";
				send(arguments->clientSocket, response, strlen(response), 0);
			}
			pthread_mutex_unlock(&lock);
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else if(strcmp(cmd, "CLSBX") == 0){	//E.7
			if(open == 1){
				if(strcmp(arg0, currentOpenBox) == 0){
					closeBox(arguments->clientSocket, currentOpenBox);
					free(currentOpenBox);
					open = -1;
					response = "CLSBX\n";
					send(arguments->clientSocket, "OK!\n", 4, 0);
				}else{
					response = "ER:NOOPN";
					send(arguments->clientSocket, response, strlen(response), 0);
				}
			}else{
				response = "ER:NOOPN";
				send(arguments->clientSocket, response, strlen(response), 0);
			}
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}else{
			response = "ER:WHAT?";
			send(arguments->clientSocket, response, strlen(response), 0);
			printf("%02d%02d %d Dec %s %s\n", ptm->tm_hour, ptm->tm_min, ptm->tm_mday, clientID, response);
		}
	}
	return;
}

void clientHandler(int serverSocket, struct sockaddr_in addr, int addrlen){	//args contains: serverSocket, serverAddress, addressSize
	struct sockaddr *serverAddr = (struct sockaddr*)&addr;	//*E*	
	struct itimerval timer;
	signal(SIGALRM, sighandler);
	timer.it_value.tv_sec = 100;
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 100;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	int clientSocket;
	pthread_mutex_init(&lock, NULL);
	pthread_t threadID;
	while(killFlag == 0){
		if((clientSocket = accept(serverSocket, serverAddr, (socklen_t*)&addrlen)) < 0){	//*E*
			perror("Accept");
			exit(EXIT_FAILURE);
		}
		Node *threadNode = (Node*)malloc(sizeof(Node));
		commandArgs *args = (commandArgs*)malloc(sizeof(int));
		args->clientSocket = clientSocket;
		if((pthread_create(&threadID, NULL, (void*)commandHandler, (void*)args)) != 0){
			perror("Listener");
			exit(EXIT_FAILURE);
		}
		threadNode->threadID = threadID;
		threadNode->clientSocket = clientSocket;
		//threadNode list used to keep track of individual client requests and more importantly to join all threads at the end
		if(threadHead == NULL){
			threadHead = threadNode;
		}else{
			Node *ptr;
			for(ptr = threadHead; ptr->next != NULL; ptr = ptr->next){	//don't think it is necessary to put at the end of list
			}
			ptr->next = threadNode;	
		}
	}
	pthread_mutex_destroy(&lock);
	pthread_detach(threadHead->threadID);
	return;
}

int main(int argc, char** argv){
	if(argc != 2){
		printf("Error: Only port number required!\n");
		return 0;
	}
	
	int serverSocket;
	struct sockaddr_in serverAddr;
	int addrlen = sizeof(serverAddr);
	int option = 1;	//boolean value used to specify if bind() should allow reuse of local addresses (USED in conjunction with SO_REUSEADDR)

	//**Create socket**
	if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}
	
	//**Set rules for new socket**
	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))){	//setsockopt returns -1 if error
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	//**Set socket ruleset**
	//Address family = Internet
	serverAddr.sin_family = AF_INET;//Sockets created with the socket()function are inititally unnamed they are identified by their address family
	//INADDR_ANY is used when you don't need to bind a socket to a specific IP
	//When you use this value as the address when calling bind(), the socket accepts connections to all IPs machine
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	//Set port number, using htons function to use proper byte order
	serverAddr.sin_port = htons(atoi(argv[1]));

	//**bind socket to given port**
	if(bind(serverSocket, (struct sockaddr*)&serverAddr, addrlen) == -1){	//bind() shall return 0; otherwise, -1 shall be returned for error
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}
	
	//**mark socket as listening**
	if(listen(serverSocket, 5) < 0){
		perror("Listen");
		exit(EXIT_FAILURE);
	}
	printf("listening...\n");
	
	clientHandler(serverSocket, serverAddr, addrlen);
	return 0;
}
