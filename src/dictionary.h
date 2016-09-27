
/*-------------------------------------------------------------------------*/
/**
   @file    dictionary.h
   @author  N. Devillard
   @brief   Implements a dictionary for string variables.

   This module implements a simple dictionary object, i.e. a list
   of string/string associations. This object is useful to store e.g.
   informations retrieved from a configuration file (ini files).
*/
/*--------------------------------------------------------------------------*/

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/*---------------------------------------------------------------------------
                                New types
 ---------------------------------------------------------------------------*/

typedef uint32_t hash_t; /** hash is 32 bit unsigned */

/*-------------------------------------------------------------------------*/
/**
  @brief    Key/value pair with hash

  This object contains a pair key/value with hash. Looking up values
  in the dictionary is speeded up by the use of a (hopefully collision-free)
  hash function.
 */
/*-------------------------------------------------------------------------*/
typedef struct {
    char         *  key ;   /** Key name */
    char         *  val ;   /** Key value */
    hash_t          hash ;  /** Hash of key name */
} keyval;


/*-------------------------------------------------------------------------*/
/**
  @brief    Dictionary entry object

  This object contains a list of key/value pairs for given dictionary entry.
  Each entry also (as each keyval) have hash value.
 */
/*-------------------------------------------------------------------------*/
typedef struct {
    size_t          n ;     /** Number of pairs in object */
    size_t          len ;   /** amount of memory allocated for kvlist (if n == len, grow entry size) */
    keyval       *  kvlist ;/** list of key/value pairs */
    int             sorted ;/** ==1 if kvlist sorted */
    char         *  name;   /** entry name */
    hash_t          hash ;  /** Hash of entry name */
} dictentry;


/*-------------------------------------------------------------------------*/
/**
  @brief    Dictionary object

  This object contains a list of all entries in given ini file.
 */
/*-------------------------------------------------------------------------*/
typedef struct _dictionary_ {
    size_t          n ;     /** Number of named entries in dictionary */
    size_t          len ;   /** amount of memory allocated for entries (if n == len, grow dictionary size) */
    dictentry    *  noname ;/** Unnamed entry (key/value pairs outside of any named block) */
    dictentry    *  entries;/** List of entries in dictionary */
    int             sorted ;/** ==1 if all entries are sorted */
} dictionary ;


/*---------------------------------------------------------------------------
                            Function prototypes
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  @brief    Compute the hash key for a string.
  @param    key     Character string to use for key.
  @return   1 unsigned int on at least 32 bits.

  This hash function has been taken from an Article in Dr Dobbs Journal.
  This is normally a collision-free function, distributing keys evenly.
  The key is stored anyway in the struct so that collision can be avoided
  by comparing the key itself in last resort.
 */
/*--------------------------------------------------------------------------*/
hash_t dictionary_hash(const char * key);

/*-------------------------------------------------------------------------*/
/**
  @brief    Create a new dictionary & dictentry object.
  @param    size    Optional initial size of the dictionary.
  @return   1 newly allocated object.

  This function allocates a new dictionary/dictentry object of given size and
  returns it. If you do not know in advance (roughly) the number of entries
  in the dictionary, give size=0.
 */
/*--------------------------------------------------------------------------*/
dictionary * dictionary_new(size_t size);
dictentry * dictentry_new(size_t size);

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a dictionary/dictentry object
  @param    o   object to deallocate.
  @return   void

  Deallocate a dictionary/dictentry object and all memory associated to it.
 */
/*--------------------------------------------------------------------------*/
void dictionary_del(dictionary * o);
void dictentry_del(dictentry * o);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get a value from a dictionary.
  @param    d       dictionary object to search.
  @param    key     Key to look for in the dictionary.
  @param    def     Default value to return if key not found.
  @return   1 pointer to internally allocated character string.

  This function locates a key in a dictionary and returns a pointer to its
  value, or the passed 'def' pointer if no such key can be found in
  dictionary. The returned character pointer points to data internal to the
  dictionary object, you should not try to free it or modify it.
 */
/*--------------------------------------------------------------------------*/
const char * dictionary_get(const dictionary * d, const char * key, const char * def);

/*-------------------------------------------------------------------------*/
/**
  @brief    Find a section from given dictionary.
  @param    d       dictionary object to search.
  @param    key     Entry name to look for in the dictionary.
  @return   1 pointer to internally allocated character string.

  This function locates a section in dictionary `d` and returns pointer to it
  or NULL if no entries found.
 */
/*--------------------------------------------------------------------------*/
dictentry * dictentry_find(const dictionary * d, const char * key);

/*-------------------------------------------------------------------------*/
/**
  @brief    Set a value in a dictionary.
  @param    d       dictionary object to modify.
  @param    key     Key to modify or add ("entryname:keyname").
  @param    val     Value to add.
  @return   int     0 if Ok, anything else otherwise

  If the given key is found in the dictionary, the associated value is
  replaced by the provided one. If the key cannot be found in the
  dictionary, it is added to it.

  If key have format "keyname" it will be stored in d->noname entry. If it have
  format "entryname:keyname", it will be stored in entry "entryname" (if found,
  in existing entry; if not found, in created one).
  When create an entry d->sorted will be reset to 0.
  If "key" not found in corresponding entry, value "sorted" of this entry also
  will be reset to 0.

  It is Ok to provide a NULL value for val, but NULL values for the dictionary
  or the key are considered as errors: the function will return immediately
  in such a case.

  If value of `val` is NULL, this function erase given key from dictionary.

  This function returns non-zero in case of failure.
 */
/*--------------------------------------------------------------------------*/
int dictionary_set(dictionary * vd, const char * key, const char * val);


typedef enum{
    DERR_OK = 0,    // all OK
    DERR_BADDATA,   // bad arguments of function (NULL instead of data)
    DERR_EMPTY      // empty dictionary
} dicterr_t;

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump
  @param    f   Opened file pointer.
  @return   0 or error code

  Dumps a dictionary onto an opened file pointer. Key pairs are printed out
  as @c [Key]=[Value], one per line. It is Ok to provide stdout or stderr as
  output file pointers.
 */
/*--------------------------------------------------------------------------*/
dicterr_t dictionary_dump(const dictionary * d, FILE * out);
void dictentry_dump(const dictentry *de, FILE *out);

/*-------------------------------------------------------------------------*/
/**
  @brief    Sort objects stored in dictionary for quick binary search.
  @param    d   Dictionary to sort
  @return   void

  Sort all records in dictionary by their hash.
 */
/*--------------------------------------------------------------------------*/
void dictionary_sort_hash(dictionary *d);

/** Sort by names */
void dictionary_sort(dictionary *d);

#ifdef __cplusplus
}
#endif

#endif
