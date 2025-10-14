#include "Sequence.h"
#include "deq.h"
#include "error.h"

extern Sequence newSequence()
{
  return deq_new();
}

extern void addSequence(Sequence sequence, Pipeline pipeline)
{
  deq_tail_put(sequence, pipeline);
}

extern void freeSequence(Sequence sequence)
{
  deq_del(sequence, freePipeline);
}

extern void execSequence(Sequence sequence, Jobs jobs, int *eof)
{
  // Continue processing pipelines while:
  //   - The sequence still has pipelines (deq_len(sequence) > 0)
  //   - AND the user hasn't requested to exit (!*eof)
  while (deq_len(sequence) && !*eof)
  {
    // Get the first pipeline from the sequence (removes it from deque)
    // and execute it. execPipeline handles:
    //   - Forking child processes
    //   - Waiting (if foreground) or not waiting (if background)
    //   - Setting up pipes between commands
    //   - Job management
    execPipeline(deq_head_get(sequence), jobs, eof);
  }
  // Clean up: Free the sequence and any remaining pipelines
  freeSequence(sequence);
}
