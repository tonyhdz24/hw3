#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Parser.h"
#include "Tree.h"
#include "Scanner.h"
#include "error.h"

static Scanner scan;

#undef ERROR
#define ERROR(s) ERRORLOC(__FILE__, __LINE__, "error", "%s (pos: %d)", s, posScanner(scan))

// Scanner Integreation
// Utilizes Scanner functionality to tokenize input command string
static char *next() { return nextScanner(scan); }       // Get next token and advances Scanner
static char *curr() { return currScanner(scan); }       // Gets current token without advancing Scanner
static int cmp(char *s) { return cmpScanner(scan, s); } // Compares a given string with current token does not advance
static int eat(char *s) { return eatScanner(scan, s); } // Compares a given string with current token advance if equal

// Grammer Integration
// Based on the provided grammar each rule gets its own parser
static T_word p_word();   // Parses word
static T_words p_words(); // Parses words
static T_redir p_redir();
static T_command p_command();   // Parses commands
static T_pipeline p_pipeline(); // Parses pipeline
static T_sequence p_sequence(); // Parses sequence

/**
 * @brief Parses a single word token from the input stream
 *
 * Extracts the current token from the scanner and creates a T_word node containing a copy of the token string. Advancing the scanner to next token
 *
 * @return T_word node containg the parsed word or NULL if not token available
 */
static T_word p_word()
{
  // Getting the current token
  char *s = curr();
  if (!s)
    return 0;
  // Create T_word node
  T_word word = new_word();

  // Set T_word node to be the token
  word->s = strdup(s);

  // Advance scanner
  next();
  return word;
}

/**
 * @brief Parses multiple words
 *  Recusively parses multiple words until a shell operator is encountered. Creates a linked list of T_words nodes where each node contains one word and optionally points to the next words in the sequence
 * @return T_words a linked list of parsed words
 */
static T_words p_words()
{
  // Get first word
  T_word word = p_word();
  // If word NULL return ERROR
  if (!word)
    return 0;
  // Create new Tree words node
  T_words words = new_words();

  // Set T_words word attribute to be equal to word parsed
  words->word = word;

  // If any operators encountered return words
  // Grammar states that words can only be word or words word
  if (cmp("|") || cmp("&") || cmp(";") || cmp("<") || cmp(">"))
    return words;

  // Recursively parse more words
  // set T_words words attriubute to be the next word in the sequence
  words->words = p_words();
  return words;
}

/**
 * @brief Parses a single command consisting of a program name and arguments
 *
 * A command is composed of one or more words with a possible redir (<,>). Creates a T_command node that wraps the parsed words
 *
 * MORE INFO grammar says that a command can contain a redir but Tree.[hc] does not support functionality for that
 *
 * @return T_command node containing the parsed command
 */
static T_command p_command()
{
  // Instantziate words
  T_words words = 0;
  // Set words node to be result of parsing words
  words = p_words();
  if (!words)
    return 0;
  // Create T_command node
  T_command command = new_command();
  // Set T_command words attribute to be the parsed words
  command->words = words;
  command->redir = p_redir();
  return command;
}

/**
 * @brief Parse a pipeline of commands connected by a pipe operator
 *
 * Handles commands connected via pipes for I/O. Recursively parses multiple piped commands creating a linked list structure where each pipeline node contains a command and may point to the next pipeline segment
 *
 * @return T_pipeline linked list of piped commands
 */
static T_pipeline p_pipeline()
{
  T_command command = p_command();
  if (!command)
    return 0;
  T_pipeline pipeline = new_pipeline();
  pipeline->command = command;
  if (eat("|"))
    pipeline->pipeline = p_pipeline();
  return pipeline;
}

/**
 * @brief Parses a sequence of pipelines connected by control operators
 *
 * Top-level parsing function that handles command sequences with:
 * - & operator: run pipeline in background and continue
 * - ; operator: run pipeline and wait for completion before continuing
 * Creates the root of the parse tree structure.
 *
 * @return root node of the parse tree
 */
static T_sequence p_sequence()
{
  T_pipeline pipeline = p_pipeline();
  if (!pipeline)
    return 0;
  T_sequence sequence = new_sequence();
  sequence->pipeline = pipeline;
  if (eat("&"))
  {
    sequence->op = "&";
    sequence->sequence = p_sequence();
  }
  if (eat(";"))
  {
    sequence->op = ";";
    sequence->sequence = p_sequence();
  }
  return sequence;
}

/**
 * @brief Main entry point for parsing a shell command string
 *
 * Intitializes the scanner with the input string parses it inot a tree structure and validates that all the input ahs been consumed. Resulting tree represents the structure of the shell command with sequences, pipelines, commands and words
 *
 * @param s Input string containing shell commands to parse
 * @return Tree (opaque pointer to T_sequence) representing the parse tree
 */
extern Tree parseTree(char *s)
{
  scan = newScanner(s);

  Tree tree = p_sequence();
  if (curr())
    ERROR("extra characters at end of input");
  freeScanner(scan);
  return tree;
}

// Memory Managment
static void f_word(T_word t);
static void f_words(T_words t);
static void f_command(T_command t);
static void f_pipeline(T_pipeline t);
static void f_sequence(T_sequence t);

static void f_redir(T_redir t)
{
  if (!t)
    return;
  if (t->input)
    free(t->input); // Free input filename
  if (t->output)
    free(t->output); // Free output filename
  free(t);
}
static void f_word(T_word t)
{
  if (!t)
    return;
  if (t->s)
    free(t->s);
  free(t);
}

static void f_words(T_words t)
{
  if (!t)
    return;
  f_word(t->word);
  f_words(t->words);
  free(t);
}

static void f_command(T_command t)
{
  if (!t)
    return;
  f_words(t->words);
  free(t);
}

static void f_pipeline(T_pipeline t)
{
  if (!t)
    return;
  f_command(t->command);
  f_pipeline(t->pipeline);
  free(t);
}

static void f_sequence(T_sequence t)
{
  if (!t)
    return;
  f_pipeline(t->pipeline);
  f_sequence(t->sequence);
  free(t);
}

extern void freeTree(Tree t)
{
  f_sequence(t);
}

static T_redir p_redir()
{
  T_redir redir = new_redir();
  redir->input = NULL;
  redir->output = NULL;

  // Parse < word (input redirection)
  if (eat("<"))
  {
    T_word word = p_word();
    if (!word)
      ERROR("expected filename after <");
    redir->input = strdup(word->s); // Save filename
    free(word->s);
    free(word);
  }

  // Parse > word (output redirection)
  if (eat(">"))
  {
    T_word word = p_word();
    if (!word)
      ERROR("expected filename after >");
    redir->output = strdup(word->s); // Save filename
    free(word->s);
    free(word);
  }

  return redir;
}