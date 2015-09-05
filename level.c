/*- level.c -----------------------------------------------------------------*

 Separated from bsp.c 2000/08/26 by Colin Phipps

 Node builder for DOOM levels (c) 1998 Colin Reed, version 3.0 (dos extended)

 Performance increased 200% over 1.2x

 Many thanks to Mark Harrison for finding a bug in 1.1 which caused some
 texture align problems when a flipped SEG was split.

 Credit to:-

 Raphael Quinet (A very small amount of code has been borrowed from DEU).

 Matt Fell for the doom specs.

 Lee Killough for performance tuning, support for multilevel wads, special
 effects, and wads with lumps besides levels.

 Also, the original idea for some of the techniques where also taken from the
 comment at the bottom of OBJECTS.C in DEU, and the doc by Matt Fell about
 the nodes.

 Use this code for your own editors, but please credit me.

*---------------------------------------------------------------------------*/

#include "structs.h"
#include "bsp.h"

/*- Global Vars ------------------------------------------------------------*/

struct Vertex  *vertices;
long            num_verts = 0;

struct LineDef *linedefs;
long            num_lines = 0;

struct SideDef *sidedefs;
long            num_sides = 0;

struct Sector  *sectors;
long            num_sects = 0;

struct SSector *ssectors;
long            num_ssectors = 0;

struct Pseg    *psegs = NULL;
long            num_psegs = 0;

static struct Pnode *pnodes = NULL;
static long     num_pnodes = 0;
static long     pnode_indx = 0;
long            num_nodes = 0;

unsigned char  *SectorHits;

long            psx, psy, pex, pey, pdx, pdy;
long            lsx, lsy, lex, ley;

/*- Prototypes -------------------------------------------------------------*/

static void     GetVertexes(void);
static void     GetLinedefs(void);
static void     GetSidedefs(void);
static void     GetSectors(void);

static struct Seg *CreateSegs(void);

/*--------------------------------------------------------------------------*/
/* Find limits from a list of segs, does this by stepping through the segs */
/* and comparing the vertices at both ends. */
/*--------------------------------------------------------------------------*/

void FindLimits(struct Seg * ts, bbox_t box)
{
	int             minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN,
	                maxy = INT_MIN;
	int             fv, tv;

	for (;;) {
		fv = ts->start;
		tv = ts->end;
		/* printf("%d : %d,%d\n",n,vertices[n].x,vertices[n].y); */
		if (vertices[fv].x < minx)
			minx = vertices[fv].x;
		if (vertices[fv].x > maxx)
			maxx = vertices[fv].x;
		if (vertices[fv].y < miny)
			miny = vertices[fv].y;
		if (vertices[fv].y > maxy)
			maxy = vertices[fv].y;
		if (vertices[tv].x < minx)
			minx = vertices[tv].x;
		if (vertices[tv].x > maxx)
			maxx = vertices[tv].x;
		if (vertices[tv].y < miny)
			miny = vertices[tv].y;
		if (vertices[tv].y > maxy)
			maxy = vertices[tv].y;
		if (ts->next == NULL)
			break;
		ts = ts->next;
	}
	/* cph - top, bottom, left, right */
	box[BB_TOP] = maxy; box[BB_BOTTOM] = miny;
	box[BB_LEFT] = minx; box[BB_RIGHT] = maxx;
}

/*--------------------------------------------------------------------------*/

static struct Seg *
add_seg(struct Seg * cs, int n, int fv, int tv,
	struct Seg ** fs, struct SideDef * sd)
{
	struct Seg     *ts = GetMemory(sizeof(struct Seg));	/* get mem for Seg */
	cs = cs ? (cs->next = ts) : (*fs = ts);
	cs->next = NULL;
	cs->start = fv;
	cs->end = tv;
	cs->pdx = (long) (cs->pex = vertices[tv].x)
		- (cs->psx = vertices[fv].x);
	cs->pdy = (long) (cs->pey = vertices[tv].y)
		- (cs->psy = vertices[fv].y);
	cs->ptmp = cs->pdx * cs->psy - cs->psx * cs->pdy;
	cs->len = (long) sqrt((double) cs->pdx * cs->pdx +
			      (double) cs->pdy * cs->pdy);

	if ((cs->sector = sd->sector) == -1)
		fprintf(stderr, "\nWarning: Bad sidedef in linedef %d (Z_CheckHeap error)\n", n);

	cs->angle = ComputeAngle(cs->pdx, cs->pdy);

	if (linedefs[n].tag == 999)
		cs->angle = (cs->angle + (unsigned) (sd->xoff * (65536.0 / 360.0))) & 65535u;

	cs->linedef = n;
	cs->dist = 0;
	return cs;
}

/*- initially creates all segs, one for each line def ----------------------*/

static struct Seg* CreateSegs(void)
{
	struct Seg     *cs = NULL;	/* current Seg */
	struct Seg     *fs = NULL;	/* first Seg in list */
	struct LineDef *l = linedefs;
	int             n;

	Verbose("Creating Segs... ");

	for (n = 0; n < num_lines; n++, l++) {	/* step through linedefs and
		 * get side *//* numbers */
                /* If line is 0 length, don't generate any segs */
                if (!memcmp(&vertices[l->start],&vertices[l->end],sizeof *vertices))
                  continue;
		if (l->sidedef1 != -1)
			(cs = add_seg(cs, n, l->start, l->end, &fs, sidedefs + l->sidedef1))->flip = 0;
		else
			fprintf(stderr,"\nWarning: Linedef %d has no right sidedef\n", n);

		if (l->sidedef2 != -1)
			(cs = add_seg(cs, n, l->end, l->start, &fs, sidedefs + l->sidedef2))->flip = 1;
		else if (l->flags & 4)
			fprintf(stderr,"\nWarning: Linedef %d is 2s but has no left sidedef\n", n);
	}
	Verbose("done.\n");
	return fs;
}

/*- read the vertices from the wad file and place in 'vertices' ------------
    Rewritten by Lee Killough, to speed up performance                       */

static struct lumplist *vertlmp;

static void 
GetVertexes(void)
{
        long            n, used_verts;
	int            *translate;
	struct lumplist *l = FindDir("VERTEXES");

	if (!l || !(num_verts = l->dir->length / sizeof(struct Vertex)))
		ProgError("Couldn't find any Vertices");

	vertlmp = l;

	vertices = ReadLump(l);

	translate = GetMemory(num_verts * sizeof(*translate));

	for (n = 0; n < num_verts; n++)	/* Unmark all vertices */
		translate[n] = -1;

	for (n = 0; n < num_lines; n++) {	/* Mark all used vertices */
		int             s = linedefs[n].start;
		int             e = linedefs[n].end;
		if (s < 0 || s >= num_verts || e < 0 || e >= num_verts)
			ProgError("Linedef %ld has vertex out of range\n", n);
		translate[s] = translate[e] = 0;
	}
	used_verts = 0;
	for (n = 0; n < num_verts; n++)	/* Sift up all unused vertices */
		if (!translate[n])
			vertices[translate[n] = used_verts++] = vertices[n];

	for (n = 0; n < num_lines; n++) {	/* Renumber vertices */
		int             s = translate[linedefs[n].start];
		int             e = translate[linedefs[n].end];
		if (s < 0 || s >= used_verts || e < 0 || e >= used_verts)
			ProgError("Trouble in GetVertexes: Renumbering\n");
		linedefs[n].start = s;
		linedefs[n].end = e;
	}

	free(translate);

	Verbose("Loaded %ld vertices", num_verts);
	if (num_verts > used_verts)
		Verbose(", but %ld were unused\n(this is normal if the nodes were built before).\n",
		       num_verts - used_verts);
	else
		Verbose(".");
	num_verts = used_verts;
	if (!num_verts)
		ProgError("Couldn't find any used Vertices");
}

/*- read the linedefs from the wad file and place in 'linedefs' ------------*/

static void 
GetLinedefs(void)
{
	struct lumplist *l = FindDir("LINEDEFS");

	if (!l || !(num_lines = l->dir->length / sizeof(struct LineDef)))
		ProgError("Couldn't find any Linedefs");

	linedefs = ReadLump(l);
}

/*- read the sidedefs from the wad file and place in 'sidedefs' ------------*/

static void 
GetSidedefs(void)
{
	struct lumplist *l = FindDir("SIDEDEFS");

	if (!l || !(num_sides = l->dir->length / sizeof(struct SideDef)))
		ProgError("Couldn't find any Sidedefs");

	sidedefs = ReadLump(l);
}

/*- read the sectors from the wad file and place count in 'num_sectors' ----*/

static void 
GetSectors(void)
{
	struct lumplist *l = FindDir("SECTORS");

	if (!l || !(num_sects = l->dir->length / sizeof(struct Sector)))
		ProgError("Couldn't find any Sectors");

	sectors = ReadLump(l);
}

/*--------------------------------------------------------------------------*/
/* Converts the nodes from a btree into the array format for inclusion in 
 * the .wad. Frees the btree as it goes */

static signed short ReverseNodes(struct Node * tn)
{
	struct Pnode   *pn;

	tn->chright = tn->nextr ? ReverseNodes(tn->nextr) :
		tn->chright | 0x8000;

	tn->chleft = tn->nextl ? ReverseNodes(tn->nextl) :
		tn->chleft | 0x8000;

	pn = pnodes + pnode_indx++;

	pn->x = tn->x;
	pn->y = tn->y;
	pn->dx = tn->dx;
	pn->dy = tn->dy;
	memcpy(pn->leftbox , tn->leftbox , sizeof(pn->leftbox ));
	memcpy(pn->rightbox, tn->rightbox, sizeof(pn->rightbox));
	pn->chright = tn->chright;
	pn->chleft = tn->chleft;

	/* Free the node, it's in our array now */
	memset(tn,0,sizeof *tn);
	free(tn);
	return num_pnodes++;
}

/* Height of nodes */
static unsigned 
height(const struct Node * tn)
{
	if (tn) {
		unsigned        l = height(tn->nextl), r = height(tn->nextr);
		return l > r ? l + 1 : r + 1;
	}
	return 1;
}

/*
 * DoLevel -
 * 
 * Process a level, building NODES, etc
 */

void 
DoLevel(const char *current_level_name, struct lumplist * current_level)
{
	struct Seg     *tsegs;
	static struct Node *nodelist;
	bbox_t mapbound;

	Verbose("\nBuilding nodes on %-.8s\n\n", current_level_name);

	num_ssectors = 0;
	num_psegs = 0;
	num_nodes = 0;

	GetLinedefs();		/* Get and convert linedefs first */
        ConvertLinedef();       /* so we can to remove redundant */
	GetVertexes();		/* vertices here. */
	GetSidedefs();
	GetSectors();

        ConvertVertex();
        ConvertSidedef();
        ConvertSector();

	tsegs = CreateSegs();	/* Initially create segs */

	FindLimits(tsegs,mapbound);	/* Find limits of vertices */

	Verbose("Map goes from (%d,%d) to (%d,%d)\n", 
		mapbound[BB_TOP   ],mapbound[BB_LEFT  ],
		mapbound[BB_BOTTOM],mapbound[BB_RIGHT ]);

	SectorHits = GetMemory(num_sects);

	nodelist = CreateNode(tsegs,mapbound);	/* recursively create nodes */

	Verbose("%lu NODES created, with %lu SSECTORS.\n", num_nodes, num_ssectors);

	Verbose("Found %lu used vertices\n", num_verts);

	Verbose("Heights of left and right subtrees = (%u,%u)\n",
	       height(nodelist->nextl), height(nodelist->nextr));

	vertlmp->dir->length = num_verts * sizeof(struct Vertex);
	vertlmp->data = vertices;

	add_lump("SEGS", psegs, sizeof(struct Pseg) * num_psegs);

	add_lump("SSECTORS", ssectors, sizeof(struct SSector) * num_ssectors);

	if (!FindDir("REJECT")) {
		long            reject_size = (num_sects * num_sects + 7) / 8;
		void           *data = GetMemory(reject_size);
		memset(data, 0, reject_size);
		add_lump("REJECT", data, reject_size);
	}
	CreateBlockmap(mapbound);

	pnodes = GetMemory(sizeof(struct Pnode) * num_nodes);
	num_pnodes = 0;
	pnode_indx = 0;
	ReverseNodes(nodelist);
	add_lump("NODES", pnodes, sizeof(struct Pnode) * num_pnodes);

	free(SectorHits);

	ConvertAll();		/* Switch back to file endianness */
}				/* if (lump->islevel) */

/*- end of file ------------------------------------------------------------*/
