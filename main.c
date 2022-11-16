#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHARSH_BUFFSIZE 2048 //arbritary value. 
#define CHARSH_TOKEN_BUFFSIZE 128 //token buffer size. 
#define CHARSH_TOKEN_DELIMITER " \t\r\n\a"

//Colours
#define ORANGE "\033[38;5;208m"
#define RED "\033[31m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

//prototyping
int charsh_launch_process(char** args);
void cleanExit(int statusCode);
void sigHandler(int);
void execFile(char*);

//Built-in commands
int charsh_cd(char** args);
int charsh_help(char** args);
int charsh_exit();
int charsh_execute(char** args);
int charsh_time(char** args);
char** charsh_split_line(char*);

//Environment variables
char* shell;

//Options
#ifndef bool
#define bool signed int
#define true 1
#define false 0
#endif
typedef struct options {
	bool isGay;
} options_t;
//TODO: Add options
options_t Options;

//Globals
char* histFile = NULL;
FILE* histFilefp;
char* optionsFile = NULL;
FILE* optionsFilefp;
bool verbose;

char* builtin_str[] = 
{
	"cd",
	"help",
	"exit",
	"time"
};

int(*builtin_func[]) (char**) = 
{
	&charsh_cd,
	&charsh_help,
	&charsh_exit,
	&charsh_time
};

int charsh_num_builtins()
{
	return sizeof(builtin_str) / sizeof(char*);
}

char* getHomeDir()
{
	char* homedir = getenv("HOME");
	if(homedir == NULL) homedir = getpwuid(getuid())->pw_dir;
	return homedir;
}

//built in implementation

int charsh_time(char** args)
{
	//FIXME: clock() doesn't work and neither does this.
	// struct timespec t1, t2;
	// clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
	// //Args starts with time so gotta move everything over by 1
	// //Get numbers of args
	// int numArgs = 0;
	// for(int i = 0; i < PATH_MAX; ++i)
	// {
		// ++numArgs;
		// if(args[i] == NULL) break;
	// }
	// //Edit args
	// for(int i = 0; i < numArgs; ++i)
	// {
		// args[i] = args[i+1];
	// }
	// charsh_execute(args);
	// clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
	// double time = (t2.tv_sec - t1.tv_sec) * 1000.0;
	// printf("Time: %.3f ms\n", time);
	return 1;
}

int charsh_cd(char** args)
{
	//if(args[1] == NULL) fprintf(stderr, "charsh: no argument given\n"); //could do some alt stuff
	if(args[1] == NULL) //if no arguments given i.e "cd"
	{
		//Get the home directory
		if(chdir(getHomeDir()) != 0) //change it to home directory
		{
			perror("charsh");
		}
	}
	else
	{
		//handle special cases
		if(strcmp(args[1], "~") == 0)
		{
			//TODO: Append home directory, e.g "cd ~/Documents" becomes "/home/user/Documents/"

			//If "cd ~" change to home directory
			if(chdir(getHomeDir()) != 0)
			{
				perror("charsh");
			}
		}
		//no special cases, cd to directory
		else if(chdir(args[1]) != 0)
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
		if(args[2] == NULL)
		{
			printf("Built in commands:\n");
			for(int i = 0; i < charsh_num_builtins(); ++i)
			{
				printf("\t%s\n", builtin_str[i]);
			}
			printf("Use the \"man\" command for more information on other programs.\n");
			printf("Use --help <command> for more information on specific commands.\n");
		}
		else
		{
			if(strcmp(args[2], "cd")   == 0) printf("cd <args>\nChange directory to the specified argument\n");
			if(strcmp(args[2], "exit") == 0) printf("Exits the shell\n");
		}
	}
	else if(strcmp(args[1], "cd")   == 0) printf("cd <args>\nChange directory to the specified argument\n");
	else if(strcmp(args[1], "exit") == 0) printf("Exits the shell\n");
	else perror("charsh");
	return 1;
}

int charsh_exit()
{
	cleanExit(EXIT_SUCCESS);
}

//execute built-ins

int charsh_execute(char** args)
{
	if(args[0] == NULL) return 1; //empty command
	long unsigned int numArgs = 0;
	//Get numbers of args
	for(long unsigned int i = 0; i < PATH_MAX; ++i)
	{
		++numArgs;
		if(args[i] == NULL) break;
	}
	//Write all to file
	for(long unsigned int i = 0; i < numArgs; ++i)
	{
		// printf("DEBUG: %s\n", args[i]);
		fprintf(histFilefp, args[i]); //write current arg
		fprintf(histFilefp, " "); //write space
	}
	//Append newline
	fprintf(histFilefp, "\n");
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
	uid_t uid = geteuid();
	struct passwd* pw = getpwuid(uid);
	char hostname[PATH_MAX];
	gethostname(hostname, PATH_MAX);
	char cwd[PATH_MAX];
	do
	{
		if(getcwd(cwd, sizeof(cwd)) != NULL)
		{
			const char* homedir = getHomeDir();
			if(strcmp(cwd, homedir) == 0) 
			{
				if(pw)
				{
					printf("%s%s@%s%s %s[%s~%s] %sλ%s ", pw->pw_name, RED,  RESET, hostname, BLUE, RESET, BLUE, ORANGE, RESET);
				}
				else 
				{
					printf("%s[%s~%s] %sλ%s ", BLUE, RESET, BLUE, ORANGE, RESET);
				}
			}
			else if(pw)
			{
				printf("%s%s@%s%s %s[%s%s%s] %sλ%s ", pw->pw_name, RED,  RESET, hostname, BLUE, RESET, cwd, BLUE, ORANGE, RESET);
			}
			else 
			{
				printf("%s[%s%s%s] %sλ%s ", BLUE, RESET, cwd, BLUE, ORANGE, RESET);
			}
		}
		else
		{
			if(pw)
			{
				printf("%s%s@%s%s %s[%s%s%s] %sλ%s ", pw->pw_name, RED,  RESET, hostname, BLUE, RESET, cwd, BLUE, ORANGE, RESET);
			}
			else 
			{
				printf("%s[%s%s%s] %sλ%s ", BLUE, RESET, cwd, BLUE, ORANGE, RESET);
			}
		}
		//TODO: Get arrow keys and apply history 
		// if(getch() == '\033')
		// {
			// switch(getch())
			// {
				// case 'A': //Arrow up
					// printf("DEBUG: Pressed Up\n");
					// break;
				// case 'B':
					// printf("DEBUG: Pressed Down\n");
					// break;
			// }
		// }
		line = charsh_read_line();
		args = charsh_split_line(line);
		status = charsh_execute(args);

		free(line);
		free(args);
	} while(status);
}

void cleanExit(int statusCode)
{
	//Reset shell env variable
	setenv("SHELL", shell, 1);
	//Close history file
	fclose(histFilefp);
	exit(statusCode);
}

void sigHandler(int sig)
{
	//printf("DEBUG: Goodbye via Ctr+C\n");
	cleanExit(137); //SIGKILL
}

void execFile(char* filepath)
{
	//TODO: Implement this
	printf("Function not yet implemented\n");
	cleanExit(EXIT_SUCCESS);
}

void parseOptions()
{
	if(verbose) printf("DEBUG: Parsing options file\n");
	//FIXME: Need to convert char* to char[] and then go through the array
	//		 to parse each option
	
	// char* options[PATH_MAX] = {""};
	// int index = 0;
	// int indexChar = 0;
	// char* tmp = "";
	// for(unsigned int i = 0; i < strlen(optionsFile); ++i)
	// {
		// strcat(tmp, optionsFile[indexChar++]);
		// if(strcmp(optionsFile[i], '\n') == 0 || strcmp(optionsFile[i], ' ') == 0)
		// {
			// options[index++] = tmp;
			// tmp = "";
		// }
	// }
	// if(verbose) printf("DEBUG: %s\n", optionsFile);
	// if(verbose)
	// {
		// for(int i = 0; i < PATH_MAX; ++i)
		// {
			// if(options[i] == NULL) break;
			// printf("DEBUG: %s\n", options[i]);
		// }
	// }
	//TODO: Set options
	//Options.isGay = true;
	fclose(optionsFilefp);
	if(verbose) printf("DEBUG: Options file parsed\n");
}

int main(int argc, char** argv)
{
	verbose = false;
	if(argc >= 2)
	{
		if(strcmp(argv[1],"--version") == 0)
		{
			printf("CharSH v0.0.1\n");
			return EXIT_SUCCESS;
		}
		else if(strcmp(argv[1], "--help") == 0)
		{
			argv[1] = NULL; //Wow this is so hacky
			charsh_help(argv);
			return EXIT_SUCCESS;
		}
		else if(strcmp(argv[1], "--verbose") == 0 || strcmp(argv[1], "-v") == 0)
		{
			verbose = true;	
		}
		else
		{
			//Assume file to run
			execFile(argv[1]);
			return EXIT_SUCCESS;
		}
	}
	//Register signal handler
	if(verbose) printf("DEBUG: Registering signal handler\n");
	signal(SIGINT, sigHandler);
	//Get the shell env variable so can change it back once exited
	if(verbose) printf("DEBUG: Getting SHELL env\n");
	shell = getenv("SHELL");
	//Change it to be charsh
	char absPath[PATH_MAX];
	char* p;
	if(!(p = strrchr(argv[0], '/'))) getcwd(absPath, sizeof(absPath));
	else
	{
		*p = '\0';
		char pathSave[PATH_MAX];
		getcwd(pathSave, sizeof(pathSave));
		chdir(argv[0]);
		getcwd(absPath, sizeof(absPath));
		chdir(pathSave);
	}
	if(verbose) printf("DEBUG: Setting SHELL env\n");
	setenv("SHELL", absPath, 1);
	//Get history for up arrow down arrow operations
	char historyFilePath[PATH_MAX] = "";
	strcat(historyFilePath, getHomeDir());
	strcat(historyFilePath, "/.charsh_history");
	if(verbose) printf("DEBUG: Reading history file from %s\n", historyFilePath);
	//TODO: Clean this up and generalise function
	histFilefp = fopen(historyFilePath, "a+");
	if(histFilefp != NULL)
	{
		if(verbose) printf("DEBUG: History file opened\n");
		if(fseek(histFilefp, 0L, SEEK_END) == 0)
		{
			long bufSize = ftell(histFilefp);
			if(bufSize == -1)
			{
				printf("Warning: Couldn't read history file: Buffer size is -1\n");
				//cleanExit(EXIT_FAILURE);
			}
			else
			{
				histFile = malloc(sizeof(char) * (bufSize + 1));
				if(fseek(histFilefp, 0L, SEEK_SET != 0))
				{
					printf("Warning: Couldn't read history file: Couldn't get file size\n");
					//cleanExit(EXIT_FAILURE);
				}
				else
				{
					size_t newLen = fread(histFile, sizeof(char), bufSize, histFilefp);
					if(ferror(histFilefp) != 0)
					{
						printf("Warning: Couldn't read history file: Unknown error\n");
						//cleanExit(EXIT_FAILURE);
					}
					else 
					{
						histFile[newLen++] = '\0'; //Safety mechanism
						if(verbose) printf("DEBUG: History file read\n");
					}
				}
			}
		}
	}
	else
	{
		printf("Warning: Couldn't read history file: File error. Tried reading %s\n", historyFilePath);
		//cleanExit(EXIT_FAILURE);
	}
	//Get options file
	char optionsFilePath[PATH_MAX] = "";
	strcat(optionsFilePath, getHomeDir());
	strcat(optionsFilePath, "/.charshrc");
	if(verbose) printf("DEBUG: Reading options file from %s\n", optionsFilePath);
	optionsFilefp = fopen(optionsFilePath, "r");
	if(optionsFilefp != NULL)
	{
		if(verbose) printf("DEBUG: Options file opened\n");
		if(fseek(optionsFilefp, 0L, SEEK_END) == 0)
		{
			long bufSize = ftell(optionsFilefp);
			if(bufSize == -1)
			{
				printf("Warning: Couldn't read options file: Buffer size is -1\n");
				//cleanExit(EXIT_FAILURE);
			}
			else
			{
				optionsFile = malloc(sizeof(char) * (bufSize + 1));
				if(fseek(optionsFilefp, 0L, SEEK_SET != 0))
				{
					printf("Warning: Couldn't read options file: Couldn't get file size\n");
					//cleanExit(EXIT_FAILURE);
				}
				else
				{
					size_t newLen = fread(optionsFile, sizeof(char), bufSize, optionsFilefp);
					if(ferror(optionsFilefp) != 0)
					{
						printf("Warning: Couldn't read options file: Unknown error\n");
						//cleanExit(EXIT_FAILURE);
					}
					else
					{
						optionsFile[newLen++] = '\0'; //Safety mechanism
						if(verbose) printf("DEBUG: Options file read\n");
					}
				}
			}
		}
	}
	else
	{
		printf("Warning: Couldn't read options file: File error. Tried reading %s\n", historyFilePath);
	}
	parseOptions();
	//Start processing commands
	charsh_loop();
	cleanExit(EXIT_SUCCESS);
}