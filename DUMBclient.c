#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

int checkError(int sock, int command);
void commands(int sockfd);

void commands(int sockfd){
	int work = 0;
	//send(sockfd, "HELLO\n", 5, 0);
	char buffer[200];
	recv(sockfd, buffer, 200, 0);
	printf(buffer);
	int checkEr = 0;

	while(work == 0){
		char commandInput[256] = {0};
		char mailboxName[256] = {0};
		char serverCommands[1024] = {0};
		char message[256] = {0};
		
		printf("Please type in command\n> ");
		scanf("%s", commandInput);

		if(strcmp("quit", commandInput) == 0){
			strcpy(serverCommands, "GDBYE\n");
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			work = 1;
			
		}else if(strcmp("create", commandInput) == 0){ // create message
			printf("What is the name of the mailbox you want to create?\n");
			write(1, "create:> ", 9);
			read(0,mailboxName,sizeof(mailboxName));
			strcpy(serverCommands, "CREAT!");
			strcpy(&serverCommands[6], mailboxName);
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd, 1);
			
			if(checkEr == 1){
				printf("Success! Message box created.\n");
			}
		}else if(strcmp("open", commandInput) == 0){ // open mailbox
			printf("What is the name of the mailbox you want to open?\n");
			write(1, "open:> ", 7);
			read(0, mailboxName, sizeof(mailboxName));
			strcpy(serverCommands, "OPNBX!");
			strcpy(&serverCommands[6], mailboxName);
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd,2);
			if(checkEr == 1){
				printf("Success! Message box is now open.\n");
			}
		}else if(strcmp("next", commandInput) == 0){ // get next message
			strcpy(serverCommands,"NXTMG!");
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd, 3); //
		}else if(strcmp("put", commandInput)== 0){ // put message
			printf("What message do you want to put?\n");
			write(1, "put:> ", 6);
			read(0, message, sizeof(message));
			int sizeOfMessage = strlen(message)-1;
			printf(message);
			char number [15] = {0};
			sprintf(number,"%d", sizeOfMessage); //converts num to string
			int j =0;
			while(number[j] != '\0'){
				j++;
			}
			number[j] = '!';
			strcpy(serverCommands, "PUTMG!");
			strcpy(&serverCommands[6],number);
			strcpy(&serverCommands[6+strlen(number)], message);
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd, 4);
		}else if(strcmp("delete", commandInput) == 0){ // delete mailbox
			printf("Which mailbox do you want to delete?\n");
			write(1, "delete:> ", 9);
			read(0, mailboxName, sizeof(mailboxName));
			strcpy(serverCommands, "DELBX!");
			strcpy(&serverCommands[6], mailboxName);
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd, 5);
			if(checkEr == 1){
				printf("Success! Message box deleted.\n");
			}
		}else if(strcmp("close", commandInput) == 0){ //close mailbox
			printf("Which mailbox do you want to close?\n");
			write(1, "close:> ",8);
			read(0, mailboxName, sizeof(mailboxName));
			strcpy(serverCommands, "CLSBX!");
			strcpy(&serverCommands[6], mailboxName);
			send(sockfd, serverCommands, strlen(serverCommands), 0);
			checkEr = checkError(sockfd, 6);
			if(checkEr == 1){
				printf("Success! Message box deleted.\n");
			}
		} else if(strcmp("help", commandInput) == 0){
			printf("Here are the commands you can type\n");
			printf("quit\ncreate\ndelete\nopen\nclose\nnext\nput\n");
		}else{
			printf("Unknown command. Please try again\n");
		}
		
	}
	return;
}

void separateStrings(char* messagefromServer, int command){
	char length[10] = {0};
	char message[2000] = {0};
	int i;
	int j;
	int k = 0;
	int l = 0;
	for(i=3; i < strlen(messagefromServer); i++){
		if(messagefromServer[i] == '!'){
			for(j=i+1; j < strlen(messagefromServer); j++){
				message[l] = messagefromServer[j];
				l++;
			}
			message[l] = '\0';
			break;
		}
		length[k] = messagefromServer[i];
		k++;
	}
	length[k] = '\0';
	if(command == 3){
		printf("Okay, message of length: %s says: %s\n",  length, message);
	}	//next
	if(command == 4){
		printf("Command successfully performed, message of length %s inserted\n", length);	//put
	}
	return;
}

int checkError(int sock, int command){
	char messagefromServer[2000] = {0};
	int msgLength = recv(sock, messagefromServer, 2000, 0);
	messagefromServer[msgLength] = '\0';

	if(messagefromServer[0] == 'O' && command == 3){
		separateStrings(messagefromServer, command);
		return 0;	
	}else if(messagefromServer[0] == 'O' && command == 4){	//successful nxt, expects OK!arg0!msg
		separateStrings(messagefromServer, command);
		return 0;
	}else if(messagefromServer[0] == 'O'){	//successful put, expects OK!arg0
		return 1;
	}else{
		if(command==1){
			if(strcmp("ER:EXIST", messagefromServer) == 0){ // create
				printf("Error: Username already exists\n");
				return 0;
			}else{
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
		if(command == 2){
			if (strcmp("ER:NEXST", messagefromServer)== 0){ //openbox
				printf("Box does not exist, cannot be opened\n");
				return 0;
			}else if(strcmp("ER:OPEND", messagefromServer)==0){
				printf("Box already open, cannot be opened\n");
				return 0;
			}else{
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
		if(command == 3){
			if (strcmp("ER:EMPTY", messagefromServer)== 0){//nxtmg
				printf("No messages left in the message box\n");
				return 0;
			}else if(strcmp("ER:NOOPN", messagefromServer)==0){
				printf("Message box not open or message box does not exist\n");
				return 0;
			}else{
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
		if(command == 4){
			if(strcmp("ER:NOOPN", messagefromServer)==0){ //putmg
				printf("Message box not open or message box does not exist\n");
				return 0;
			}else{
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
		if(command == 5){ 
			if(strcmp("ER:NEXST", messagefromServer)==0){ //delbx
				printf("Message box does not exist\n");
				return 0;
			}else if(strcmp("ER:OPEND", messagefromServer)==0){ 
				printf("Message box currently opened, cannot delete while opened\n");
				return 0;
			}else if(strcmp("ER:NOTMT", messagefromServer)==0){ 
				printf("Message box still has messages, cannot delete\n");
				return 0;
			}else{
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
		if(command == 6){
			if(strcmp("ER:NOOPN", messagefromServer)==0){ //clsbx
				printf("No open message box, cannot close\n");
				return 0;
			}else {
				printf("Error, your message is broken or malformed\n");
				return 0;
			}
		}
	}
	return 0;
}


int main(int argc, char** argv){

        
 
        //check for error, if error occurs more than 3 times, shut down
	if(argc < 3){
	        printf("Error! Type in correct number of arguments, IP address/ hostname and port number\n");
	        return -1;
	}
	struct sockaddr_in address;
	
	//int portNum = atoi(argv[2]);
	int sockfd = 0;
	
	//create socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
	        printf("Error creating socket\n");
	        return -1;
	}
	
	//initializing server address
	
	address.sin_family = AF_INET;
	address.sin_port = htons(atoi(argv[2]));
	if(inet_pton(AF_INET, argv[1], &address.sin_addr)<=0){
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}
	
	
	//check for connection errors
	int wrong = 0;
	while(wrong != 3){
		if(connect(sockfd, (struct sockaddr*)&address, sizeof(address))<0){
			printf("connection failed\n");
			wrong++;
			
		}else{
			printf("Connection successful!\n");
			break;
		}
	}
	if(wrong == 3) {
		printf("Failed too many times, Goodbye!\n");
		return -1;
	}

	send(sockfd, argv[1], strlen(argv[1]), 0);
	commands(sockfd);
	
	return 0;
}
