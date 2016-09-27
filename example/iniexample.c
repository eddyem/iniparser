#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"

void create_example_ini_file(void);
int  parse_ini_file(char * ini_name);

int main(int argc, char * argv[])
{
    int     status ;

    if (argc<2) {
        create_example_ini_file();
        status = parse_ini_file("example.ini");
    } else {
        status = parse_ini_file(argv[1]);
    }
    return status ;
}

void create_example_ini_file(void)
{
    FILE    *   ini ;

    if ((ini=fopen("example.ini", "w"))==NULL) {
        fprintf(stderr, "iniparser: cannot create example.ini\n");
        return ;
    }

    fprintf(ini,
    "#\n"
    "# This is an example of ini file\n"
    "#\n"
    "\n"
    "globval1  = \"global value 1\"\n"
    "Table:cup = 3; This will create section Table with key cup\n"
    "globval2  = \"global value 2\"\n"
    "[Pizza]\n"
    "\n"
    "Ham       = yes ;\n"
    "Mushrooms = TRUE ;\n"
    "Capres    = 0 ;\n"
    "Cheese    = Non ;\n"
    "Fish      = no\n"
    "Parrots   = no\n"
    "Monkeys   = no\n"
    "Humans    = no\n"
    "Something bad = no\n\n"
    "[Wine]\n\n"
    "Grape     = Cabernet Sauvignon ;\n"
    "Year      = 1989 ;\n"
    "Country   = Spain ;\n"
    "Alcohol   = 12.5  ;\n\n"
    "[Table]\n\n"
    "Spoon     = 5\n"
    "Fork      = 5\n"
    "Knife     = 1\n"
    "Plate     = 8\n\n"
    );
    fclose(ini);
}


int parse_ini_file(char * ini_name)
{
    dictionary  *   ini ;

    /* Some temporary variables to hold query results */
    int             b ;
    int             i ;
    double          d ;
    const char  *   s ;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        fprintf(stderr, "%s\n", get_errmsg());
        return -1 ;
    }
    iniparser_dump(ini, stderr);

    /* Get pizza attributes */
    printf("\n\nPizza:\n");

    b = iniparser_getboolean(ini, "pizza:ham", -1);
    printf("Ham:       [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:mushrooms", -1);
    printf("Mushrooms: [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:capres", -1);
    printf("Capres:    [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:cheese", -1);
    printf("Cheese:    [%d]\n", b);

    /* Get wine attributes */
    printf("Wine:\n");
    s = iniparser_getstring(ini, "wine:grape", NULL);
    printf("Grape:     [%s]\n", s ? s : "UNDEF");

    i = iniparser_getint(ini, "wine:year", -1);
    printf("Year:      [%d]\n", i);

    s = iniparser_getstring(ini, "wine:country", NULL);
    printf("Country:   [%s]\n", s ? s : "UNDEF");

    d = iniparser_getdouble(ini, "wine:alcohol", -1.0);
    printf("Alcohol:   [%g]\n", d);

    i = iniparser_getint(ini, "spaceman", -1);
    printf("Spaceman:  [%d]\n", i);



    // add something
    printf("\n\nChange something\n");
    if(iniparser_set(ini, "Pizza:pepper", "yes")) printf("Can't add pepper into pizza :(\n");
    // and global objects
    if(iniparser_set(ini, "spaceman", "4") ||
        iniparser_set(ini, "Pokemon", "no") ||
        iniparser_set(ini, "Big and fat man", "yes") ||
        iniparser_set(ini, "Stonehenge", "1"))
            printf("Can't add global keys :(\n");
    // remove record
    if(iniparser_set(ini, "Pizza:humans", NULL)) printf("Can't remove humans from pizza :(\n");
    // remove section
    if(iniparser_set(ini, "Table", NULL)) printf("Can't remove table :(\n");



    // now sort & try again
    iniparser_sort_hash(ini);
    printf("\n\n\nNow sorted by hash\n\n");
    iniparser_dump(ini, stderr);

    /* Get pizza attributes */
    printf("\n\nPizza:\n");

    b = iniparser_getboolean(ini, "pizza:ham", -1);
    printf("Ham:       [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:mushrooms", -1);
    printf("Mushrooms: [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:capres", -1);
    printf("Capres:    [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:cheese", -1);
    printf("Cheese:    [%d]\n", b);

    /* Get wine attributes */
    printf("Wine:\n");
    s = iniparser_getstring(ini, "wine:grape", NULL);
    printf("Grape:     [%s]\n", s ? s : "UNDEF");

    i = iniparser_getint(ini, "wine:year", -1);
    printf("Year:      [%d]\n", i);

    s = iniparser_getstring(ini, "wine:country", NULL);
    printf("Country:   [%s]\n", s ? s : "UNDEF");

    d = iniparser_getdouble(ini, "wine:alcohol", -1.0);
    printf("Alcohol:   [%g]\n", d);

    s = iniparser_getstring(ini, "wine:volume", NULL);
    printf("Volume:    [%s]\n", s ? s : "UNDEF");

    i = iniparser_getint(ini, "spaceman", -1);
    printf("Spaceman:  [%d]\n", i);


    // Now sort all by names for pretty dump
    iniparser_sort(ini);
    printf("\n\n\nAnd sorted by names\n\n");
    iniparser_dump(ini, stderr);

    iniparser_freedict(ini);
    return 0 ;
}


