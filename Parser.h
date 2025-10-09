#ifndef PARSER_H
#define PARSER_H

// Opaque void pointer to impelement encapsulation
typedef void *Tree;

/**
 * @brief
 *
 * @param s Input string that will be scanned using Scanner.c and then parsed creating a parse tree
 * @return Tree - The final build parse tree created from the input command string
 */
extern Tree parseTree(char *s);

/**
 * @brief Clean up function that will free the Tree returned from parseTree() from memory
 * @param Tree - Parse tree returned from parseTree()
 * @return VOID
 */
extern void freeTree(Tree t);

#endif
