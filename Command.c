#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/history.h>
#include <fcntl.h>
#include "Command.h"
#include "error.h"
#include "deq.h"

/**
 * CommandRep - Internal representation of a Command
 *
 * This struct holds the informatin to execute a command:
 * - file: The name of the program to execute (ls)
 * - argv: Array of argument strings ["ls","-l"]
 *         argv[0] is always the program name
 */
typedef struct
{
  char *file;
  char **argv;
  char *input;
  char *output;
} *CommandRep;

// Macro Definitions for Builtin Commands
#define BIARGS CommandRep r, int *eof, Jobs jobs      // Set built in number of arguments
#define BINAME(name) bi_##name                        // set the name of built in
#define BIDEFN(name) static void BINAME(name)(BIARGS) // define built in
#define BIENTRY(name) {#name, BINAME(name)}           // ??

// Old working directory
static char *owd = 0;
// Current working directory
static char *cwd = 0;
// Queue to track all background process
static Deq background_pids = NULL;

/**
 *
 * Reap any terminated background process
 *
 * Checks all PIDs in the background_pids and removes any that have terminated
 *
 */
extern void reap_background_processes()
{
  if (!background_pids || deq_len(background_pids) == 0)
  {
    return;
  }

  int num_to_check = deq_len(background_pids);

  for (int i = 0; i < num_to_check; i++)
  {
    int pid = (int)(long)deq_head_get(background_pids);
    int status;
    int result = waitpid(pid, &status, WNOHANG);

    if (result == 0)
    {
      // Still running, put back at tail
      deq_tail_put(background_pids, (Data)(long)pid);
    }
    else if (result == pid)
    {
      // Process terminated, don't put back (it's reaped!)
      // That's it! Just don't put it back in the queue
    }
    else if (result == -1)
    {
      // Error or process doesn't exist, don't put back
    }
  }
}
/**
 * Validates that a builtin command received the correct number of arguments
 *
 * This function counts the actual number of arguments in argv and compares
 * it to the expected count 'n'. If they don't match, it reports an error.
 *
 * Algorithm:
 * 1. Start with n (expected count)
 * 2. Walk through argv, decrementing n for each argument
 * 3. After the loop, if n != 0, we have wrong number of args
 *
 * @param r  Command representation containing argv
 * @param n  Expected number of arguments (NOT counting program name)
 *
 * Example: For "cd /tmp", argv=["cd", "/tmp", NULL], n should be 1
 *          Loop: n=1+1=2, *argv++=argv[0] n=1, *argv++=argv[1] n=0, *argv++=NULL loop ends
 *          If n=0 at end, correct number of args!
 */
static void builtin_args(CommandRep r, int n)
{
  char **argv = r->argv;
  for (n++; *argv++; n--)
    ;
  if (n)
    ERROR("wrong number of arguments to builtin command"); // warn
}

// BIDEFN(histroy)
// {
//   printf("DEBUG print command history");
// }

// Exit Built in command
BIDEFN(exit)
{
  // printf("DEBUG Exit called check for any running processes\n");
  // printf("DEBUG Checking if the background job queue is empry\n");
  // printf("DEBUG length => %d\n", deq_len(background_pids));
  builtin_args(r, 0);
  if (background_pids)
  {
    while (deq_len(background_pids) > 0)
    {
      int exit_pid = (int)(long)deq_head_get(background_pids);
      waitpid(exit_pid, 0, 0);
    }
  }

  *eof = 1; // Set end of file to 1 exiting program
}

// Print Working Directory command
BIDEFN(pwd)
{
  builtin_args(r, 0); // valdiate number of arguments

  // Check if current working directory exists
  if (!cwd)
  {
    // IF not get current working directory
    cwd = getcwd(0, 0);
  }
  // Out put current directory to terminal
  printf("%s\n", cwd);
}

// Change Directory command
BIDEFN(cd)
{
  builtin_args(r, 1); // Validate number of arguments

  // Check if the first argument is -
  // - means go to the previous directory
  if (strcmp(r->argv[1], "-") == 0)
  {
    char *twd = cwd;
    cwd = owd;
    owd = twd;
  }
  else
  {
    // IF there was an old working directory
    if (owd)
    {
      free(owd); // free it from memory
    }
    // Update owd and cwd variables
    owd = cwd;

    // Change directroy
    if (chdir(r->argv[1]) == -1)
    {
      ERROR("chdir() failed");
      return;
    }

    // Update cwd variable
    cwd = getcwd(0, 0);
    if (strcmp(r->argv[1], "-") == 0 && cwd && chdir(cwd))
      ERROR("chdir() failed");
  }
}
// Implementation of history built in
BIDEFN(history)
{
  // Valdiate number of arguments
  builtin_args(r, 0);
  // Get history list
  HIST_ENTRY **list = history_list();
  if (!list)
  {
    return;
  }

  for (int i = 0; list[i]; i++)
  {
    printf("%d: %s\n", i + history_base, list[i]->line);
  }
}

/**
 * Dispatcher function that checks if a command is a builtin and executes it
 *
 * This function maintains a table of all builtin commands and their
 * corresponding handler functions. It searches the table for a match
 * with the command name and executes it if found.
 *
 * The builtin table is defined using our macros for consistency:
 * - Each entry maps a command name (string) to its function pointer
 * - Table is terminated with {0, 0} sentinel
 *
 * @param r    Command representation (r->file is command name)
 * @param eof  EOF flag pointer (passed to builtin if executed)
 * @param jobs Job table (passed to builtin if executed)
 *
 * @return 1 if command was a builtin and was executed
 *         0 if command was not found in builtin table
 */
static int builtin(BIARGS)
{
  typedef struct
  {
    char *s;
    void (*f)(BIARGS);
  } Builtin;
  static const Builtin builtins[] = {
      BIENTRY(exit),
      BIENTRY(pwd),
      BIENTRY(cd),
      BIENTRY(history),
      {0, 0}};
  int i;
  for (i = 0; builtins[i].s; i++)
    if (!strcmp(r->file, builtins[i].s))
    {
      builtins[i].f(r, eof, jobs);
      return 1;
    }
  return 0;
}
/**
 * Converts a T_words linked list from the parse tree into an argv array
 *
 * The parse tree represents command arguments as a linked list of words.
 * The exec family of functions requires arguments as a NULL-terminated
 * array of strings (char**). This function performs that conversion.
 *
 * Algorithm:
 * 1. First pass: Count the number of words in the linked list
 * 2. Allocate array of (n+1) char* pointers (extra slot for NULL terminator)
 * 3. Second pass: Copy each word string into the array
 * 4. Set final element to NULL (required by execvp)
 *
 * Example:
 *   Input:  T_words list: "ls" -> "-l" -> "/tmp" -> NULL
 *   Output: argv array: ["ls", "-l", "/tmp", NULL]
 *
 * @param words  Linked list of words from parse tree
 *
 * @return Newly allocated argv array (caller must free)
 *         NULL-terminated array of string pointers
 */
static char **getargs(T_words words)
{
  int n = 0;
  T_words p = words;
  while (p)
  {
    p = p->words;
    n++;
  }
  char **argv = (char **)malloc(sizeof(char *) * (n + 1));
  if (!argv)
    ERROR("malloc() failed");
  p = words;
  int i = 0;
  while (p)
  {
    argv[i++] = strdup(p->word->s);
    p = p->words;
  }
  argv[i] = 0;
  return argv;
}

extern Command newCommand(T_words words, T_redir redir)
{
  CommandRep r = (CommandRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->argv = getargs(words);
  r->file = r->argv[0];
  // handle redirs
  r->input = redir && redir->input ? strdup(redir->input) : NULL;
  r->output = redir && redir->output ? strdup(redir->output) : NULL;
  return r;
}
/**
 * Child process execution function
 *
 * This function runs in the CHILD process after fork().
 * It's responsible for:
 * 1. Checking if command is a builtin (execute if so, then return)
 * 2. If not builtin, replace child process with the external program (execvp)
 *
 * IMPORTANT: This function only returns if the command is a builtin.
 * If execvp() is called, it REPLACES the child process entirely.
 * If execvp() fails, we error and exit.
 *
 * Why check for builtins in child?
 * - Some builtins might be in a pipeline or backgrounded
 * - In those cases, they should run in a child process, not the shell
 *
 * Process flow:
 *   fork() creates child
 *   ↓
 *   child() called in child process
 *   ↓
 *   Is builtin? → Yes → execute, return, child exits
 *             → No  → execvp replaces process with new program
 *                     (if execvp fails, error and exit)
 *
 * @param r   Command representation to execute
 * @param fg  Foreground flag (currently unused in this function)
 */
// NEW child() - handles redirection
static void child(CommandRep r, int fg)
{
  // Handle input redirection
  if (r->input)
  {
    int fd = open(r->input, O_RDONLY); // Open input file
    if (fd == -1)
    {
      ERROR("failed to open input file");
      exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDIN_FILENO) == -1) // Redirect stdin to file
    {
      ERROR("dup2() failed for input");
      exit(EXIT_FAILURE);
    }
    close(fd); // Close original fd, stdin is now the file
  }

  // Handle output redirection
  if (r->output)
  {
    int fd = open(r->output, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Create/open output file
    if (fd == -1)
    {
      ERROR("failed to open output file");
      exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDOUT_FILENO) == -1) // Redirect stdout to file
    {
      ERROR("dup2() failed for output");
      exit(EXIT_FAILURE);
    }
    close(fd); // Close original fd, stdout is now the file
  }

  // Now execute - stdin/stdout are redirected if needed
  int eof = 0;
  Jobs jobs = newJobs();
  if (builtin(r, &eof, jobs))
  {
    return;
  }
  execvp(r->argv[0], r->argv);
  ERROR("execvp() failed");
  exit(EXIT_FAILURE);
}
// static void child(CommandRep r, int fg)
// {

//   // printf("DEBUG in child()\n");

//   int eof = 0;
//   Jobs jobs = newJobs();
//   // check if child command is a builtIn if it is then execute the built in command
//   if (builtin(r, &eof, jobs))
//   {
//     return;
//   }
//   // If not a builtIN then call execvp() replaces the child process with the external program
//   execvp(r->argv[0], r->argv);
//   ERROR("execvp() failed");
//   exit(0);
// }

extern void execCommand(Command command, Pipeline pipeline, Jobs jobs,
                        int *jobbed, int *eof, int fg)
{
  // command to execute
  CommandRep r = command;
  // printf("DEBUG executing command => %s \n", r->file);

  // printf("DEBUG comamand fg is => %d \n", fg);

  // IF command is set to run in foreground and is a built in run immediatly
  if (fg && builtin(r, eof, jobs))
  {
    // printf("DEBUG this command is a built in!\n");
    return;
  }
  // Check if this command has been jobbed (added to the jobs queue) if not add to jobs queue
  if (!*jobbed)
  {
    *jobbed = 1;
    addJobs(jobs, pipeline);
  }
  // Fork (create a new child process)
  int pid = fork();

  // printf("DEBUG pid is = %d \n", pid);

  if (pid == -1)
  {
    ERROR("fork() failed");
  }

  // If process is a child
  if (pid == 0)
  {
    // printf("DEBUG process is a child !!!\n");
    // process child
    child(r, fg);
  }
  else
  {
    // Process is a parent wait for child to exit
    if (fg)
    {
      int status;
      waitpid(pid, &status, 0);
    }
    else
    {
      if (!background_pids)
      {
        // printf("DEBUG Init background jobs queue!\n");
        background_pids = deq_new();
      }
      deq_tail_put(background_pids, (Data)(long)pid);
    }
  }
}
extern void freeCommand(Command command)
{
  CommandRep r = command;
  char **argv = r->argv;
  while (*argv)
    free(*argv++);
  free(r->argv);
  free(r);
}

extern void freestateCommand()
{
  if (cwd)
    free(cwd);
  if (owd)
    free(owd);
}
