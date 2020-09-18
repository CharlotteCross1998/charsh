#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHARSH_BUFFSIZE 2048 //arbritary value. Recommended 1024.

#define CHARSH_TOKEN_BUFFSIZE 128 //token buffer size. Recommended 64.
#define CHARSH_TOKEN_DELIMITER " \t\r\n\a"

//prototyping
int charsh_launch_process(char** args);

//Built-in commands
int charsh_cd(char** args);
int charsh_help(char** args);
int charsh_exit(char** args);

char* builtin_str[] = 
{
	"cd",
	"help",
	"exit"	
};

int(*builtin_func[]) (char**) = 
{
	&charsh_cd,
	&charsh_help,
	&charsh_exit
};

int charsh_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char*);
}

//built in implementation

int charsh_cd(char** args)
{
	if(args[1] == NULL) fprintf(stderr, "charsh: no argument given\n"); //could do some alt stuff
	else
	{
		//handle special cases
		if(strcmp(args[1], "~") == 0) //strcmp returns 0 if it's a match 
		{
			const char* homedir;
			if((homedir = getenv("HOME")) == NULL) homedir = getpwuid(getuid())->pw_dir;
			args[1] = homedir;
		}
		//no special cases, cd to directory
		if(chdir(args[1]) != 0)
		{
			perror("charsh");
		}
	}
	return 1;
}

int charsh_help(char** args)
{
	printf("Charlotte's Shell\n");
	if(args[1] == NULL)
	{
		printf("Built in commands:\n");
		for(int i = 0; i < charsh_num_builtins(); ++i)
		{
			printf("\t%s\n", builtin_str[i]);
		}
		printf("Use the \"man\" command for more information on other programs.\n");
	}
	else if(strcmp(args[1], "cd")   == 0) printf("cd <args>\nChange directory to the specified argument\n");
	else if(strcmp(args[1], "exit") == 0) printf("Exits the shell\n");
	else perror("charsh");
	return 1;
}

int charsh_exit(char** args)
{
	return 0;
}

//execute built-ins

int charsh_execute(char** args)
{
	if(args[0] == NULL) return 1; //empty command
	for(int i = 0; i < charsh_num_builtins(); ++i)
	{
		if(strcmp(args[0], builtin_str[i]) == 0)
		{
			return (*builtin_func[i])(args);
		}
	}

	return charsh_launch_process(args);
}

int charsh_launch_process(char** args)
{
	pid_t pid, wpid;
	int status;

	pid = fork();
	if(pid == 0)
	{
		if(execvp(args[0], args) == -1) perror("charsh");
		exit(EXIT_FAILURE);
	}
	else if(pid < 0) perror("charsh");
	else
	{
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 1;
}

char** charsh_split_line(char* line)
{
	int buffsize = CHARSH_TOKEN_BUFFSIZE, position = 0;
	char** tokens = malloc(buffsize * sizeof(char*));
	char *token;

	if(!tokens)
	{
		fprintf(stderr, "charsh: allocation error. \n"); //oopsie woopsie, we did a fuckie wucky;
		exit(EXIT_FAILURE);
	}

	token = strtok(line, CHARSH_TOKEN_DELIMITER);
	while(token != NULL)
	{
		tokens[position] = token;
		++position;

		if(position >= buffsize) // run out of memory? reallocate more.
		{
			buffsize += CHARSH_TOKEN_BUFFSIZE;
			tokens = realloc(tokens, buffsize);
			if(!tokens)
			{
				fprintf(stderr, "charsh: allocation error. \n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, CHARSH_TOKEN_DELIMITER);
	}
	tokens[position] = NULL;
	return tokens;
}

char* charsh_read_line(void)
{
	char* line = NULL;
	ssize_t buffsize = 0; //getline will allocate buffer for us.
	getline(&line, &buffsize, stdin);
	return line;
}

void charsh_loop(void)
{
	char* line;
	char** args;
	int status;
	do
	{
		char cwd[PATH_MAX];
		if(getcwd(cwd, sizeof(cwd)) != NULL)
		{
			const char* homedir;
			if((homedir = getenv("HOME")) == NULL) homedir = getpwuid(getuid())->pw_dir;
			if(strcmp(cwd, homedir) == 0) printf("~ charsh# "); //strcmp returns 0 if a match
			else printf("%s charsh# ", cwd);
		}
		else
		{
			printf("charsh# ");
		}
		line = charsh_read_line();
		args = charsh_split_line(line);
		status = charsh_execute(args);

		free(line);
		free(args);
	} while(status);
}

int main(int argc, char** argv)
{
	charsh_loop();
	return EXIT_SUCCESS;
}

