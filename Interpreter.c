#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Interpreter.h"
#include "Sequence.h"
#include "Pipeline.h"
#include "Command.h"
/**
 * Interpreter takes parse tree created from parser, walks through it and executes the shell commands. Is the bridge between parsed commands and actual execution
 *
 */
static Command i_command(T_command t);
static void i_pipeline(T_pipeline t, Pipeline pipeline);
static void i_sequence(T_sequence t, Sequence sequence);

/**
 * @brief Interprets a single command node from the parse tree
 *
 * The bottom level of the tree-walking interpreter. It extracts the words (command name and arguments) from a comand parse tree node and creates an executable Command object
 *
 * @param t Command node from parse tree containing words and redirection info
 *
 * @return Command object ready to be added to a pipeline
 */

//  TODO Does not handle I/O redirection
static Command i_command(T_command t)
{
  if (!t)
    return 0;
  Command command = 0;
  if (t->words)
    command = newCommand(t->words);
  return command;
}

/**
 * @brief Interprets a pipelin node from the parse tree
 *
 * Recursively walks through a pipeline parse tree (commands connected by |) and builds a Pipleine object containing all commands in the pipeline.
 *
 * @param t Pipeline node from parse tree
 * @param pipeline Pipeline object being built
 *
 * @return VOID
 */
static void i_pipeline(T_pipeline t, Pipeline pipeline)
{
  if (!t)
    return;
  addPipeline(pipeline, i_command(t->command));
  i_pipeline(t->pipeline, pipeline);
}
/**
 * @brief Interprets a sequence node from the parse tree
 *
 * Recursively walks through a sequence parse tree (pipelines separated by
 * ; or &) and builds a Sequence object containing all pipelines.
 * Each pipeline will be executed in order (for ;) or in background (for &).

 *
 * @param t Sequence node from parse tree (may be NULL)
 * @param sequence Sequence object being built; pipelines are added to this
 *
 * @return void (modifies sequence parameter in-place)
 *
 * @note Recursive function - processes current pipeline then calls itself
 */

// TODO Implement background flag (&)
// TODO newPipleine has hard coded parameter of 1 needs to be changed to actual foreground/background flag from parse tree
static void i_sequence(T_sequence t, Sequence sequence)
{
  if (!t)
  {
    return;
  }
  // printf("DEBUG Interpreting a new sequencce ===========\n");
  // printf("DEBUG operation is => %s\n", t->op);

  // Whether the process runs in the fg or bg is determend by the operator after the command
  // & = run in the background
  // ; = run in the foreground
  int processFlag = 1;

  if (t->op && !strcmp(t->op, "&"))
  {
    // printf("DEBUG This sequence is set to run in the background!!\n");
    processFlag = 0;
  }

  Pipeline pipeline = newPipeline(processFlag);
  // printf("DEBUG ^^^^^^^^ Return from newPipeline ^^^^^^^^\n");
  // Newly parsed pipeline passed in to pipeline interpreter
  i_pipeline(t->pipeline, pipeline);
  // printf("DEBUG ^^^^^^^^ Return from i_pipeline ^^^^^^^^\n");

  addSequence(sequence, pipeline);
  i_sequence(t->sequence, sequence);
}

/**
 * @brief Main entry point for interpreting and executing a parse tree
 *
 * This is the public interface to the interpreter. It orchestrates the
 * two-phase interpretation process:
 *
 * Phase 1 - Structure Building (Tree Walking):
 *   Creates a new Sequence object and recursively walks the parse tree,
 *   converting it into a hierarchy of executable objects:
 *   Tree → Sequence → Pipeline(s) → Command(s)
 *
 * Phase 2 - Execution:
 *   Calls execSequence() which handles the actual execution of all
 *   commands, including process creation, I/O setup, waiting, etc.
 *
 * This separation of building and execution provides several benefits:
 * - Can validate entire structure before executing anything
 * - Can set up complex constructs (like pipelines) that need global view
 * - Can handle background jobs and job control more easily
 * - Can optimize or transform the structure if needed
 *
 * @param t Parse tree root representing complete user input (may be NULL)
 * @param eof Pointer to EOF flag; execSequence may set this to signal exit
 * @param jobs Job table for tracking all running/suspended processes
 *
 * @return void
 *
 * @note If t is NULL (empty input or parse error), function returns immediately
 * @note All actual execution happens in execSequence() - this function only
 *       builds the data structures
 * @note Memory management: Sequence and contained objects should be freed
 *       after execution (currently this may be missing - check for leaks!)
 */
extern void interpretTree(Tree t, int *eof, Jobs jobs)
{
  // Validate parse tree is valid
  if (!t)
  {
    return;
  }
  // New sequence created
  // A sequence is the root of the grammer the highest level rule
  Sequence sequence = newSequence();
  i_sequence(t, sequence);
  execSequence(sequence, jobs, eof);
}
