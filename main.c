#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Function propotype for built in shell commands */
int fsh_cd(char **args);
int fsh_help(char **args);
int fsh_exit(char **args);

char *builtin_str[] = {
	"cd",
	"help", 
	"exit"
};

int (*builtin_func[]) (char **) = {
	&fsh_cd,
	&fsh_help,
	&fsh_exit
};

int fsh_num_builtins() {
	return sizeof(builtin_str) / sizeof(char *);
}

int fsh_cd(char **args) {
	if (args[1] == NULL) {
		fprintf(stderr, "fsh: expected argument to \"cd\"\n");
	} else {
		if (chdir(args[1]) != 0) {
			perror("fsh");
		}
	}
	return 1;
}

int fsh_help(char **args) {
	int i;
	printf("Noah FSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in: \n");

	for (i = 0; i < fsh_num_builtins(); i++) {
		printf(" %s\n", builtin_str[i]);
	}

	printf("Use the man command for information on other porgrams.\n");
	return 1;
}

int fsh_exit(char **args) {
	return 0;
}

int fsh_launch(char **args) {
	pid_t pid;
	int status;

	pid = fork();
	if (pid == 0) {
		if (execvp(args[0], args) == -1) {
			perror("fsh");
		}
		exit(EXIT_FAILURE);
	} else {
		do {
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int fsh_execute(char **args) {
	if (args[0] == NULL) {
		return 1;
	}
	for (int i = 0; i < fsh_num_builtins(); i++) {
		if (strcmp(args[0], builtin_str[i]) == 0) {
			return (*builtin_func[i])(args);
		}
	}
	return fsh_launch(args);
}

char *fsh_read_line(void) {
#ifdef FSH_USE_STD_GETLINE
	char *line = NULL;
	ssize_t bufsize = 0; 
	if (getline(&line, &bufsize, stdin) == -1) {
		if(feof(stdin)) {
			exit(EXIT_SUCCESS);
		} else {
			perror("fsh: getline\n");
			exit(EXIT_FAILURE);
		}
	}
	return line;
#else 
#define FSH_RL_BUFSIZE 1024
	int bufsize = FSH_RL_BUFSIZE;
	int position = 0;
	char *buffer = malloc(sizeof(char) * bufsize);
	int c;

	if (!buffer) {
		fprintf(stderr, "fsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		c = getchar();

		if (c == EOF) {
			exit(EXIT_SUCCESS);
		} else if (c == '\n') {
			buffer[position] = '\0';
			return buffer;
		} else {
			buffer[position] = c;
		}
		position++;

		if (position >= bufsize) {
			bufsize += FSH_RL_BUFSIZE;
			buffer = realloc(buffer, bufsize);
			if (!buffer) {
				fprintf(stderr, "fsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
#endif
}

#define FSH_TOK_BUFSIZE 64
#define FSH_TOK_DELIM " \t\r\n\a"

char **fsh_split_line(char *line) {
	int bufsize = FSH_TOK_BUFSIZE, position = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token, **tokens_backup;

	if (!tokens) {
		fprintf(stderr, "fsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, FSH_TOK_DELIM);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += FSH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = realloc(tokens, bufsize * sizeof(char *));
			if (!tokens) {
				free(tokens_backup);
				fprintf(stderr, "fsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, FSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

void fsh_loop(void) {
	char *line;
	char **args;
	int status;

	do {
		printf("> ");
		line = fsh_read_line();
		args = fsh_split_line(line);
		status = fsh_execute(args);

		free(line);
		free(args);
	} while (status);
}

int main(int argc, char **argv) {
	fsh_loop();
	return EXIT_SUCCESS;
}