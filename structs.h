#include "config.h"

#include <inttypes.h>

/*- Doom Structures .. Colin Reed 1994 -------------------------------------*/

struct wad_header 					/* Linked wad files list.*/
{
	char type[4];
	uint32_t num_entries;
	uint32_t dir_start;
} __attribute__((packed));

struct directory 						/* The directory entry header*/
{
	uint32_t start;
	uint32_t length;
	char name[8];
} __attribute__((packed));

struct Block
{
	int16_t minx;
	int16_t miny;
	int16_t xblocks;
	int16_t yblocks;
} __attribute__((packed));

struct lumplist {
 struct lumplist *next;
 struct directory *dir;
 void *data;
 struct lumplist *level;
};

/*- The level structures ---------------------------------------------------*/

struct Thing
{
   int16_t xpos;      /* x position */
   int16_t ypos;      /* y position */
   int16_t angle;     /* facing angle */
   int16_t type;      /* thing type */
   int16_t when;      /* appears when? */
} __attribute__((packed));

struct Vertex
{
   int16_t x;         /* X coordinate */
   int16_t y;         /* Y coordinate */
} __attribute__((packed));

struct LineDef
{
   uint16_t start;     /* from this vertex ... */
   uint16_t end;       /* ... to this vertex */
   uint16_t flags;     /* see NAMES.C for more info */
   uint16_t type;      /* see NAMES.C for more info */
   uint16_t tag;       /* crossing this linedef activates the sector with the same tag */
   int16_t sidedef1;  /* sidedef */
   int16_t sidedef2;  /* only if this line adjoins 2 sectors */
} __attribute__((packed));

struct SideDef
{
   int16_t xoff;      /* X offset for texture */
   int16_t yoff;      /* Y offset for texture */
   char tex1[8];  /* texture name for the part above */
   char tex2[8];  /* texture name for the part below */
   char tex3[8];  /* texture name for the regular part */
   int16_t sector;    /* adjacent sector */
} __attribute__((packed));

struct Sector
{
   int16_t floorh;    /* floor height */
   int16_t ceilh;     /* ceiling height */
   char floort[8];/* floor texture */
   char ceilt[8]; /* ceiling texture */
   int16_t light;     /* light level (0-255) */
   int16_t special;   /* special behaviour (0 = normal, 9 = secret, ...) */
   int16_t tag;       /* sector activated by a linedef with the same tag */
} __attribute__((packed));

/*--------------------------------------------------------------------------*/
/* These are the structure used for creating the NODE bsp tree.             */
/*--------------------------------------------------------------------------*/

struct Seg
{
   short int start;     /* from this vertex ... */
   short int end;       /* ... to this vertex */
   unsigned short angle;/* angle (0 = east, 16384 = north, ...) */
   short int linedef;   /* linedef that this seg goes along*/
   short int flip;      /* true if not the same direction as linedef */
   unsigned short dist; /* distance from starting point */
	struct Seg *next;
        short psx,psy,pex,pey;  /* Start, end coordinates */
        long pdx,pdy,ptmp;      /* Used in intersection calculations */
        long len;
        short sector;
} __attribute__((packed));

struct Pseg
{
   short int start;     /* from this vertex ... */
   short int end;       /* ... to this vertex */
   unsigned short angle;/* angle (0 = east, 16384 = north, ...) */
   short int linedef;   /* linedef that this seg goes along*/
   short int flip;      /* true if not the same direction as linedef */
   unsigned short dist; /* distance from starting point */
} __attribute__((packed));

/* cph - dedicated type for bounding boxes, as in the Doom source */
typedef int16_t bbox_t[4];
enum { BB_TOP, BB_BOTTOM, BB_LEFT, BB_RIGHT };

struct Node
{
   int16_t x, y;			/* starting point*/
   int16_t dx, dy;			/* offset to ending point*/
   bbox_t rightbox;			/* bounding rectangle 1*/
   bbox_t leftbox;			/* bounding rectangle 2*/
   int16_t chright, chleft;		/* Node or SSector (if high bit is set)*/
	struct Node *nextr,*nextl;
	int16_t node_num;	        		/* starting at 0 (but reversed when done)*/
        long ptmp;
} __attribute__((packed));

struct Pnode
{
   int16_t x, y;		/* starting point*/
   int16_t dx, dy;		/* offset to ending point*/
   bbox_t rightbox;		/* bounding rectangle 1*/
   bbox_t leftbox;		/* bounding rectangle 2*/
   int16_t chright, chleft;	/* Node or SSector (if high bit is set)*/
} __attribute__((packed));

struct SSector
{
   uint16_t num;       /* number of Segs in this Sub-Sector */
   uint16_t first;     /* first Seg */
} __attribute__((packed));

/*--------------------------------------------------------------------------*/
