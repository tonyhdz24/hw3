#ifndef SCANNER_H
#define SCANNER_H

// Public users see it as only an opaque pointer and cannot see the the internal structure for encapsulations purposes.
typedef void *Scanner;

/**
 * @brief Creates a new scanner
 *
 * @param s String to be parsed using the scanner
 * @return A new Scanner ready to parse
 */
extern Scanner newScanner(char *s);

/**
 * @brief Cleans up Scanner, freeing all allocated memory
 * @param scan Scanner object
 * @return void
 */
extern void freeScanner(Scanner scan);

/**
 * @brief Get next token and advances the scanner
 * @param scan Scanner object
 * @return Next token in the string
 */
extern char *nextScanner(Scanner scan);

/**
 * @brief Peek at the current Token with out advancing the scanner
 * @param scan Scanner object
 * @return Current token
 */
extern char *currScanner(Scanner scan);

/**
 * @brief Compares current token to a given string. Does not advance the scanner
 * @param scan Scanner object
 * @param s Token string to be compared to current token
 * @return int representing the equality of the two strings. 1 if a match and 0 if not
 */
extern int cmpScanner(Scanner scan, char *s);

/**
 * @brief Compares current token to a given string. If equal advance the scanner
 * @param scan Scanner object
 * @param s Token string to be compared to current token
 * @return int representing the equality of the two strings. 1 if a match and 0 if not
 */
extern int eatScanner(Scanner scan, char *s);

/**
 * @brief Get Current Position
 * @param scan Scanner object
 * @return Number of characters into the originial string the scanner is
 */
extern int posScanner(Scanner scan);

#endif
