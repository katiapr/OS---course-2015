
///*************************///
//ex2:mini_shell
//Description:this program create a mini shell
//that can read and run a bush commands include prev ex1.
//for more information-README.pdf
//Author:Katia Prigon
//ID:322088154
//Date: 12/4/15
///************************///
#include <stdio.h>//use for input/output.
#include <string.h>//use for strings.
#include <unistd.h>//use for system call wrapper functions such as fork(),chdir()
#include <stdlib.h>// use for defines several general purpose functions and dynamic memory management.
#include <sys/wait.h>//use for wait
#include <sys/types.h>//use for pid_t
#include <signal.h>//use for signals
//****from ex1****//
//prints commands and arguments

//get input from line commands
char** readTokens(FILE* stream);
//free the memory that was allocated
void freeTokens(char** tokens);
//printing the user name and computer name
void printPrompt();
//**************************************//
void newProcess(char** command);//creat a child
void signalHandler(int signal);//catch SIGINT(CTRL+C)
void cdComCatch(char** command);//when user wrote 'cd' + path direction.
int findWrongArg(char** command);//find '$' and returns error massage and num of arg.
pid_t child_pid;
pid_t ch_pid;
int numOfwords;

int main(void)
{
	char** str;//input string

	while(0==0)
	{
		printPrompt();
		str = readTokens(stdin);

		if(str == NULL)//something went wrong or user wrote 'exit'
			continue;

		if(findWrongArg(str) != -1)
			fprintf(stderr,"Our minishell cannot handle the $ sign for argument %d\n"
					,findWrongArg(str));

		else
		{
			if(strcmp(str[0],"cd")==0)
				cdComCatch(str);
			else newProcess(str);
		}
		freeTokens(str);

	}

	return EXIT_SUCCESS;
}
void printPrompt()
{
	char host[100];
	if(gethostname(host,99)!=0)
	{
		printf("ERROR getting hostName\n");
	}
	printf("%s@%s$:",getlogin(),host);//user's data
}
char** readTokens(FILE* stream)
{
	char str[510],copystr[510];
	char* ptr;
	int num = 0,i = 0,length = 0;
	char** words;

	if(fgets(str,510,stream)==NULL)//read input
		return NULL;
	if(str[0] == '\n' ||str[0] == '\0'|| str[0]== ' ')
		return NULL;
	strcpy(copystr,str);
	ptr = strtok(str," \n");
	while(ptr!=NULL)
	{
		num++;//counting
		ptr = strtok(NULL," \n");//next expression
	}
	numOfwords = num;
	words = (char**)malloc(numOfwords*sizeof(char*)+1);

	if(words == NULL)
	{
		freeTokens(words);
		fprintf(stderr,"memory allocation fail!\n");
		exit(EXIT_FAILURE);
	}
	ptr = strtok(copystr," \n");
	while(ptr!=NULL)
	{
		length = strlen(ptr);
		//allocation memory for each word
		words[i] = (char*)malloc(length*sizeof(char));
		if(words[i]==NULL)
		{
			freeTokens(words);
			fprintf(stderr,"memory allocation fail!\n");
			exit(EXIT_FAILURE);
		}
		strcpy(words[i],ptr);
		ptr = strtok(NULL," \n");
		i++;
	}
	words[numOfwords] = NULL;



	if(strcmp(words[0],"exit") == 0 && words[1] == NULL)//develop
	{
		freeTokens(words);
		exit(EXIT_FAILURE);
	}
	return words;
}

void freeTokens(char** tokens)
{
	if(tokens == NULL)
		return;
	int i = 0;
	while(tokens[i] != NULL)
	{
		free(tokens[i]);
		i++;
	}
	free(tokens[i]);
	free(tokens);
}

void newProcess(char** command)//create a child
{
	int status = 0;
	child_pid = fork();
	signal(SIGINT,signalHandler);//overwriting dealing with SIGINT
	if(child_pid >= 0)
	{
		if(child_pid == 0)
		{
			ch_pid = getpid();
			if(execvp(command[0],command)<0)
				fprintf(stderr,"There is no such a command\n");

		}
		else//child_pid>0 father process
		{
			child_pid=wait(&status);//father waits his child process will end
		}
	}
	else //child_pid <0 //error at create the process
	{
		fprintf(stderr,"**ERROR -fork fail- **\n");
		exit(EXIT_FAILURE);
	}
}
void signalHandler(int sign)
{
	kill(ch_pid,SIGINT);
	signal(SIGINT, SIG_IGN);
	return;
}
void cdComCatch(char** command)
{

	if(command[1] == NULL)
		chdir(getenv("HOME"));
	else if(numOfwords == 2)
	{
		if(chdir(command[1]) == -1)
			fprintf(stderr,"there was an error at path\n");
		else  fprintf(stderr,"too few arguments to command 'cd'\n");
	}

}
int findWrongArg(char** command)
{
	int i = 0 ;
	for(i = 0; command[i]!= NULL; i++)
	{
		if(command[i][0] == '$')
			return i+1;
	}
	return -1;//there is no '$' at command line
}
