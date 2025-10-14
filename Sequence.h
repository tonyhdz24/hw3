#ifndef SEQUENCE_H
#define SEQUENCE_H

typedef void *Sequence;

#include "Jobs.h"
#include "Pipeline.h"

/**
 * @brief Creates a new empty Sequence
 *
 * A Sequence represents a series of Pipelines that should be executed
 * in order. For example, in the command "ls ; pwd & echo done", there
 * are three pipelines in the sequence.
 *
 * Implementation note: A Sequence is just a deque of Pipeline objects
 *
 * @return Sequence - A new empty sequence ready to have pipelines added
 */
extern Sequence newSequence();

/**
 * @brief Adds a Pipeline to the end of a Sequence
 *
 * When the parser builds the parse tree, it discovers pipelines one at
 * a time (left to right). Each discovered pipeline is added to the
 * sequence in order using this function.
 *
 * Example: For input "pwd ; ls ; echo hi"
 *   - addSequence is called with the "pwd" pipeline
 *   - addSequence is called with the "ls" pipeline
 *   - addSequence is called with the "echo hi" pipeline
 *
 * The sequence will then contain all three pipelines in order.
 *
 * @param sequence - The sequence to add to
 * @param pipeline - The pipeline to add to the end of the sequence
 *
 * @return void
 */
extern void addSequence(Sequence sequence, Pipeline pipeline);

/**
 * @brief Frees all memory associated with a Sequence
 *
 * This function deallocates:
 *   1. All Pipeline objects in the sequence (via freePipeline callback)
 *   2. The deque structure itself
 *
 * The freePipeline callback is passed to deq_del, which will call it
 * on each Pipeline in the sequence before freeing the deque.
 *
 * @param sequence - The sequence to free
 *
 * @return void
 */
extern void freeSequence(Sequence sequence);

/**
 * @brief Executes all pipelines in a sequence in order
 *
 * This is the main execution loop for the shell. It processes each
 * pipeline in the sequence one at a time, from first to last, until
 * either:
 *   1. The sequence is empty (all pipelines executed), OR
 *   2. The eof flag is set (user typed 'exit' or EOF)
 *
 * IMPORTANT: This function removes each pipeline from the sequence as
 * it executes (via deq_head_get), so the sequence is consumed during
 * execution. After this function completes, the sequence is freed.
 *
 * Execution Flow:
 *   1. Check if sequence has pipelines AND shell shouldn't exit
 *   2. Get and remove the first pipeline from the sequence
 *   3. Execute that pipeline (which may involve forking processes)
 *   4. Repeat until sequence is empty or exit requested
 *   5. Free the (now empty) sequence
 *
 * Example: For "ls ; pwd ; exit"
 *   - Loop iteration 1: execPipeline(ls), eof still 0
 *   - Loop iteration 2: execPipeline(pwd), eof still 0
 *   - Loop iteration 3: execPipeline(exit), eof becomes 1
 *   - Loop exits because eof is now 1
 *
 * @param sequence - The sequence of pipelines to execute
 * @param jobs - Job table for tracking background processes
 * @param eof - Pointer to flag indicating if shell should exit
 *              Set to non-zero by 'exit' builtin or EOF
 *
 * @return void
 *
 * @note This function always frees the sequence before returning
 * @note Foreground vs background execution is handled within execPipeline
 */
extern void execSequence(Sequence sequence, Jobs jobs, int *eof);

#endif
