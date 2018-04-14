#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

char history[HISTORY_DEPTH][COMMAND_LENGTH]; //global variable
int count = 0;
int front = -1;

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[])
{
	int token_count = 0;
	_Bool in_token = false;
	int num_chars = strnlen(buff, COMMAND_LENGTH);
	for (int i = 0; i < num_chars; i++) {
		switch (buff[i]) {
		// Handle token delimiters (ends):
		case ' ':
		case '\t':
		case '\n':
			buff[i] = '\0';
			in_token = false;
			break;

		// Handle other characters (may be start)
		default:
			if (!in_token) {
				tokens[token_count] = &buff[i];
				token_count++;
				in_token = true;
			}
		}
	}
	tokens[token_count] = NULL;
	return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of tokens).
 * in_background: pointer to a boolean variable. Set to true if user entered
 *       an & as their last token; otherwise set to false.
 */
void addHistory(char *buff){

	front = (front + 1) % HISTORY_DEPTH;
	strcpy(history[front], buff);
	count++;
}

void runHistory(char* buff){


	if(buff[1] == '!'){ //run previous command
		if(count == 0){
			write(0,"ERROR: Unknown History Command" ,strlen("ERROR: Unknown History Command"));
		 	write(0, "\n", strlen("\n"));				
			strcpy(buff, "");//do nothing	maybe?		
		}
		else{
			strcpy(buff, history[(count-1) % HISTORY_DEPTH]);
		 	write(0, history[(count-1) % HISTORY_DEPTH], strlen(history[(count-1) % HISTORY_DEPTH]));
		 	write(0, "\n", strlen("\n"));					
		}
	}

	else{ //get numbered history
		int historyNum = atoi(buff + 1); //move the buffer pointer forward

		if(historyNum == 0){//argument is not a valid number or is out of range
			write(0,"ERROR: Unknown History Command" ,strlen("ERROR: Unknown History Command"));
			write(0,"\n",strlen("\n"));
			strcpy(buff, ""); //do nothing maybe?

		 }

		else if((count < HISTORY_DEPTH && historyNum > count)||(count >= HISTORY_DEPTH && (historyNum < ((count+1)-HISTORY_DEPTH) || historyNum > count))){
			//If count is less than 10 and the user wants to retreive a command number greater than count
			//or if the count is greater than history depth and the user wants to retreive a command number out of the range of the history

			write(0,"ERROR: Unknown History Command" ,strlen("ERROR: Unknown History Command"));
			write(0,"\n",strlen("\n"));
			strcpy(buff, ""); //do nothing maybe?

		}
		 else{
		 	write(0, history[(historyNum-1) % HISTORY_DEPTH], strlen(history[(historyNum-1) % HISTORY_DEPTH]));
		 	write(0, "\n", strlen("\n"));	
			strcpy(buff, history[(historyNum-1) % HISTORY_DEPTH]);	
		 }

	}
}


void read_command(char *buff, char *tokens[], _Bool *in_background)
{
	*in_background = false;

	// Read input
	int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);

	if ((length < 0) && (errno !=EINTR)) {
		perror("Unable to read command from keyboard. Terminating.\n");
		exit(-1);
	}

	// Null terminate and strip \n.
	buff[length] = '\0';
	if (buff[strlen(buff) - 1] == '\n') {
		buff[strlen(buff) - 1] = '\0';
	}

	if(buff[0] == '!'){

		runHistory(buff);
	}
	if(buff[0] != '\0'){

		addHistory(buff);
	}
	

	// Tokenize (saving original command string)
	int token_count = tokenize_command(buff, tokens);
	if (token_count == 0) {
		return;
	}

	// Extract if running in background:
	if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
		*in_background = true;
		tokens[token_count - 1] = 0;
	}
}




void printHistory(){

	char commandNum[1024];
	int len;

	if(count < HISTORY_DEPTH){
		for(int i = 0;i<count;i++){

			len = sprintf(commandNum,"%d 		%s", i+1, history[i]);

			write(STDOUT_FILENO, commandNum, len);
			write(STDOUT_FILENO, "\n", strlen("\n"));
			
		}
	}
	else{
		int start = front+1;
		int num = (count+1) - HISTORY_DEPTH;
		for(int j = start; j < (start+HISTORY_DEPTH); j++){

			len = sprintf(commandNum,"%d 		%s", num, history[j % HISTORY_DEPTH]);

			write(STDOUT_FILENO, commandNum, len);
			write(STDOUT_FILENO, "\n", strlen("\n"));
			num++;
		}
	}
}


void getdir(){

	char buff[COMMAND_LENGTH];
	getcwd(buff, sizeof(buff));
	write(STDOUT_FILENO, buff , strlen(buff));
}

void handle_SIGINT(){
	write(STDOUT_FILENO, "\n", strlen("\n"));
	printHistory();
	//write(0,"hello", strlen("hello"));
	//return;
}
/**
 * Main and Execute Commands
 */
int main(int argc, char* argv[])
{
	//setup the signal handler

	char input_buffer[COMMAND_LENGTH];
	char *tokens[NUM_TOKENS];

	struct sigaction handler;
	handler.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &handler, NULL);

	while (true) {
		strcpy(input_buffer, ""); //clear the buffer if there is a signal interrupt
		// Get command
		// Use write because we need to use read() to work with
		// signals, and read() is incompatible with printf().
		getdir();
		write(STDOUT_FILENO, "> ", strlen("> "));
		_Bool in_background = false;
		read_command(input_buffer, tokens, &in_background);

		// DEBUG: Dump out arguments:
		// for (int i = 0; tokens[i] != NULL; i++) {
		// 	write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
		// 	write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
		// 	write(STDOUT_FILENO, "\n", strlen("\n"));
		// }
		// if (in_background) {
		// 	write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
		// }


		if(tokens[0] != NULL && strcmp(tokens[0], "exit") == 0){

			return 0; //is exit or return better?
		}
		else if(tokens[0] != NULL && strcmp(tokens[0], "pwd") == 0){ //works without it?
			getdir();
			write(STDOUT_FILENO, "\n", strlen("\n"));
		}

		else if(tokens[0] != NULL && strcmp(tokens[0], "cd") == 0){ //support for quotes? Also for empty cd
			if(tokens[1] == NULL){
				; //do nothing
			}

			else if(chdir(tokens[1]) == -1){
			 	perror("Invalid directory");

			 }
		}
		else if(tokens[0] != NULL && strcmp(tokens[0], "history") == 0){
			printHistory();
		}

		else{

			pid_t pid;

			pid = fork();

			if(pid < 0){
				perror("Error forking");
				exit(-1);
			}

			else if(pid == 0){ //child process

				if(execvp(tokens[0], tokens) == -1){ //run command and check for error
					perror("Unknown command");
				}
				exit(0);
			}

			else{ //parent, check if in_background is true
				if(!in_background){
					waitpid(pid, NULL, 0);
				}

			}
			
		}


		/**
		 * Steps For Basic Shell:
		 * 1. Fork a child process
		 * 2. Child process invokes execvp() using results in token array.
		 * 3. If in_background is false, parent waits for
		 *    child to finish. Otherwise, parent loops back to
		 *    read_command() again immediately.
		 */

		// Cleanup any previously exited background child processes
		// (The zombies)
		while (waitpid(-1, NULL, WNOHANG) > 0)
			; // do nothing.
	}
	return 0;
}
