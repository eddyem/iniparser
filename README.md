# Iniparser 5 #


## I - Overview

This modules offers parsing of ini files from the C level.
See a complete documentation in HTML format, from this directory
open the file html/index.html with any HTML-capable browser.

Key features :

 - Small : around 1800 lines inside 4 files (2 .c and 2 .h)
 - Portable : no dependancies, written in `-ansi -pedantic` C99
 - Partly reentrant : last error codes stored in global variables

## II - Building project

A simple `make` at the root of the project should be enough to get the static
(i.e. `libiniparser.a`) and shared (i.e. `libiniparser.so.0`) libraries compiled.

You should consider trying the following rules too :

  - `make example` : compile the example, run it with `./example/iniexample`

## III - License

This software is released under MIT/GPL License.
See LICENSE for full informations

## IV - Versions

Current version is 5.0 which introduces breaking changes in the api.

## V - FAQ

### Is Iniparser thread safe ?

Starting from version 5, iniparser is designed to be partially thread-safe, provided you surround it with your own mutex logic.
For thread-safety you shouldn't use functions `get_error()` and `get_errmsg()`. Also search would be slower.

### Your build system isn't portable, let me help you...

I have received countless contributions from distrib people to modify the Makefile into what they think is the "standard", which I had to reject.
The default, standard Makefile for Debian bears absolutely no relationship with the one from SuSE or RedHat and there is no possible way to merge them all.
A build system is something so specific to each environment that it is completely pointless to try and push anything that claims to be standard. The provided Makefile in this project is purely here to have something to play with quickly.

## VI - New in version 5.0

These things are added by E.Emelianov forming ver.5.

  - Keys are stored now in different data structures, according to their sections' names.
  - Memory for sections and keys now not redoubles but increased by constant size.
  - To remove records and sections use same function `iniparser_set()` with NULL in `val`.
  - For working with large ini files I add binary search in sorted (by hash) lists. To sort dictionary use function `iniparser_sort_hash()`.
  - Sort by keyword & section names for pretty output by `iniparser_sort()`.
  - Very often user works with same section many times (read/add/modify keys inside single section), so I add global variable storing last accessed section.

