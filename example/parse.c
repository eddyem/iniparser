#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"

int main(int argc, char * argv[])
{
    dictionary * ini ;
    char       * ini_name ;

    if (argc<2) {
        ini_name = "twisted.ini";
    } else {
        ini_name = argv[1] ;
    }

    ini = iniparser_load(ini_name);
    if(!ini){
        fprintf(stderr, "Error: %s\n", get_errmsg());
        return 1;
    }
    iniparser_sort(ini);
    iniparser_dump(ini, stdout);
    iniparser_freedict(ini);
    return 0 ;
}
