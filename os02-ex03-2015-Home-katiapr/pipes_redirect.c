///*************************///
//katia prigon  -322088154 katiapr - katiapr89@gmail.com
//ex3:pipes_redirect
//Description:this program is extension for commands of mini shell
//it can implement commands of i\o redirections and pipes,
//it create processes, read and run a bush commands include prev ex1 & ex2.
//for more information-README.pdf
//Date: 7/6/15
///************************///
#include <stdio.h>//use for input/output.
#include <string.h>//use for strings.
#include <unistd.h>//use for system call wrapper functions such as fork(),chdir()
#include <stdlib.h>// use for defines several general purpose functions and dynamic memory management.
#include <sys/wait.h>//use for wait
#include <sys/types.h>//use for pid_t
#include <signal.h>//use for signals
#include <fcntl.h>//use for oflags fd
#include <math.h>//math functions
#define EXIT_ERROR 255
#define SUCCESS 0
#define NUM_OF_SGN 4
#define FOUND_PIPE  10
#define FOUND_RED  11
#define EMPTY -1
#define SGN_ARRAY_LEGHT 5
enum {
	RIGHT = 0, LEFT, DOUBLE, PIPE
};
typedef enum {
	FALSE, //0
	TRUE //1
} bool;
//bool checkCommand(char* command, int buf);
typedef struct {
	int index; //empty
	bool empty;
	char sgn[2];
} sgn_type;
//global variables
int numOfwords;
pid_t child_pid;
int status = 0;
int temp = 0;

char** readTokens(FILE* stream);
void freeTokens(char** tokens);
void printPrompt();
//**************************************//
void simpleProcess(char** command); //creat a child
void signalHandler(int signal) {
} //catch SIGINT(CTRL+C)
void cdComCatch(char** command); //when user wrote 'cd' + path direction.
int findWrongArg(char** command); //find '$' and returns error massage and num of arg.
void general(char** command); //sort and decide {redirection,pipe,no
void pipeCommand(char** command, char** left, char** right, sgn_type* type); // | include redirections
//redirections:
void rightCommand(char** command, int right); // >
void leftCommand(char** command, int left); // <
void doubleCommand(char** command, int doub); // >>
void left_right_double_command(char** command, sgn_type* type); // < > or < >>

//copy array:
//copying left side of command
char** left_side(char** command, int end);
//copying right side of command
char** right_side(char** command, int stast);
//copying middle side of command
char** middle(char** command, int start, int end);

int main()
{
	char** str; //input string
	signal(SIGINT, signalHandler); //overwriting dealing with SIGINT

	while (TRUE)
	{
		printPrompt();
		str = readTokens(stdin);

		if (str == NULL) //something went wrong or user wrote 'exit'
			continue;

		if (findWrongArg(str) != -1)
			fprintf(stderr,
					"minishell cannot handle the $ sign for argument %d\n",
					findWrongArg(str));
		else
		{
			if (strcmp(str[0], "cd") == 0)
				cdComCatch(str);
			else
				general(str);
		}
		freeTokens(str);

	}

	return EXIT_SUCCESS;
}

void printPrompt()
{
	char host[100];
	if (gethostname(host, 99) != 0)
	{
		printf("ERROR getting hostName\n");
	}
	printf("%d %s@%s$:", WEXITSTATUS(status), getlogin(), host); //user's data
}
char** readTokens(FILE* stream)
{
	char str[510], copystr[510];
	char* ptr;
	int num = 0, i = 0, length = 0;
	char** words;

	if (fgets(str, 510, stream) == NULL) //read input
		return NULL;
	if (str[0] == '\n' || str[0] == '\0')
		return NULL;
	strcpy(copystr, str);
	ptr = strtok(str, " \n");
	if (ptr == NULL)
		return NULL;
	while (ptr != NULL)
	{
		num++; //counting
		ptr = strtok(NULL, " \n"); //next expression
	}
	numOfwords = num;
	words = (char**) malloc((numOfwords + 1) * sizeof(char*));

	if (words == NULL)
	{
		freeTokens(words);
		fprintf(stderr, "memory allocation fail!\n");
		exit(EXIT_FAILURE);
	}
	ptr = strtok(copystr, " \n");
	while (ptr != NULL)
	{
		length = strlen(ptr);
		//allocation memory for each word
		words[i] = (char*) malloc((length + 1) * sizeof(char));
		if (words[i] == NULL)
		{
			freeTokens(words);
			fprintf(stderr, "memory allocation fail!\n");
			exit(EXIT_FAILURE);
		}
		strcpy(words[i], ptr);
		ptr = strtok(NULL, " \n");
		i++;
	}
	words[numOfwords] = NULL;

	if (strcmp(words[0], "exit") == 0 && words[1] == NULL)
	{
		freeTokens(words);
		exit(EXIT_FAILURE);
	}
	return words;
}
void freeTokens(char** tokens)
{
	if (tokens == NULL)
		return;
	int i = 0;
	while (tokens[i] != NULL)
	{
		free(tokens[i]);
		i++;
	}
	free(tokens[i]);
	free(tokens);
}
void simpleProcess(char** command) //create a child
{
	child_pid = fork();
	if (child_pid >= 0)
	{
		if (child_pid == 0)
		{
			if (execvp(command[0], command) < 0)
			{
				perror("There is no such a command");
				freeTokens(command);
				exit(EXIT_ERROR);
			}

		}
		else //child_pid>0 father process
			child_pid = wait(&status); //father waits his child process will end

	}
	else //child_pid <0 //error at create the process
	{
		perror("fork faild\n");
		exit(EXIT_FAILURE);
	}
}
void cdComCatch(char** command)
{

	if (command[1] == NULL)
		chdir(getenv("HOME"));
	else if(command[2] == NULL)
	{
		if (chdir(command[1]) == -1)
		{
			perror("no such directory");
			return;
		}
		return;
	}
}
int findWrongArg(char** command)
{
	int i = 0;
	for (i = 0; command[i] != NULL; i++)
	{
		if (command[i][0] == '$')
			return i + 1;
	}
	return -1; //there is no '$' at command line
}
//this func  get the command line and direct to relevant situations
void general(char** command)
{
	sgn_type type[SGN_ARRAY_LEGHT];
	int i, j;
	int red_pip = -1;
	strcpy(type[RIGHT].sgn, ">");
	strcpy(type[LEFT].sgn, "<");
	strcpy(type[DOUBLE].sgn, ">>");
	strcpy(type[PIPE].sgn, "|");
	strcpy(type[SGN_ARRAY_LEGHT - 1].sgn, "\0");
	for (i = 0; i < SGN_ARRAY_LEGHT; i++)
	{
		type[i].empty = TRUE;
		type[i].index = -1;
	}
	bool flag = FALSE, isPipe = FALSE;
	int count = 0;
	if (type == NULL)
		return;

	for (i = 0; command[i] != NULL; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (strcmp(command[i], type[j].sgn) == 0)
			{
				if (type[j].empty == TRUE) {
					if (strcmp(type[j].sgn, "|") == 0)
					{
						red_pip = FOUND_PIPE;
						isPipe = TRUE;
					} else if (!isPipe && !(strcmp(type[j].sgn, "|") == 0)) {
						red_pip = FOUND_RED;
						count++;
					}
					type[j].index = i;
					type[j].empty = FALSE;
					flag = TRUE;
				}
				else
				{
					fprintf(stderr, "%s cannot use more then one time\n",
							type[j].sgn);
					exit(EXIT_ERROR);
				}
			}

		}
	}
	if (!flag) //simple command
		simpleProcess(command);
	else if (type[RIGHT].index == 0 || type[LEFT].index == 0
			|| type[DOUBLE].index == 0 || type[RIGHT].index == numOfwords - 1
			|| type[LEFT].index == numOfwords - 1
			|| type[DOUBLE].index == numOfwords - 1)
	{
		fprintf(stderr, "bash: syntax error near unexpected token\n");
		return;
	}
	switch (red_pip)
	{
	case FOUND_PIPE:
		//command pipe
		if (type[PIPE].index >= 1 && command[type[PIPE].index + 1] != NULL)
		{
			char** left = left_side(command, type[PIPE].index);
			char** right = right_side(command, type[PIPE].index);
			pipeCommand(command, left, right, type);
			freeTokens(left);
			freeTokens(right);
		}
		else
		{
			perror("error: the input is invalid");
			return;
		}
		break;
	case FOUND_RED:
		switch (count)
		{
		case 1: //one of < > >>
			if (type[RIGHT].index >= 1&& !(command[type[RIGHT].index+1] == NULL) // >
					&& command[type[RIGHT].index+2] == NULL)
			{
				rightCommand(command, type[RIGHT].index);
				return;
			}
			else if (type[LEFT].index
					>= 1&& command[type[LEFT].index+1]!= NULL // <
					&& command[type[LEFT].index+2] == NULL)
			{
				leftCommand(command, type[LEFT].index);
				return;
			}
			else if (type[DOUBLE].index
					>= 1&& command[type[DOUBLE].index+1] != NULL // >>
					&& command[type[DOUBLE].index+2] == NULL)
			{
				doubleCommand(command, type[DOUBLE].index);
				return ;
			}
			else
			{
				fprintf(stderr, "something went wrong\n");
				return;
			}
			break;
		case 2: //two of > < >> : < > or < >>
			break;
		}
		if ((type[RIGHT].index != -1
				|| type[DOUBLE].index != -1)
			&& (command[type[RIGHT].index + 1] != NULL
				|| command[type[DOUBLE].index + 1] != NULL)
			&& type[LEFT].index != -1
			&& command[type[LEFT].index + 1] != NULL
			&& (type[RIGHT].index > type[LEFT].index
				|| type[DOUBLE].index > type[LEFT].index))//< > or < >>
		{
			left_right_double_command(command, type);
			return;
		}
		break;
	}
}
//this func handle with only commands with pipe:
// | ; > | ; | > ; | >> ; < | > ; < | >>;
void pipeCommand(char** command, char** left, char** right, sgn_type* type) // |
{
	int i = 0;
	int pipe_descs[2];
	char file1_path[128], file2_path[128];
	char file_name[32];
	int fd_1, fd_2;
	int red_count = 0;
	pid_t childA, childB;
	char** middle_s = NULL;
	for (i = 0; i < 3; i++)
	{
		if (type[i].index != -1)
			red_count++;
	}
	if (pipe(pipe_descs) == -1)
	{
		perror("ERROR: cannot open pipe\n");
		freeTokens(command);
		freeTokens(left);
		freeTokens(right);
		exit(EXIT_FAILURE);
	}

	childA = fork();

	if (childA < 0)
	{
		perror("fork faild");
		freeTokens(command);
		freeTokens(left);
		freeTokens(right);
		exit(EXIT_FAILURE);
	}

	else if (childA == 0)
	{
		close(pipe_descs[0]);
		dup2(pipe_descs[1], STDOUT_FILENO);

		if (red_count > 0 && type[LEFT].index != -1) // <
		{
			left = left_side(command, type[LEFT].index);
			if (getcwd(file1_path, sizeof(file1_path)) == NULL)
			{
				perror("error at getcwd");
				freeTokens(left);
				freeTokens(right);
				freeTokens(command);
				exit(EXIT_FAILURE);
			}
			strcpy(file_name, command[type[LEFT].index + 1]);
			strcat(file1_path, "/");
			strcat(file1_path, file_name);
			fd_1 = open(file1_path, O_RDONLY,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd_1 == -1)
			{
				perror("input file error: cannot open");
				freeTokens(command);
				freeTokens(left);
				freeTokens(right);
				exit(EXIT_ERROR);
			}
			dup2(fd_1, STDIN_FILENO);
			close(fd_1);
		}
		close(pipe_descs[1]);
		if (execvp(left[0], left) < 0)
		{
			perror("ERROR: in first command");
			freeTokens(command);
			freeTokens(left);
			freeTokens(right);

			exit(EXIT_ERROR);
		}
		exit(1);
	}
	if (childA)
		childB = fork();

	if (childB < 0)
	{
		perror("fork faild");
		exit(EXIT_FAILURE);
	}
	else if (childB == 0)
	{
		close(pipe_descs[1]);
		dup2(pipe_descs[0], STDIN_FILENO);
		if (type[RIGHT].index == -1 && type[DOUBLE].index == -1) // < |
		{
			if (execvp(right[0], right) < 0)
			{
				perror("ERROR: in second command");
				freeTokens(command);
				freeTokens(left);
				freeTokens(right);
				exit(EXIT_ERROR);
			}
		}
		if (type[RIGHT].index != -1) // < | > or | >
		{
			middle_s = middle(command, type[PIPE].index + 1,
					type[RIGHT].index);
			if (getcwd(file2_path, sizeof(file2_path)) == NULL)
			{
				perror("error at getcwd");
				freeTokens(middle_s);
				freeTokens(command);
				freeTokens(right);
				freeTokens(left);
				exit(EXIT_FAILURE);
			}
			strcpy(file_name, command[type[RIGHT].index + 1]);
			strcat(file2_path, "/");
			strcat(file2_path, file_name);

			fd_2 = open(file2_path, O_WRONLY | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd_2 == -1)
			{
				perror("output file error: cannot open");
				freeTokens(middle_s);
				freeTokens(command);
				freeTokens(right);
				freeTokens(left);
				exit(EXIT_ERROR);
			}
			close(STDOUT_FILENO);
			dup(fd_2);
		}
		else if (type[DOUBLE].index != -1) // < | >> or | >>
		{
			middle_s = middle(command, type[PIPE].index + 1,
					type[DOUBLE].index);
			if (getcwd(file2_path, sizeof(file2_path)) == NULL)
			{
				perror("error at getcwd");
				freeTokens(middle_s);
				freeTokens(command);
				freeTokens(right);
				freeTokens(left);
				exit(EXIT_FAILURE);
			}
			strcpy(file_name, command[type[DOUBLE].index + 1]);
			strcat(file2_path, "/");
			strcat(file2_path, file_name);

			fd_2 = open(file2_path, O_WRONLY | O_CREAT | O_APPEND,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd_2 == -1)
			{
				perror("output file error: cannot open");
				freeTokens(middle_s);
				freeTokens(command);
				freeTokens(right);
				freeTokens(left);
				exit(EXIT_ERROR);
			}
			close(STDOUT_FILENO);
			dup(fd_2);
		}
		close(pipe_descs[0]);
		if (execvp(middle_s[0], middle_s) < 0)
		{
			perror("ERROR: in second command");
			freeTokens(command);
			freeTokens(left);
			freeTokens(right);
			freeTokens(middle_s);
			exit(EXIT_ERROR);
		}
		exit(1);
		freeTokens(middle_s);
	}
	close(pipe_descs[0]);
	close(pipe_descs[1]);

	if (childA != 0 && childB != 0)
	{
		temp = 0;
		//wait till the children finish their processes
		while (wait(&status) > 0)
		{
			if (WEXITSTATUS(status) != 0)
				temp = 255;
		}
		status = temp * 256;
	}
	freeTokens(middle_s);
}

void rightCommand(char** command, int right) // >
{
	char cwd[500];
	char* file_name = NULL;
	int fd;
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("error at getcwd");
		exit(EXIT_FAILURE);
	}
	file_name = command[right + 1];
	char file_path[100];
	sprintf(file_path, "%s%s%s", cwd, "/", file_name);
	child_pid = fork();
	if (child_pid < 0)
	{
		perror("fork fail\n");
		return;
	}
	else if (child_pid == 0) //child
	{
		fd = open(file_path, O_CREAT | O_WRONLY | O_TRUNC,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd == -1)
		{
			perror("cannot open input file");
			exit(EXIT_ERROR);
		}
		close(STDOUT_FILENO);
		dup(fd);
		char** temp = left_side(command, right);
		if (execvp(temp[0], temp) < 0) {
			printf("%s: ERROR_\n", temp[0]);
			freeTokens(temp);
			freeTokens(command);
			exit(EXIT_ERROR);
		}
		freeTokens(temp);
	}
	else wait(&status);
}
void leftCommand(char** command, int left) // <
{
	char cwd[500];
	char* file_name = NULL;
	int fd;
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		fprintf(stderr, "error at getcwd\n");
		exit(EXIT_FAILURE);
	}
	file_name = command[left + 1];
	char file_path[100];
	sprintf(file_path, "%s%s%s", cwd, "/", file_name);
	child_pid = fork();
	if (child_pid < 0)
	{
		fprintf(stderr, "fork fail\n");
		exit(EXIT_FAILURE);
	}
	else if (child_pid == 0) //father
	{
		fd = open(file_path, O_RDONLY, S_IRUSR | S_IWUSR | S_IXUSR);
		if (fd == -1)
		{
			perror("cannot open output file");
			exit(EXIT_ERROR);
		}
		close(STDIN_FILENO);
		dup(fd);
		char** temp = left_side(command, left);
		if (execvp(temp[0], temp) < 0)
		{
			printf("%s: ERROR_\n", temp[0]);
			freeTokens(temp);
			freeTokens(command);
			exit(EXIT_ERROR);
		}
		freeTokens(temp);
	}
	else wait(&status);
}
void doubleCommand(char** command, int doub) // >>
{
	char cwd[500];
	char* file_name = NULL;
	int fd;
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		fprintf(stderr, "error at getcwd\n");
	file_name = command[doub + 1];
	char file_path[100];
	sprintf(file_path, "%s%s%s", cwd, "/", file_name);
	child_pid = fork();
	if (child_pid < 0) {
		fprintf(stderr, "fork fail\n");
	}
	else if (child_pid == 0) //child
	{
		fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND,
				S_IRUSR | S_IWUSR | S_IXUSR);
		if (fd == -1)
		{
			perror("output file error");
			exit(EXIT_ERROR);
		}
		close(STDOUT_FILENO);
		dup(fd);
		char** temp = left_side(command, doub);
		if (execvp(temp[0], temp) < 0)
		{
			printf("%s: ERROR_\n", temp[0]);
			freeTokens(temp);
			freeTokens(command);
			exit(EXIT_ERROR);
		}
		freeTokens(temp);
	}
	else wait(&status);
}
// < > or < >>
void left_right_double_command(char** command, sgn_type* type)
{
	char cwd[1000];
	int fd_1, fd_2;
	char mid_file[100];
	char rig_file[100];
	char** left_s = NULL;
	char middle[32];
	char right_s[32];

	pid_t ch_id;

	if (getcwd(cwd, sizeof(cwd)) == NULL)
	{
		perror("error at cwd");
		exit(EXIT_FAILURE);
	}
	left_s = left_side(command, type[LEFT].index);

	strcpy(middle, command[type[LEFT].index + 1]);
	if (type[RIGHT].index != -1)
		strcpy(right_s, command[type[RIGHT].index + 1]);
	else if (type[DOUBLE].index != -1)
		strcpy(right_s, command[type[DOUBLE].index + 1]);

	sprintf(mid_file, "%s%s%s", cwd, "/", middle);
	sprintf(rig_file, "%s%s%s", cwd, "/", right_s);

	ch_id = fork();

	if (ch_id == 0)
	{
		fd_1 = open(mid_file, O_RDONLY,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd_1 == -1)
		{
			perror("input file error open error");
			exit(EXIT_ERROR);
		}
		close(STDIN_FILENO);
		dup(fd_1);
		close(fd_1);
		if (type[RIGHT].index != -1)
			fd_2 = open(rig_file, O_WRONLY | O_CREAT | O_TRUNC,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		else if (type[DOUBLE].index != -1)
			fd_2 = open(rig_file, O_WRONLY | O_CREAT | O_APPEND,
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd_2 == -1)
		{
			perror("cannot open output file error");
			exit(EXIT_ERROR);
		}
		close(STDOUT_FILENO);
		dup(fd_2);
		if (execvp(left_s[0], left_s) < 0)
		{
			perror("execvp error");
			freeTokens(command);
			freeTokens(left_s);
			exit(EXIT_ERROR);
		}

		status = 0;
		freeTokens(left_s);
	}
	if (ch_id < 0)
	{
		perror("fork faild");
		exit(EXIT_FAILURE);
	}
	else wait(&status);
	status = 0;
	freeTokens(left_s);
}
char** middle(char** command, int start, int end)
{
	int i = 0, j = 0;
	char** temp = (char**) malloc(sizeof(char*) * (end - start + 1));
	if (temp == NULL)
	{
		perror("ERROR at allocation");
		exit(1);
	}
	for (i = start; i < end; i++)
	{
		temp[j] = (char*) malloc((strlen(command[i]) + 1) * sizeof(char));
		if (temp[j] == NULL)
		{
			freeTokens(temp);
			fprintf(stderr, "allocation faild");
			exit(EXIT_FAILURE);
		}
		strcpy(temp[j], command[i]);
		j++;
	}
	temp[end - start] = NULL;
	return temp;
}
char** right_side(char** command, int pipe_index)
{
	int i = 0, j = 0;
	char** temp = (char**) malloc(sizeof(char*) * (numOfwords - pipe_index));
	if (temp == NULL)
	{
		fprintf(stderr, "ERROR at malloc\n");
		exit(1);
	}
	for (i = pipe_index + 1; command[i] != NULL; i++)
	{
		temp[j] = (char*) malloc((strlen(command[i]) + 1) * sizeof(char));
		if (temp[j] == NULL)
		{
			freeTokens(temp);
			fprintf(stderr, "allocation faild");
			exit(EXIT_FAILURE);
		}
		strcpy(temp[j], command[i]);
		j++;
	}
	temp[numOfwords - pipe_index - 1] = NULL;
	return temp;
}
char** left_side(char** command, int pipe_index)
{
	int i = 0;
	char** temp = (char**) malloc(sizeof(char*) * (pipe_index + 1));
	if (temp == NULL) {
		fprintf(stderr, "ERROR at malloc\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < pipe_index; i++)
	{
		temp[i] = (char*) malloc((strlen(command[i]) + 1) * sizeof(char));
		if (temp[i] == NULL)
		{
			freeTokens(temp);
			fprintf(stderr, "allocation faild");
			exit(EXIT_FAILURE);
		}
		strcpy(temp[i], command[i]);
	}
	temp[pipe_index] = NULL;
	return temp;
}
