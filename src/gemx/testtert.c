
#include <stdio.h>
#include <windows.h>

#include "terttree.h"


#define NODEMAX  130000
#define iMax     65535
//#define NODEMAX  2000000
//#define iMax     1000000

PTN ptnRoot;



void main()
{
    int i;
    char rgch[256];
    char *pch;

    ptnRoot = PtnCreateTertiaryTree(NODEMAX);

    for (i = 0; i < iMax; i++)
        {
        sprintf(rgch, "%d", i);
        
        if (!FInsertPtnPchPv(ptnRoot, rgch, strdup(rgch)))
            printf("ERROR INSERTING: %s!!!\n", rgch);
        }

    printf("nodes / strings = %d / %d\n", ptnRoot->ptnLo - ptnRoot, ptnRoot->ptnEq->cnt);

    for (i = 0; i < iMax; i++)
        {
        sprintf(rgch, "%d", i);
        
        pch = PvSearchPtnPch(ptnRoot, rgch);

        if (!pch)
            printf("STRING NOT FOUND: %s\n", rgch);
        else
            printf("FOUND STRING %s: %s\n", rgch, pch);
        }

    for (i = 0; i < iMax; i++)
        {
        memset(rgch, 0, sizeof(rgch));

        pch = PvGetIthNodePtn(ptnRoot, i, rgch);

        if (!pch)
            printf("%dth NODE NOT FOUND\n", i);
        else
            printf("%dth NODE = %s: %s\n", i, rgch, pch);
        }

}


