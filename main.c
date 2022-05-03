/*
 * COP4610   - Operating Systems
 * Project 1 - Implementing a Shell
 * Code By   - Sam Anderson, Luis Corps, James Kerrigan
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/queue.h>

#define TRUE 1
#define FALSE 0
#define MAXSIZE 1024
#define PATHMAX 4096

typedef struct
{
    int size;
    char **items;
} tokenlist;

typedef struct Job
{
    int pid;
    int pos;
    char command[256];
    time_t start;
    TAILQ_ENTRY(Job) jobs;
} Job;

TAILQ_HEAD(tailhead, Job);


/** global variables **/
static time_t longestProcess = 0;

/** provided functions **/
char *get_input (void);

tokenlist *get_tokens (char *input);

tokenlist *new_tokenlist (void);

void add_token (tokenlist *tokens, char *item);

void free_tokens (tokenlist *tokens);

/** our functions **/
char *replace_env (char *input);

void prompt ();

char *file_expansion (char *input);

/** built in commands **/
void cd_command (const tokenlist *tokens);

/** check up on background processes status **/
void jobs_command (struct tailhead *head);

void exit_command (const tokenlist *tokens, time_t start);

/** external commands **/
char *valid_externalcmd (const tokenlist *tokens);

void run_externalcmd (const tokenlist *tokens, const char *file);

/** background process & piping **/
void pipe_command (const tokenlist *pipeTokens, struct tailhead *head);

/** tokenizes background processes **/
void background_process (const tokenlist *tokens, time_t start, struct tailhead *head);

/** helper functions (misc) **/
void execute (const tokenlist *tokens, time_t start, struct tailhead *head);

void prepend (char *str, char *pre);

/** validate path **/
int check_path (char *path);

/** prints output to console **/
void print_output (const tokenlist *tokens);

tokenlist *get_tokens_sep (char *input, char *sep);

void update_longestProcess (time_t start, time_t end);

/** checks validity of command for background processing **/
int check_sep (const tokenlist *tokens, char *sep);

/** checks current process and list status **/
void check_processes (struct tailhead *head);

/** creates jobs out of background process tokens **/
void add_job (struct tailhead *head, int pid, time_t time,int pos, char *command);

/** checks for legal i/o commands **/
int valid_io (const tokenlist *tokens);

int main ()
{
    time_t start;
    time (&start);

    struct tailhead head;
    TAILQ_INIT(&head);

    while (1)
    {
        check_processes (&head);
        prompt ();
        char *input = get_input ();
        // printf ("whole input: %s\n", input);
        tokenlist *tokens = get_tokens (input);

        time_t begin, end;
        time (&begin);
        
        if(valid_io(tokens) == 1)
        {
          printf("error, bad input\n");
        }
        
        /* check for background processes first */
        else if (check_sep (tokens, "&") == TRUE)
        {
            tokenlist *bpTokens = get_tokens_sep (input, "&");
            background_process (bpTokens, start, &head);
            free_tokens (bpTokens);
        } else if (check_sep (tokens, "|") == TRUE)
        {
            tokenlist *pipeTokens = get_tokens_sep (input, "|");
            pipe_command (pipeTokens, &head);
            free_tokens (pipeTokens);
        } else
        {
            execute (tokens, start, &head);
        }
        /* update the longest process time */
        time (&end);
        update_longestProcess (begin, end);
        /* deallocate dynamic memory */
        free (input);
        free_tokens (tokens);
    }
}
/** start of functions **/

// *****************************************
// Provided Functions
// *****************************************
tokenlist *new_tokenlist (void)
{
    tokenlist *tokens = (tokenlist *) malloc (sizeof (tokenlist));
    tokens->size = 0;
    tokens->items = (char **) malloc (sizeof (char *));
    tokens->items[0] = NULL; /* make NULL terminated */
    return tokens;
}

void add_token (tokenlist *tokens, char *item)
{
    int i = tokens->size;
    tokens->items = (char **) realloc (tokens->items, (i + 2) * sizeof (char *));
    tokens->items[i] = (char *) malloc (strlen (item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);
    tokens->size += 1;
}

char *get_input (void)
{
    char *buffer = NULL;
    int bufsize = 0;
    char line[5];
    while (fgets (line, 5, stdin) != NULL)
    {
        int addby = 0;
        char *newln = strchr (line, '\n');

        if (newln != NULL)
        {
            addby = newln - line;
        } else
        {
            addby = 4;
        }

        buffer = (char *) realloc (buffer, bufsize + addby);
        memcpy(&buffer[bufsize], line, addby);
        bufsize += addby;

        if (newln != NULL)
        {
            break;
        }
    }
    buffer = (char *) realloc (buffer, bufsize + 1);
    buffer[bufsize] = 0;
    return buffer;
}

tokenlist *get_tokens (char *input)
{
    char *buf = (char *) malloc (strlen (input) + 1);
    strcpy(buf, input);
    tokenlist *tokens = new_tokenlist ();
    char *tok = strtok (buf, " ");
    const int dollar = '$', tilde = '~';

    while (tok != NULL)
    {
        const int ch = tok[0];
        /* update values based on first character */
        if (ch == dollar)
        {
            tok = replace_env (tok);
        } else if (ch == tilde)
        {
            tok = file_expansion (tok);
        }
        /* add values to tokenlist and get next token */
        add_token (tokens, tok);
        tok = strtok (NULL, " ");
    }
    free (buf);
    return tokens;
}

void free_tokens (tokenlist *tokens)
{
    for (int i = 0; i < tokens->size; i++)
    {
        free (tokens->items[i]);
    }
    free (tokens);
}

// ******************************************
// Our Functions
// ******************************************
char *replace_env (char *input)
{
    /* remove $ at index 0 of env and return result of env */
    char *result = input;
    const int dollar = '$', ch = input[0];
    /* compare ascii values of $ and first char of input */
    if (ch == dollar)
    {
        /* remove $ at front of variable */
        memmove(input, input + 1, strlen (input));
        result = getenv (input);
        result[strlen (result) + 1] = '\0';
    }
    return result;
}

void prompt ()
{
    printf ("%s@%s : %s > ", getenv ("USER"), getenv ("MACHINE"), getenv ("PWD"));
}

char *file_expansion (char *input)
{
    char *home = getenv ("HOME");
    static char expansion[PATHMAX];
    memset(expansion, '\0', PATHMAX); // init expansion
    memcpy(expansion, home, strlen (home));
    /* append input to expansion --> input + 1 move pointer 1 over the ~ */
    memcpy(&expansion[strlen (home)], (input + 1), strlen (input) - 1);
    return expansion;
}

// ******************************************
// Built-in Commands
// ******************************************
void cd_command (const tokenlist *tokens)
{
    if (tokens->size > 2)
    {
        printf ("Error more than one arguments");
        return;
    } else if (tokens->size == 1)
    {
        /* update PWD to HOME */
        char *home = getenv ("HOME");
        chdir (home);
        setenv ("PWD", home, 1);
        return;
    }
    char *temp = tokens->items[1];
    chdir (temp);
    char *cwd = getcwd (NULL, 0);
    setenv ("PWD", cwd, 1); // 1 means overwrite
    free (cwd);
}

void exit_command (const tokenlist *tokens, time_t start)
{
    time_t end;
    time (&end);                                /* get the end time of the program */
    const double total = difftime (end, start); /* calculate total run time */
    printf ("%s %.0f %s %.0f %s", "Shell ran for", total, "seconds and took", (double) longestProcess,
            "seconds to execute one command\n");
    /* clean up memory before program is terminated */
    // free_tokens (tokens);
    exit (0);
}

void jobs_command (struct tailhead *head)
{
    time_t end;
    time(&end);
    Job *np;
    int inx = 0;
    int status = 0;
    TAILQ_FOREACH(np, head, jobs)
    {
        status = waitpid (np->pid, NULL, WNOHANG);
        if (status != 0)
        {
            update_longestProcess (np->start, end);
            printf ("%s%i%s%i%s%s\n", "[", inx + 1, "] ", np->pid," done +\t", np->command);
            TAILQ_REMOVE(head, np, jobs);
            free (np);
        } else
        {
            printf ("%s%i%s%i%s%s\n", "[", inx + 1, "] ", np->pid," running +\t", np->command);
        }
        ++inx;
    }
}

// ******************************************
// External Commands
// ******************************************
void run_externalcmd (const tokenlist *tokens, const char *file)
{
    /* make a copy of file */
    char copy[MAXSIZE] = "";
    strncpy(copy, file, MAXSIZE - 1);

    pid_t pid = fork ();
    if (pid == 0)
    {
        /* execute external cmd in child process */
        int fdout = 0;
        int fdin = 0;
        char *argv[tokens->size + 1];
        memset(argv, 0, sizeof (argv));
        argv[0] = copy;

        /* Expands arguments for execv(); also applies I/O redirection if needed */
        for (int i = 1; i < tokens->size; i++)
        {

            if (strcmp (tokens->items[i], ">") == 0)
            {
                fdout = open (tokens->items[++i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                close (1);
                dup (fdout);
                close (fdout);
            } else if (strcmp (tokens->items[i], "<") == 0)
            {
                fdin = open (tokens->items[++i], O_RDONLY);
                close (0);
                dup (fdin);
                close (fdin);
            } else
            {
                argv[i] = tokens->items[i];
            }
        }
        execv (copy, argv);
    } else
    {
        waitpid (pid, NULL, 0); // parent
    }
}

char *valid_externalcmd (const tokenlist *tokens)
{
    char *cmd = tokens->items[0];
    /* prepend / to cmd */
    if (cmd[0] != '/')
    {
        prepend (cmd, "/");
    }

    char *path = getenv ("PATH");
    char copy[MAXSIZE] = "";
    strncpy(copy, path, MAXSIZE - 1); // make a copy
    char *list = strtok (copy, ":");

    while (list != NULL)
    {
        char temp[MAXSIZE] = "";
        strncpy(temp, list, strlen (list)); // use temp to make a copy
        strcat(temp, cmd);                 // append /cmd to list

        if (check_path (temp) == TRUE)
        {
            list = temp;
            list[strlen (list) + 1] = '\0';
            break;
        }
        list = strtok (NULL, ":"); // get next token
    }
    return list;
}

// ******************************************
// background processes & piping
// ******************************************
void background_process (const tokenlist *tokens, time_t start, struct tailhead *head)
{
    time_t bpStart;
    time(&bpStart);
    for (int inx = 0; inx < tokens->size; ++inx)
    {
        printf ("%s%i%s%s%i%s", "[", inx + 1, "]", "[", getpid (), "]\n");
        pid_t pid = fork ();
        if (pid == 0)
        {
            tokenlist *command = get_tokens (tokens->items[inx]);
            /* check for a pipe cmd */
            if (check_sep (command, "|") == TRUE)
            {
                tokenlist *pipeTokens = get_tokens_sep (tokens->items[inx], "|");
                pipe_command (pipeTokens, head);
                free_tokens (pipeTokens);
            } else
            {
                execute (command, start, head);
            }

            exit (1);

        } else
        {
            check_processes(head);

            if (TAILQ_EMPTY(head) == 1)
            {
                add_job (head, pid, bpStart, 1, tokens->items[inx]);

            } else
            {
                Job *last = TAILQ_LAST(head, tailhead);
                add_job (head, pid, bpStart, last->pos + 1, tokens->items[inx]);
            }
        }
    }
}

void pipe_command (const tokenlist *pipeTokens, struct tailhead *head)
{
    tokenlist *commands[pipeTokens->size];         //create tokenlist pointer array to store dynamic amount of commands 
    for (int i = 0; i < pipeTokens->size; i++) {            //populate tokenList * array with dynamic amount of commands
        commands[i] = get_tokens (pipeTokens->items[i]);
    }
    int numOfPipes = (pipeTokens->size - 1);            //# of pipes always one less than # of commands
    
    int pfd[numOfPipes][2];                         //pipe fd matrix, x coordinate initialized with # of pipes
    for (int i = 0; i < numOfPipes; i++) {
        pipe(pfd[i]);                               //populating pipe fd matrix
    }
    time_t timesOfRuns [pipeTokens->size];          //keep track of indivdual command times
    int pids[pipeTokens->size];                     //child process pids that will be forked in main for loop
    for (int i = 0; i < numOfPipes + 1; i++) {      //loop that will run for the amount of commands, creating separate child proccesses for each command
        pids[i] = fork();
        if (pids[i] == 0) {
            for (int j = 0; j < numOfPipes; j++) {
                if (i == 0) {       
                    if (i != j) {                   //first command only writing
                        close (pfd[j][1]);
                    }
                    close(pfd[j][0]);
                }
                else if (i == numOfPipes) {         //last command only reading and sending to stdout
                    if (i - 1 != j) {
                        close (pfd[j][0]);
                    }
                    close (pfd[j][1]);
                }
                else {                              //commands in between writing and reading to pipe fds 
                    if (i != j) {
                        close(pfd[j][1]);
                    }
                    if (i - 1 != j) {
                        close (pfd[j][0]);
                    }
                }
            }
            if (i == 0) {                           //first command only writing
                close(1);
                dup(pfd[0][1]);
                close(pfd[0][1]);
            }
            else if (i == numOfPipes) {             //last command only reading and sending to stdout
                close(0);
                dup(pfd[numOfPipes - 1][0]);
                close(pfd[numOfPipes - 1][0]);
            }
            else {                                  //commands in between writing and reading to pipe fds
                close(0);
                dup(pfd[i - 1][0]);
                close(1);
                dup (pfd[i][1]);
                close(pfd[i - 1][0]);
                close(pfd[i][1]);
            }
            time(&timesOfRuns[i]);
            execute(commands[i], timesOfRuns[i], head);
            exit(0);
        }
    }

    for (int i = 0; i < pipeTokens->size; i++) {    //freeing up tokens
        free_tokens(commands[i]);
    }
    for (int i = 0; i < numOfPipes; i++) {          //closing all fds
        close (pfd[i][0]);
        close (pfd[i][1]);
    }
    for (int i = 0; i < pipeTokens->size; i++) {    //waiting on all processes
        waitpid (pids[i], NULL, 0);
    }
}

// ******************************************
// Helper Functions (Misc.)
// ******************************************
void execute (const tokenlist *tokens, time_t start, struct tailhead *head)
{
    /* check if command is valid */
    if (tokens->size == 0)
    {
        /* empty cmd print newline and return */
        printf ("\n");
        return;
    } else if (strcmp (tokens->items[0], "echo") == 0)
    {
        print_output (tokens);
    } else if (strcmp (tokens->items[0], "cd") == 0)
    {
        cd_command (tokens); /* execute cd command */
    } else if (strcmp (tokens->items[0], "jobs") == 0)
    {
        jobs_command (head);
    } else if ((strcmp (tokens->items[0], "exit") == 0))
    {
        exit_command (tokens, start); /* terminate shell */
    } else if (valid_externalcmd (tokens) != NULL)
    {
        char *file = valid_externalcmd (tokens);
        run_externalcmd (tokens, file);
    } else
    {
        printf ("error command not found\n");
    }
}

int check_path (char *path)
{
    /* if path exist then return TRUE else FALSE */
    struct stat buffer;
    return stat (path, &buffer) == 0 ? TRUE : FALSE;
}

void prepend (char *str, char *pre)
{
    memmove(str + 1, str, strlen (str) + 1);
    memcpy(str, pre, 1);
}

void print_output (const tokenlist *tokens)
{
    /* print out the echo command */
    int size = tokens->size;
    int fdout = 0;
    int fdin = 0;
    int save_out = dup (fileno (stdout));
    int save_err = dup (fileno (stderr));
    
    //checks and operates on i/o redirectional arguments
    for (int inx = 1; inx < size; ++inx)
    {
        if (strcmp (tokens->items[inx], ">") == 0 && tokens->items[inx + 1] != NULL)
        {

            fdout = open (tokens->items[++inx], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            close (1);
            dup (fdout);
            close (fdout);

        } else if (strcmp (tokens->items[inx], "<") == 0 && tokens->items[inx + 1] != NULL)
        {
            fdin = open (tokens->items[++inx], O_RDONLY);
            close (0);
            dup (fdin);
            close (fdin);
        }
    }

    for (int inx = 1; inx < size; ++inx)
    {
        if (strcmp (tokens->items[inx], "<") == 0 ||
            strcmp (tokens->items[inx], ">") == 0)
        {
            break;
        }
        printf ("%s ", tokens->items[inx]);
    }
    printf ("\n");

    dup2 (save_out, fileno (stdout));
    dup2 (save_err, fileno (stderr));
}

tokenlist *get_tokens_sep (char *input, char *sep)
{
    char *buf = (char *) malloc (strlen (input) + 1);
    strcpy(buf, input);
    tokenlist *tokens = new_tokenlist ();
    char *tok = strtok (buf, sep);

    while (tok != NULL)
    {
        add_token (tokens, tok);
        tok = strtok (NULL, sep);
    }
    free (buf);
    return tokens;
}

void update_longestProcess (time_t start, time_t end)
{
    /* update longestProcess if temp is larger */
    const double temp = difftime (end, start);
    longestProcess = temp > longestProcess ? temp : longestProcess;
}

int check_sep (const tokenlist *tokens, char *sep)
{
    /* check for the existence if a given separator in tokens */
    int num = 0;
    for (int inx = 0; inx < tokens->size; ++inx)
    {
        if (strcmp (tokens->items[inx], sep) == 0)
        {
            num++;
            break; /* separator found break from loop */
        }
    }
    return num > 0;
}

void check_processes (struct tailhead *head)
{
    time_t end;
    time(&end);
    if (TAILQ_EMPTY(head) == 1)
    {
        return;
    }

    pid_t status = 0;
    Job *np;
    int inx = 0;
    TAILQ_FOREACH(np, head, jobs)
    {
        if ((status = waitpid (np->pid, NULL, WNOHANG)) != 0)
        {
            update_longestProcess (np->start, end);
            printf ("%s%i%s%s%s\n", "[", inx + 1, "]", " done +\t", np->command);
            TAILQ_REMOVE(head, np, jobs);
            free (np);
        }
        ++inx;
    }
}

void add_job (struct tailhead *head, int pid, time_t time,int pos, char *command)
{
    Job *job = (Job *) malloc (sizeof (Job));
    job->pid = pid;
    job->pos = pos;
    job->start = time;
    strncpy(job->command, command, 256);

    TAILQ_INSERT_TAIL(head, job, jobs);
}

int valid_io (const tokenlist *tokens)
{
  for (int i = 1; i < tokens->size; i++)
  {
    if ((strcmp (tokens->items[i], ">") == 0) ||
       (strcmp (tokens->items[i], "<") == 0))
    {

      if((strcmp(tokens->items[i - 1], ">") == 0) ||
        (strcmp(tokens->items[i + 1], "<") == 0))
      {
        return 1;
      }
    }
  }
  return 0;
}
/** end of functions **/
