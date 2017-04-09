///*************************///
//ex1:mini shell bash
//Description:this program create a mini shell 
//that can run a bush commands.
//Author:Katia Prigon
//ID:322088154
//Date: 26/3/15
///************************///
#include <unistd.h>//for using gethostname() and getlogin()
#include <stdio.h>//input/output
#include <stdlib.h>//for malloc and free
#include <string.h>//for strings functions
//prints commands and arguments
void printTokens(const char** tokens);
//get input from line commands
char** readTokens(FILE* stream);
//free the memory that was allocated
void freeTokens(char** tokens);
//printing the user name and computer name
void printPrompt();


int main(void)
{
	char** str;//input string
	printPrompt();

	while(0==0)
	{
	str = readTokens(stdin);
	if(str == NULL)//something went wrong or user wrote 'exit'
		exit(-1);
	printTokens((const char**)str);
	
	freeTokens(str);
	}

	return 0;
}
void printPrompt()
{
	char host[100];
	if(gethostname(host,99)!=0)
	{
		printf("ERROR getting hostName\n");
	}
	printf("%s@%s$ \n",getlogin(),host);//user's data
}

void printTokens(const char** tokens)
{
	int  i = 1,j = 0;

	printf("Executable: %s\n", tokens[0]);//command

	while(tokens[i]!=NULL)//arguments
	{
		if(tokens[i][0]=='$')//first char is '$'
		{
			printf("Ard %d: with $ ",i);
			for(j=1; tokens[i][j]!='\0';j++)
				printf("%c",tokens[i][j]);
		}
		else printf("Arg %d: %s",i,tokens[i]);
		i++;
		printf("\n");
	}
}

char** readTokens(FILE* stream)
{
	char str[510],copystr[510];
	char* ptr;
	int numOfwords = 0,i = 0,length = 0;
	char** words;

	if(fgets(str,510,stream)==NULL)//read input
		return NULL;

	strcpy(copystr,str);
	ptr = strtok(str," \n");
	while(ptr!=NULL)
	{
		numOfwords++;//counting
		ptr = strtok(NULL," \n");//next expresion
	}
	words = (char**)malloc(numOfwords*sizeof(char*)+1);

	if(words == NULL)
	{
		freeTokens(words);
		return NULL;
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
			return NULL;
		}
		strcpy(words[i],ptr);
		ptr = strtok(NULL," \n");
		i++;
	}
	words[numOfwords] = NULL;
	if(strcmp(words[0],"exit") == 0)
	{
		freeTokens(words);
		return NULL;
	}
	return words;
}

void freeTokens(char** tokens)
{
	int i = 0;
	while(tokens[i] != NULL)
	{
		free(tokens[i]);
		i++;
	}
	free(tokens[i]);
	free(tokens);
}

