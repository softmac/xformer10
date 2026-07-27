/* stubs_linux.c - Phase 1 external definitions for cross-TU inline functions.
   PeekBAtari and PokeBAtari are defined __forceinline (-> static inline) in
   atari800.c, so they are invisible to xvideo.c and xsio.c that call them.
   This file provides external definitions WITHOUT including atari800.h so that
   the conflicting 'static inline' declaration from that header cannot override
   the external linkage of these definitions. */

#include <stdint.h>

typedef unsigned long int ADDR;
typedef uint8_t BYTE;
typedef int BOOL;

/* PeekBAtariMON/PokeBAtariMON are non-inline external wrappers in atari800.c
   that call the real (static inline) bus-dispatch implementations. Forwarding
   through them avoids including atari800.h (which would shadow the external
   definition with its static inline declaration). */
extern BYTE PeekBAtariMON(void *candy, ADDR addr);
extern BOOL PokeBAtariMON(void *candy, ADDR addr, BYTE b);

BYTE PeekBAtari(void *candy, ADDR addr)         { return PeekBAtariMON(candy, addr); }
BOOL PokeBAtari(void *candy, ADDR addr, BYTE b) { return PokeBAtariMON(candy, addr, b); }
