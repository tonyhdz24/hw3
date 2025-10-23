#ifndef COMMAND_H
#define COMMAND_H

typedef void *Command;

#include "Tree.h"
#include "Jobs.h"
#include "Sequence.h"
/**
 * Reap terminated background processes to prevent zombies
 * Should be called periodically (e.g., after each command)
 */
extern void reap_background_processes();
/**
 * Creates a new Command object from a list of words
 *
 * This is the public constructor for Command objects. It takes the words
 * from the parse tree and creates an executable command representation.
 *
 * Steps:
 * 1. Allocate CommandRep structure
 * 2. Convert T_words linked list to argv array
 * 3. Set file pointer to program name (argv[0])
 *
 * The returned Command is an opaque pointer (void*) that hides the
 * internal CommandRep structure from other modules.
 *
 * @param words  Linked list of words from parse tree (command + arguments)
 *
 * @return Command object (opaque pointer to CommandRep)
 *         Returns newly allocated command ready for execution
 *
 * Example:
 *   Input:  words: "cat" -> "file.txt" -> NULL
 *   Output: Command with argv=["cat", "file.txt", NULL], file="cat"
 */
extern Command newCommand(T_words words);
/**
 * Executes a command - the main entry point for command execution
 *
 * This function orchestrates command execution with the following logic:
 *
 * 1. FOREGROUND BUILTIN: Execute immediately in shell process, return
 * 2. OTHER COMMANDS: Fork a child process to execute
 *
 * Process Management:
 * - Tracks jobs (pipelines) being executed
 * - First command in a pipeline adds the pipeline to job table
 * - Forks child process for non-foreground-builtin commands
 *
 * CRITICAL ISSUE: This function does NOT wait for children!
 * ⚠️ TODO: You need to add waitpid() for foreground commands
 * ⚠️ TODO: Background commands need zombie reaping later
 *
 * @param command  Command object to execute
 * @param pipeline Pipeline this command belongs to
 * @param jobs     Job table for tracking background processes
 * @param jobbed   Flag: has this pipeline been added to jobs yet?
 *                 (Set to 1 after first command adds it)
 * @param eof      EOF flag pointer (set by exit builtin)
 * @param fg       Foreground flag: 1=foreground (wait), 0=background (don't wait)
 */
extern void execCommand(Command command, Pipeline pipeline, Jobs jobs,
						int *jobbed, int *eof, int fg);

/**
 * Frees all memory associated with a Command
 *
 * Memory to free:
 * 1. Each individual string in argv array (allocated by strdup in getargs)
 * 2. The argv array itself (allocated in getargs)
 * 3. The CommandRep structure (allocated in newCommand)
 *
 * Note: r->file points to argv[0], so we don't free it separately
 *
 * @param command  Command to free
 */
extern void freeCommand(Command command);
/**
 * Frees static state variables for directory tracking
 *
 * The cwd and owd variables are static (persist across function calls).
 * This function cleans them up when the shell exits.
 *
 * Called from Shell.c before program termination.
 */
extern void freestateCommand();

#endif
