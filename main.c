#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wordexp.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>

//#include "config.h"

#define SH_BUF_SIZE 1024
#define SH_TOK_BUF 64
#define SH_TOK_DELIM " \t\r\n\a&&||"
#define MAX_PROMPT_LEN 64
#define MAX_CWD 64

static char prompt[MAX_PROMPT_LEN];
char *logfile = "/home/snow/.bnim_history";
int log_set = 1;
int log_time = 1;

char *sh_readline (void);
char **sh_splitline (char *line);
int sh_launch (char **args);
int sh_cd (char **args);
int sh_help (char **args);
int sh_exit (char **args);
int sh_num_builtins (void);
int sh_exec (char **args);
void sh_loop (void);
void int_handler (int signum);
void sh_log (char **args);
char *get_cwd (void);
int fill_prompt (void);

int fill_prompt (void)
{
    prompt [0] = '\0';

    char cwd[MAX_CWD];
    if (getcwd (cwd, sizeof (cwd)) == NULL)
        return 1;

    char *delim = "/";
    char *nl = "she";
    char *token;

    const char *copy = cwd;
    char bf[MAX_CWD];
    for (int i = strlen (copy) - 1; i > 0; i--)
    {
        if (copy[i] == '/')
            break;
        else
        {
            char ch = copy[i];
            strncat (bf, &ch, 1);
        }
    }

    static int counter = 0;

    for (int i = 0; i < strlen (cwd); i++)
    {
        if (cwd[i] == '/')
            counter++;
    }

    token = strtok (cwd, delim);
    for (int i = 1; i < counter; i++) 
    {
        char str = token[0];
        strncat (prompt, delim, strlen (delim));
        strncat (prompt, &str, 1);

        token = strtok (NULL, delim);
    }

    strncat (prompt, delim, 1);

    for (int i = strlen (bf) - 1; i >= 0; i--)
    {
        char ch = bf[i];
        strncat (prompt, &ch, 1);
    }

    char *symbol = " $ ";
    strncat (prompt, symbol, strlen (symbol));

    counter = 0;
    
}

char **sh_splitline (char *line)
{
    int bufsize = SH_TOK_BUF, position = 0;
    char **tokens = malloc (bufsize * sizeof (char*));
    char *token;

    if (!tokens)
    {
        fprintf (stderr, "[!] memory allocation failure\n");
        exit (EXIT_FAILURE);
    }

    token = strtok (line, SH_TOK_DELIM);
    while (token != NULL)
    {
        tokens [position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += SH_TOK_BUF;
            tokens = realloc (tokens, bufsize * sizeof (char*));
            if (!tokens)
            {
                fprintf (stderr, "[!] memory allocation failure\n");
                exit (EXIT_FAILURE);
            }
        }
        token = strtok (NULL, SH_TOK_DELIM);
    }
    tokens [position] = NULL;
    return tokens;
}

int sh_launch (char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork ();
    if (pid == 0)
    {
        if (execvp (args[0], args) == -1)
            perror ("bnim");
        exit (EXIT_FAILURE);
    } else if (pid < 0)
        perror ("bnim");
    else
        do
        {
            wpid = waitpid (pid, &status, WUNTRACED);
        } while (!WIFEXITED (status) && !WIFSIGNALED (status));
    if (status == 0 && log_set == 1)
        sh_log (args);
    return 1;
}

char *builtin_str[] = 
{
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = 
{
    &sh_cd,
    &sh_help,
    &sh_exit
};

int sh_num_builtins (void)
{
    return sizeof (builtin_str) / sizeof (char *);
}

int sh_cd (char **args)
{
    if (args[1] == NULL)
    {
        char *homedir = getenv ("HOME");
        chdir (homedir);
        sh_log (args);
    } else
    {
        wordexp_t exp_result;
        wordexp (args[1], &exp_result, 0);
        if (chdir ((exp_result.we_wordv[0])) != 0)
            perror ("bnim");
        else
            //if (log_set == 1)
               // sh_log (args);
        wordfree (&exp_result);
    }
    return 1;
}

int sh_exec (char **args)
{
    int i;
    if (args [0] == NULL)
        return 1;

    for (i = 0; i < sh_num_builtins (); i++)
    {
        if (strcmp (args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
    }
    return sh_launch (args);
}

int sh_help (char **args)
{
    int i;
    printf ("bnim - bash not improved\n");

    for (i = 0; i < sh_num_builtins (); i++)
        printf ("  %s\n", builtin_str[i]);

    printf ("use man <prg> for info on other programs\n");
    return 1;
}

int sh_exit (char **args)
{
    return 0;
}

void sh_loop (void)
{
    char *line;
    char **args;
    int status;

    do
    {
        fill_prompt ();

        //printf("%s", readline(prompt));
        //line = sh_readline ();
        char *line = readline (prompt);
        add_history (line);
        args = sh_splitline (line);
        status = sh_exec (args);

        free(line);
        free(args);
    } while (status);
}

void int_handler (int signum)
{
    printf ("exit successful.\n");
    exit (0);
}

void sh_log (char **args)
{
    FILE *fp;
    fp = fopen (logfile, "a");
    if (log_time == 1)
    {
        time_t secs = time (NULL);
        fprintf (fp, "%ld: ", secs);
    }
    for (int i = 0; i < 100; i++)
    {
        if (args[i] == NULL)
            break;
        else
            fprintf (fp, "%s ", args[i]);
    }
    fprintf (fp, "\n");
    fclose (fp);
}

int main (int argc, char *argv[])
{

    // TODO: LOAD CONFIG FILES

    signal (SIGINT, int_handler);

    sh_loop();

    // TODO: SHUTDOWN / CLEANUP

    return EXIT_SUCCESS;
}
