/*-------------------------------------------------------------------------*/
/**
   @file    dictionary.c
   @author  N. Devillard
   @author  E.V. Emelianov
   @brief   Implements a dictionary for string variables.

   This module implements a simple dictionary object, i.e. a list
   of string/string associations. This object is useful to store e.g.
   informations retrieved from a configuration file (ini files).
*/
/*--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
                                Includes
 ---------------------------------------------------------------------------*/
#include "dictionary.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** Maximum value size for integers and doubles. */
#define MAXVALSZ    1024

/** Minimal allocated number of values in an entry */
#define ENTMINSZ    (10)

/** Minimal allocated number of entries in a dictionary */
#define DICTMINSZ   (5)

/** Invalid key token */
#define DICT_INVALID_KEY    ((char*)-1)

#ifdef DEBUG
#define DBG(...) do{ DBG(__VA_ARGS__); }while(0)
#else
#define DBG(...)
#endif

/**
    Static object used for more quick work.
    The idea is follow: usually user search objects in same section several
    times, so if we test section hash in search function with hash of last
    dictentry accessed, we can save a lot of time, especially in large
    ini-files.

*/
static dictentry * de_last = NULL;
static hash_t      hash_last = 0;

/*---------------------------------------------------------------------------
                            Private functions
 ---------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/**
  @brief    Enlarge memory for dictionary entry by ENTMINSZ values
  @param    e entry to grow
  @return   This function returns non-zero in case of failure
 */
/*--------------------------------------------------------------------------*/
static int dictentry_grow(dictentry * e)
{
    if(!e) return -2;
    size_t newlen = e->len + ENTMINSZ;
    keyval *new_k = realloc(e->kvlist, newlen * sizeof(keyval));
    /* An allocation failed, leave the entry unchanged */
    if(!new_k) return -1;
    e->kvlist = new_k;
    e->len = newlen;
    return 0;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Enlarge memory for dictionary by DICTMINSZ values
  @param    d dictionary to grow
  @return   This function returns non-zero in case of failure
 */
/*--------------------------------------------------------------------------*/
static int dictionary_grow(dictionary * d)
{
    if(!d) return -2;
    size_t newlen = d->len + DICTMINSZ;
    dictentry *new_e = realloc(d->entries, newlen * sizeof(dictentry));
    /* An allocation failed, leave the entry unchanged */
    if(!new_e) return -1;
    d->entries = new_e;
    d->len = newlen;
    return 0;
}


/*---------------------------------------------------------------------------
                            Function codes
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
hash_t dictionary_hash(const char * key)
{
    size_t      len ;
    hash_t      hash ;
    size_t      i ;

    if (!key)
        return 0 ;

    len = strlen(key);
    for (hash=0, i=0 ; i<len ; i++) {
        hash += (hash_t)key[i] ;
        hash += (hash<<10);
        hash ^= (hash>>6) ;
    }
    hash += (hash <<3);
    hash ^= (hash >>11);
    hash += (hash <<15);
    return hash ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Create a new dictionary object.
  @param    size    Optional initial size of the dictionary.
  @return   1 newly allocated dictionary objet.

  This function allocates a new dictionary object of given size and returns
  it. If you do not know in advance (roughly) the number of entries in the
  dictionary, give size=0.
 */
/*-------------------------------------------------------------------------*/
dictentry * dictentry_new(size_t size)
{
    dictentry  *   e ;

    /* If no size was specified, allocate space for DICTMINSZ */
    if (size<ENTMINSZ) size=ENTMINSZ ;

    e = (dictentry*) calloc(1, sizeof(dictentry)) ;

    if (e) {
        e->kvlist = calloc(size, sizeof(keyval));
        if(e->kvlist) e->len = size;
    }
    return e ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Create a new dictionary object.
  @param    size    Optional initial size of the dictionary.
  @return   1 newly allocated dictionary objet.

  This function allocates a new dictionary object of given size and returns
  it. If you do not know in advance (roughly) the number of entries in the
  dictionary, give size=0.
 */
/*-------------------------------------------------------------------------*/
dictionary * dictionary_new(size_t size)
{
    dictionary  *   d ;

    /* If no size was specified, allocate space for DICTMINSZ */
    if (size<DICTMINSZ) size=DICTMINSZ ;

    d = (dictionary*) calloc(1, sizeof(dictionary)) ;

    if (d) {
        d->entries = calloc(size, sizeof(dictentry));
        if(d->entries) d->len = size;
        d->noname = dictentry_new(0);
    }
    return d ;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a dictionary object
  @param    d   dictionary object to deallocate.
  @return   void

  Deallocate a dictionary object and all memory associated to it.
 */
/*--------------------------------------------------------------------------*/
void dictionary_del(dictionary * d)
{
    size_t  i, n;

    if (d==NULL) return ;
    n = d->n;
    dictentry_del(d->noname);
    for(i = 0; i < n; ++i)
        dictentry_del(&(d->entries[i]));
    free(d->entries);
    free(d->noname);
    free(d);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete a dictentry object
  @param    e   dictentry object to deallocate.
  @return   void

  Deallocate a dictentry object.
  WARNING! The dictentry e itself isn't deallocated!
 */
/*--------------------------------------------------------------------------*/
void dictentry_del(dictentry * e)
{
    size_t  i, n;
    if(!e) return;
    n = e->n;
    for(i = 0; i < n; ++i){
        keyval *k = &(e->kvlist[i]);
        if(k){
            free(k->key);
            free(k->val);
        }
    }
    free(e->kvlist);
    free(e->name);
}

static int iter = 0;

/*-------------------------------------------------------------------------*/
/**
  @brief    Find section in given dictionary
  @param    d       dictionary object to search.
  @param    name    Entry to look for in the dictionary.
  @return   pointer to entry or NULL

  This function locates a section in dictionary `d` and returns pointer to it
  or NULL if no entries found.
 */
/*--------------------------------------------------------------------------*/
dictentry * dictentry_find(const dictionary * d, const char * key){
    if(!d || !key || !d->entries) return NULL;
    dictentry *elist = d->entries;
    int i, L = (int)d->n, down = 0, up = L-1;
    hash_t hash = dictionary_hash(key);
    DBG("search entry %s (%u, last: %u [%s])\n", key, hash, hash_last, de_last ? de_last->name : "(null)");
iter = 0;
    if(hash_last == hash) return de_last;
    if(d->sorted){ // sorted dictionary - binary search
        while(down <= up){
++iter;
            i = (up + down)/2;
            if(elist[i].hash == hash){
            /* Compare string, to avoid hash collisions */
                if (!strcmp(key, elist[i].name)) {
                    de_last = &elist[i];
                    hash_last = de_last->hash;
                    return de_last;
                }else{ // maybe there's several entries with same hash?
                    while(i && elist[--i].hash == hash); // goto first entry with this hash
                    --L;
                    while(i < L && elist[++i].hash == hash){
                        if (!strcmp(key, elist[i].name)){
                            de_last = &elist[i];
                            hash_last = de_last->hash;
                            return de_last;
                        }
                    }
                    return NULL; // not found
                }
            }else if(elist[i].hash < hash) down = i + 1; // hash searched is in right half
            else up = i - 1; // hash searched is in left half
        }
    }else{ // unsorted - direct lookup
        for(i = 0; i < L; ++i){
++iter;
            if(elist[i].hash == hash){
            /* Compare string, to avoid hash collisions */
                if (!strcmp(key, elist[i].name)) {
                    de_last = &elist[i];
                    hash_last = de_last->hash;
                    return de_last;
                }
            }
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Find keyval object with given key name from a dictionary entry.
  @param    de      dictionary entry to search.
  @param    key     key to look for in the dictionary ("keyname").
  @return   pointer to keyval found or NULL

  This function locates a key in a dictionary entry and returns a pointer to
  keyval with given key value, or NULL if no such key can be found in.
 */
/*--------------------------------------------------------------------------*/
static keyval *keyval_find(const dictentry * de, const char * key)
{
    if(!de || !key) return NULL;
    hash_t hash = dictionary_hash(key);
    keyval *kvlist = de->kvlist;
    if(!kvlist) return NULL;
    int i, L = (int)de->n, down = 0, up = L-1;
iter = 0;
    if(de->sorted){ // sorted dictionary - binary search
        while(down <= up){
++iter;
            i = (up + down)/2;
            if(kvlist[i].hash == hash){
            /* Compare string, to avoid hash collisions */
                if (!strcmp(key, kvlist[i].key)) {
                    return &kvlist[i];
                }else{ // maybe there's several entries with same hash?
                    while(i && kvlist[--i].hash == hash); // goto first entry with this hash
                    --L;
                    while(i < L && kvlist[++i].hash == hash){
                        if (!strcmp(key, kvlist[i].key)){
                            return &kvlist[i];
                        }
                    }
                    return NULL; // not found
                }
            }else if(kvlist[i].hash < hash) down = i + 1; // hash searched is in right half
            else up = i - 1; // hash searched is in left half
        }
    }else{ // unsorted - direct lookup
        for(i = 0; i < L; ++i){
++iter;
            if(kvlist[i].hash == hash){
            /* Compare string, to avoid hash collisions */
                if (!strcmp(key, kvlist[i].key)){
                    return &kvlist[i];
                }
            }
        }
    }
    return NULL;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Get a value from a dictionary.
  @param    d       dictionary object to search.
  @param    key     Key to look for in the dictionary ("entryname:keyname").
  @param    def     Default value to return if key not found.
  @return   1 pointer to internally allocated character string.

  This function locates a key in a dictionary and returns a pointer to its
  value, or the passed 'def' pointer if no such key can be found in
  dictionary. The returned character pointer points to data internal to the
  dictionary object, you should not try to free it or modify it.
  Value of key have format "entry:name" for key inside given entry, or just
  "name" for keys from unnamed entry (d->noname).
 */
/*--------------------------------------------------------------------------*/
const char * dictionary_get(const dictionary * d, const char * key, const char * def)
{
    char *delim, *str = strdup(key), *k = NULL;
    const char *ret = def;
    dictentry *de = NULL;

    if((delim = strchr(str, ':'))){
        *delim++ = 0;
        k = delim;
        de = dictentry_find(d, str);
    DBG("de found by %d steps\n", iter);
    }else{
        k = str;
        de = d->noname;
    }
    if(!de){
        goto rtn;
    }
    DBG("de name: %s\n", de->name);
    keyval *kv = keyval_find(de, k);
    DBG("kv %s found by %d steps\n", kv ? "" : "not", iter);
    if(kv) ret = kv->val;
    rtn:
    free(str);
    return ret;
}


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

  DELETED objects leaved filled with zeros!
 */
/*--------------------------------------------------------------------------*/
int dictionary_set(dictionary * d, const char * key, const char * val)
{
    hash_t hash ;
    keyval * kv = NULL;
    dictentry *de = NULL;
    if (d==NULL || key==NULL) return -1 ;
    DBG("set %s to %s\n", key, val);
    char *dup = strdup(key), *delim = strchr(dup, ':');
    if(delim){
        *delim++ = 0;
        key = (const char*) delim;
    }else{ // user give section or global parameter name
        if(!val){ // remove whole section?
            if((de = dictentry_find(d, dup))){
                dictentry_del(de);
                memset(de, 0, sizeof(dictentry));
                d->sorted = 0;
                free(dup);
                return 0;
            }
        }
    }
    /* Find if value is already in dictionary */
    if(delim)
        de = dictentry_find(d, dup); // section
    else de = d->noname; // global
    DBG("de name: %s\n", de ? de->name : "not found");
    if(de){
        if((kv = keyval_find(de, key))){ // key found - just change its value
            free(kv->val);
            if(!val){ // erase object
                free(kv->key);
                memset(kv, 0, sizeof(keyval));
                de->sorted = 0;
            }else
                kv->val = strdup(val);
            free(dup);
            return 0;
        }
    }
    /* Not found: add a new value. First check for entries */
    if(!val) return 0; // no key for erasing === we already erase it
    hash = dictionary_hash(key);
    if(!de){ // there's no entry for given key
        if(delim){ // this key should be stored in named entry - create it
            d->sorted = 0; // newly created entry breaks sort order
            /* See if dictionary needs to grow */
            if (d->n == d->len)
                if (dictionary_grow(d)){
                    free(dup);
    DBG("can't enlarge directory size!\n");
                    return -1;
                }
            de = &d->entries[d->n++];
            de->name = strdup(dup);
            de->hash = dictionary_hash(dup);
    DBG("new record: %s with hash %u\n", de->name, de->hash);
        }else // global section
            de = d->noname;
    }
    hash_last = de->hash;
    de_last = de;
    de->sorted = 0; // we broke sort order
    /* See if dictentry needs to grow */
    if(de->n == de->len)
        if(dictentry_grow(de)){
            free(dup);
            return -1;
        }
    kv = &de->kvlist[de->n++];
    kv->key = strdup(key);
    kv->val = strdup(val);
    kv->hash = hash;
    DBG("new key: %s with hash %u & value %s\n", kv->key, kv->hash, kv->val);
    free(dup);
    return 0 ;
}

void dictentry_dump(const dictentry *de, FILE *out){
    if(!de || !out) return;
    keyval *kv = de->kvlist;
    size_t i, n = de->n;
    if(!kv || !n) return; // empty dictentry
    for(i = 0; i < n; ++i, ++kv)
        if(kv->key) // not empty key/val
            fprintf(out, "%-30s = %s\n", kv->key, kv->val);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump a dictionary to an opened file pointer.
  @param    d   Dictionary to dump
  @param    f   Opened file pointer.
  @return   void

  Dumps a dictionary onto an opened file pointer, creating ini-file.
 */
/*--------------------------------------------------------------------------*/
dicterr_t dictionary_dump(const dictionary * d, FILE * out)
{
    size_t  i, n;

    if (d==NULL || out==NULL) return DERR_BADDATA;
    if ((n = d->n) < 1) return DERR_EMPTY;
    dictentry_dump(d->noname, out); // unsectioned data
    dictentry *de = d->entries;
    for(i = 0; i < n; ++i, ++de){ // dump all sections
        if(!de->n) continue; // deleted section
        fprintf(out, "\n[%s]\n", de->name); // print section name
        dictentry_dump(de, out);
    }
    return DERR_OK;
}

/** Compare keyvals in dictentry (by hash) */
static int cmpvals(const void *p1, const void *p2){
    hash_t h1 = ((keyval*)p1)->hash, h2 = ((keyval*)p2)->hash;
    if(h1 < h2) return -1;
    else if(h1 > h2) return 1;
    else return 0;
}

/** Compare dictentries in dictionary (by hash) */
static int cmpentries(const void *p1, const void *p2){
    hash_t h1 = ((dictentry*)p1)->hash, h2 = ((dictentry*)p2)->hash;
    if(h1 < h2) return -1;
    else if(h1 > h2) return 1;
    else return 0;
}

/** Compare keyvals in dictentry (by names) */
static int cmpvalnm(const void *p1, const void *p2){
    char *ch1 = ((keyval*)p1)->key, *ch2 = ((keyval*)p2)->key;
    if(ch1 && ch2) return strcmp(ch1, ch2);
    else return 0; // equal - empty objects are non-shown
}

/** Compare dictentries in dictionary (by names) */
static int cmpentrienm(const void *p1, const void *p2){
    char *ch1 = ((dictentry*)p1)->name, *ch2 = ((dictentry*)p2)->name;
    if(ch1 && ch2) return strcmp(ch1, ch2);
    else return 0;
}

/** Sort key/value pairs in dictionary section */
void dictentry_sort(dictentry * de){
    if(!de || !de->n) return;
    if(de->sorted) return;
    qsort((void*)de->kvlist, de->n, sizeof(keyval), cmpvals);
    de->sorted = 1;
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Sort objects stored in dictionary for quick binary search.
  @param    d   Dictionary to sort
  @return   void

  Sort all records in dictionary by their hash.
 */
/*--------------------------------------------------------------------------*/
void dictionary_sort_hash(dictionary * d){
    if(!d) return;
    dictentry_sort(d->noname);
    size_t i, n = d->n;
    dictentry *de = d->entries;
    for(i = 0; i < n; ++i, ++de)
        dictentry_sort(de);
    qsort((void*)d->entries, d->n, sizeof(dictentry), cmpentries);
    d->sorted = 1;
}

/** Sort key/value pairs in dictionary section */
void dictentry_sort_nm(dictentry * de){
    if(!de || !de->n) return;
    qsort((void*)de->kvlist, de->n, sizeof(keyval), cmpvalnm);
}

/*-------------------------------------------------------------------------*/
/**
  @brief    Sort objects stored in dictionary by their names.
  @param    d   Dictionary to sort
  @return   void

  Sort all sections in dictionary & all records in sections by their names.
  Usefull for saving ini files.
 */
/*--------------------------------------------------------------------------*/
void dictionary_sort(dictionary * d){
    if(!d) return;
    dictentry_sort_nm(d->noname);
    size_t i, n = d->n;
    dictentry *de = d->entries;
    for(i = 0; i < n; ++i, ++de)
        dictentry_sort_nm(de);
    qsort((void*)d->entries, d->n, sizeof(dictentry), cmpentrienm);
}
