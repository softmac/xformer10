
/****************************************************************************

    TERTTREE.H

    Public API for generic tertiary tree routines.

    04/10/98 darekm

****************************************************************************/

#pragma once

//
// Tertiary tree node structure
//

#pragma pack(push, 1)

typedef struct _tnode *PTN;

typedef struct _tnode
{
    union
        {
        char ch;
        unsigned short s;

        struct
            {
            unsigned long :8;
            unsigned long cnt:24;
            };
        };
 
    PTN ptnLo, ptnHi;
    PTN ptnEq;              // also points at data
} TN;

#pragma pack(pop)


//
// Create a tertiary tree of at most cnt nodes
//

__inline PTN PtnCreateTertiaryTree(long cnt)
{
    PTN ptn = calloc(cnt * sizeof(TN), 1);

    if (ptn == NULL)
        return NULL;

    // root node is special

    ptn->ptnLo = &ptn[1];   // first available free node
    ptn->ptnHi = &ptn[cnt]; // tree full node

    return ptn;
}


//
// Delete a tertiatery tree.
// Just destroy the array of nodes!
//

__inline BOOL FDeleteTertiaryTree(PTN ptn)
{
    if (ptn)
        free(ptn);

    return TRUE;
}


//
// Find a string in a tertiary tree.
//
// Returns pointer to data or NULL if not found
//

__inline void *PvSearchPtnPch(PTN ptn, unsigned char *pch)
{
    ptn = ptn->ptnEq;       // skip root node

    while (ptn)
        {
        if (*pch < ptn->ch)
            ptn = ptn->ptnLo;
        else if (*pch > ptn->ch)
            ptn = ptn->ptnHi;
        else if (*pch++)
            ptn = ptn->ptnEq;
        else
            return (void *)ptn->ptnEq;
        }

    return NULL;
}
 

//
// Insert a string and data into a tertiary tree.
// Does nothing if the string already in there.
//
// Returns TRUE if the string was added, FALSE if already in the tree.
//

__inline PTN PtnEnsurePptn(PTN ptnRoot, PTN *pptn, char *pch)
{
    if (*pptn == NULL)
        {
        if (ptnRoot->ptnLo == ptnRoot->ptnHi)
            return NULL;

        *pptn = ptnRoot->ptnLo;
        ptnRoot->ptnLo++;

        (*pptn)->ch = *pch;
        }

    return *pptn;
}

__inline BOOL FInsertPtnPchPv(PTN ptn, unsigned char *pch, void *pv)
{
    PTN ptnRoot = ptn;
    BOOL f;

    ptn = PtnEnsurePptn(ptnRoot, &ptn->ptnEq, pch); // skip root node

    while (ptn)
        {
        ptn->cnt++;     // increment count of entires at/below this node

        if (*pch < ptn->ch)
            ptn = PtnEnsurePptn(ptnRoot, &ptn->ptnLo, pch);
        else if (*pch > ptn->ch)
            ptn = PtnEnsurePptn(ptnRoot, &ptn->ptnHi, pch);
        else if (*pch)
            ptn = PtnEnsurePptn(ptnRoot, &ptn->ptnEq, ++pch);
        else
            break;
        }

    if (ptn == NULL)
        return FALSE;

    f = (ptn->ptnEq == NULL);

    if (!f)
        return FALSE;   // string already present (do nothing??)

    ptn->ptnEq = pv;
    return TRUE;
}


//
// Traverse the tree in order and return the ith data item
//
// Returns the item or NULL if not found. String is copied to pch.
//

__inline void *PvGetIthNodePtn(PTN ptn, long i, char *pch)
{
    int cch = 0;

    ptn = ptn->ptnEq;       // skip root node

    while (ptn)
        {
        if (i >= ptn->cnt)
            return NULL;

        if (ptn->ptnLo)
            {
            if (i < ptn->ptnLo->cnt)
                {
                ptn = ptn->ptnLo;
                continue;
                }
            else
                i -= ptn->ptnLo->cnt;
            }

        if ((i == 0) && (ptn->ch == 0))
            return ptn->ptnEq;

        if (ptn->ch == 0)
            i--;

        if (ptn->ch && ptn->ptnEq)
            {
            if (i < ptn->ptnEq->cnt)
                {
				pch[cch++] = ptn->ch;
                ptn = ptn->ptnEq;
                continue;
                }
            else
                i -= ptn->ptnEq->cnt;
            }

        if (ptn->ptnHi && (i < ptn->ptnHi->cnt))
            ptn = ptn->ptnHi;
        else
            break;
        }

    return NULL;
}

