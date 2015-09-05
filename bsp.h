/*- BSP.H ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#if defined(MSDOS) || defined(__MSDOS__)
#include <dos.h>
#endif
#ifdef __TURBOC__
#include <alloc.h>
#endif

/* cph - from FreeBSD math.h: */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*- boolean constants ------------------------------------------------------*/

#define TRUE			1
#define FALSE			0

/*- The function prototypes ------------------------------------------------*/

#undef max
#define max(a,b) (((a)>(b))?(a):(b))

/*- print a resource name by printing first 8 characters --*/

#define Printname(dir) printf("%-8.8s",(dir)->name)

/*- Global functions & variables ------------------------------------------*/

/* blockmap.c */
void CreateBlockmap_old(const bbox_t bbox);
void CreateBlockmap_compressed(const bbox_t bbox);
extern void (*CreateBlockmap)(const bbox_t bbox);

/* bsp.c */
void progress(void);
void FindLimits(struct Seg *, bbox_t box);
int SplitDist(struct Seg *ts);

extern const char *unlinkwad;

struct lumplist *FindDir(const char *);
void* ReadLump(struct lumplist *l);
void add_lump(const char *name, void *data, size_t length);

/* endian.c */

void ConvertAll(void);
void ConvertVertex(void);
void ConvertLinedef(void);
void ConvertSidedef(void);
void ConvertSector(void);

void swapshort(uint16_t *i);
void swaplong(uint32_t *l);
void swapint(unsigned int *l);

/* funcs.c */
extern int verbosity;
void Verbose(const char*, ...)  __attribute__((format(printf,1,2)));
void ProgError(const char *, ...) __attribute__((format(printf,1,2), noreturn));
void* GetMemory(size_t) __attribute__((warn_unused_result, malloc));
void* ResizeMemory(void *, size_t) __attribute__((warn_unused_result));

/* level.c */

void DoLevel(const char *current_level_name, struct lumplist * current_level);

extern struct Vertex *vertices;
extern long num_verts;

extern struct LineDef *linedefs;
extern long num_lines;

extern struct SideDef *sidedefs;
extern long num_sides;

extern struct Sector *sectors;
extern long num_sects;

extern struct SSector *ssectors;
extern long num_ssectors;

extern struct Pseg *psegs;
extern long num_psegs;

extern long num_nodes;

extern unsigned char *SectorHits;

extern long psx,psy,pex,pey,pdx,pdy;
extern long lsx,lsy,lex,ley;

/* makenode.c */
struct Node *CreateNode(struct Seg *, const bbox_t bbox);
unsigned ComputeAngle(int,int);

/* picknode.c */
extern int factor;

struct Seg *PickNode_traditional(struct Seg *, const bbox_t bbox);
struct Seg *PickNode_visplane(struct Seg *, const bbox_t bbox);
extern struct Seg *(*PickNode)(struct Seg *, const bbox_t bbox);
int DoLinesIntersect(void);
void ComputeIntersection(short int *outx,short int *outy);

/* malloc edbugging with dmalloc */
#ifdef WITH_DMALLOC
#include <dmalloc.h>

#define GetMemory malloc
#define ResizeMemory realloc

#endif

/*------------------------------- end of file ------------------------------*/
