#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Scanner.h"
#include "error.h"

typedef struct
{
  // End of string flag 1 = no more tokens
  int eos;
  // Copy of original string
  char *str;
  // Current position in the string
  char *pos;
  // Current token
  char *curr;
} *ScannerRep; // Private: Internal Representation

extern Scanner newScanner(char *s)
{
  ScannerRep r = (ScannerRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->eos = 0;
  r->str = strdup(s);
  r->pos = r->str;
  r->curr = 0;
  return r;
}

extern void freeScanner(Scanner scan)
{
  ScannerRep r = scan;
  free(r->str);
  if (r->curr)
    free(r->curr);
  free(r);
}
/**
 * Adcances pointer (p) through any characters found in string (q) stops at first character not in q
 */
static char *thru(char *p, char *q)
{
  for (; *p && strchr(q, *p); p++)
    ;
  return p;
}
/**
 * Advances pointer p up to any character found in string q
Stops at first character that IS in q
 */
static char *upto(char *p, char *q)
{
  for (; *p && !strchr(q, *p); p++)
    ;
  return p;
}

static char *wsthru(char *p) { return thru(p, " \t"); }
static char *wsupto(char *p) { return upto(p, " \t"); }

extern char *nextScanner(Scanner scan)
{
  ScannerRep r = scan;
  // Check if scanner is at end of string
  if (r->eos)
    return 0;

  // At the current position Skip through any whitespace
  char *old = wsthru(r->pos);
  // From position(not a whitespace) Finds next whitespace
  char *new = wsupto(old);
  // Size is equal to the number of tokens in the input string
  int size = new - old;

  // Checking if there are tokens
  if (size == 0)
  {
    r->eos = 1;
    return 0;
  }

  if (r->curr)
    free(r->curr);

  r->curr = (char *)malloc(size + 1);

  if (!r->curr)
    ERROR("malloc() failed");

  memmove(r->curr, old, size);

  (r->curr)[size] = 0;
  r->pos = new;
  return r->curr;
}

extern char *currScanner(Scanner scan)
{
  ScannerRep r = scan;
  // Check if at the end of the string
  if (r->eos)
    return 0;
  // Return current token if valid
  if (r->curr)
    return r->curr;
  // Call nextScanner to get next token (might happen if scanner is somehow before start of string)
  return nextScanner(scan);
}

extern int cmpScanner(Scanner scan, char *s)
{
  ScannerRep r = scan;

  // Get the current token
  currScanner(scan);
  // Check if at the end of the string
  if (r->eos)
    return 0;
  // call strcmp to compare the current token to given string
  if (strcmp(s, r->curr))
    return 0;
  return 1;
}

extern int eatScanner(Scanner scan, char *s)
{
  // Call cmpScanner to check equality of current token and given string
  int r = cmpScanner(scan, s);
  // If there is a match call nextScanner to advance scanner
  if (r)
    nextScanner(scan);
  return r;
}

extern int posScanner(Scanner scan)
{
  ScannerRep r = scan;
  return (r->pos) - (r->str);
}
