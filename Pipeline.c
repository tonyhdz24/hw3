#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Pipeline.h"
#include "Command.h"
#include "deq.h"
#include "error.h"

typedef struct
{
  Deq processes;
  int fg; // not "&"
} *PipelineRep;

extern Pipeline newPipeline(int fg)
{
  PipelineRep r = (PipelineRep)malloc(sizeof(*r));
  if (!r)
  {
    ERROR("malloc() failed");
  }
  r->processes = deq_new();
  r->fg = fg;
  return r;
}

extern void addPipeline(Pipeline pipeline, Command command)
{
  PipelineRep r = (PipelineRep)pipeline;
  deq_tail_put(r->processes, command);
}

extern int sizePipeline(Pipeline pipeline)
{
  PipelineRep r = (PipelineRep)pipeline;
  return deq_len(r->processes);
}

// Helper to execute a command with proper I/O redirection for pipes
static void exec_command_in_pipeline(Command command, int input_fd, int output_fd)
{
  // Redirect stdin if needed
  if (input_fd != STDIN_FILENO)
  {
    if (dup2(input_fd, STDIN_FILENO) == -1)
    {
      ERROR("dup2() failed for stdin");
    }
    close(input_fd);
  }

  // Redirect stdout if needed
  if (output_fd != STDOUT_FILENO)
  {
    if (dup2(output_fd, STDOUT_FILENO) == -1)
    {
      ERROR("dup2() failed for stdout");
    }
    close(output_fd);
  }

  // Execute the command (this will replace the process)
  typedef struct
  {
    char *file;
    char **argv;
  } *CommandRep;
  
  CommandRep r = command;
  execvp(r->argv[0], r->argv);
  ERROR("execvp() failed");
  exit(EXIT_FAILURE);
}

static void execute(Pipeline pipeline, Jobs jobs, int *jobbed, int *eof)
{
  PipelineRep r = (PipelineRep)pipeline;
  int n = sizePipeline(pipeline);

  // Special case: single command (no pipes needed)
  if (n == 1)
  {
    execCommand(deq_head_ith(r->processes, 0), pipeline, jobs, jobbed, eof, r->fg);
    return;
  }

  // Multiple commands: need pipes
  int **pipes = (int **)malloc(sizeof(int *) * (n - 1));
  if (!pipes)
    ERROR("malloc() failed");

  // Create all pipes
  for (int i = 0; i < n - 1; i++)
  {
    pipes[i] = (int *)malloc(sizeof(int) * 2);
    if (!pipes[i])
      ERROR("malloc() failed");
    if (pipe(pipes[i]) == -1)
      ERROR("pipe() failed");
  }

  // Add pipeline to jobs if needed
  if (!*jobbed)
  {
    *jobbed = 1;
    addJobs(jobs, pipeline);
  }

  // Fork and execute each command
  pid_t *pids = (pid_t *)malloc(sizeof(pid_t) * n);
  if (!pids)
    ERROR("malloc() failed");

  for (int i = 0; i < n; i++)
  {
    pids[i] = fork();
    if (pids[i] == -1)
      ERROR("fork() failed");

    if (pids[i] == 0)
    {
      // Child process
      typedef struct
      {
        char *file;
        char **argv;
        char *input;
        char *output;
      } *CommandRep;

      CommandRep cmd = deq_head_ith(r->processes, i);

      // Set up input redirection
      if (i > 0)
      {
        // Not the first command, read from previous pipe
        if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
          ERROR("dup2() failed");
      }
      // Override with file redirection if specified
      if (cmd->input)
      {
        int fd = open(cmd->input, O_RDONLY);
        if (fd == -1)
        {
          ERROR("failed to open input file");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDIN_FILENO) == -1)
          ERROR("dup2() failed");
        close(fd);
      }

      // Set up output redirection
      if (i < n - 1)
      {
        // Not the last command, write to next pipe
        if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
          ERROR("dup2() failed");
      }
      // Override with file redirection if specified
      if (cmd->output)
      {
        int fd = open(cmd->output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1)
        {
          ERROR("failed to open output file");
          exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1)
          ERROR("dup2() failed");
        close(fd);
      }

      // Close all pipe file descriptors in child
      for (int j = 0; j < n - 1; j++)
      {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      // Execute the command
      execvp(cmd->argv[0], cmd->argv);
      ERROR("execvp() failed");
      exit(EXIT_FAILURE);
    }
  }

  // Parent process: close all pipes
  for (int i = 0; i < n - 1; i++)
  {
    close(pipes[i][0]);
    close(pipes[i][1]);
    free(pipes[i]);
  }
  free(pipes);

  // Wait for all children if foreground
  if (r->fg)
  {
    for (int i = 0; i < n; i++)
    {
      int status;
      waitpid(pids[i], &status, 0);
    }
  }

  free(pids);
}

extern void execPipeline(Pipeline pipeline, Jobs jobs, int *eof)
{
  int jobbed = 0;
  execute(pipeline, jobs, &jobbed, eof);
  if (!jobbed)
    freePipeline(pipeline);
}

extern void freePipeline(Pipeline pipeline)
{
  PipelineRep r = (PipelineRep)pipeline;
  deq_del(r->processes, freeCommand);
  free(r);
}