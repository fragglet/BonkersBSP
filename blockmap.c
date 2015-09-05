/*- blockmap.c --------------------------------------------------------------*

 Node builder for DOOM levels (c) 1998 Colin Reed, version 3.0 (dos extended)

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

/*
 * Separated from bsp.c 2000/08/26 by Colin Phipps
 */

#include "structs.h"
#include "bsp.h"

/*--------------------------------------------------------------------------*/

static int
IsLineDefInside(int ldnum, int xmin, int ymin, int xmax, int ymax)
{
	int             x1 = vertices[linedefs[ldnum].start].x;
	int             y1 = vertices[linedefs[ldnum].start].y;
	int             x2 = vertices[linedefs[ldnum].end].x;
	int             y2 = vertices[linedefs[ldnum].end].y;
	int             count = 2;

	for (;;)
		if (y1 > ymax) {
			if (y2 > ymax)
				return (FALSE);
			x1 = x1 + (x2 - x1) * (double) (ymax - y1) / (y2 - y1);
			y1 = ymax;
			count = 2;
		} else if (y1 < ymin) {
			if (y2 < ymin)
				return (FALSE);
			x1 = x1 + (x2 - x1) * (double) (ymin - y1) / (y2 - y1);
			y1 = ymin;
			count = 2;
		} else if (x1 > xmax) {
			if (x2 > xmax)
				return (FALSE);
			y1 = y1 + (y2 - y1) * (double) (xmax - x1) / (x2 - x1);
			x1 = xmax;
			count = 2;
		} else if (x1 < xmin) {
			if (x2 < xmin)
				return (FALSE);
			y1 = y1 + (y2 - y1) * (double) (xmin - x1) / (x2 - x1);
			x1 = xmin;
			count = 2;
		} else {
			int             t;
			if (!--count)
				return (TRUE);
			t = x1;
			x1 = x2;
			x2 = t;
			t = y1;
			y1 = y2;
			y2 = t;
		}
}

/*- Create blockmap --------------------------------------------------------*/

void 
CreateBlockmap_old(const bbox_t bbox)
{
	struct Block    blockhead;
	short int      *blockptrs;
	short int      *blocklists = NULL;
	long            blockptrs_size;

	long            blockoffs = 0;
	int             x, y, n;
	int             blocknum = 0;

	Verbose("Creating Blockmap... ");

	blockhead.minx = bbox[BB_LEFT] & -8;
	blockhead.miny = bbox[BB_BOTTOM] & -8;
	blockhead.xblocks = ((bbox[BB_RIGHT] - (bbox[BB_LEFT] & -8)) / 128) + 1;
	blockhead.yblocks = ((bbox[BB_TOP] - (bbox[BB_BOTTOM] & -8)) / 128) + 1;

	blockptrs_size = (blockhead.xblocks * blockhead.yblocks) * 2;
	blockptrs = GetMemory(blockptrs_size);

	for (y = 0; y < blockhead.yblocks; y++) {
		for (x = 0; x < blockhead.xblocks; x++) {
			progress();

			blockptrs[blocknum] = (blockoffs + 4 + (blockptrs_size / 2));
			swapshort((unsigned short *) blockptrs + blocknum);

			blocklists = ResizeMemory(blocklists, ((blockoffs + 1) * 2));
			blocklists[blockoffs] = 0;
			blockoffs++;
			for (n = 0; n < num_lines; n++) {
				if (IsLineDefInside(n, (blockhead.minx + (x * 128)), (blockhead.miny + (y * 128)), (blockhead.minx + (x * 128)) + 127, (blockhead.miny + (y * 128)) + 127)) {
					/*
					 * printf("found line %d in block
					 * %d\n",n,blocknum);
					 */
					blocklists = ResizeMemory(blocklists, ((blockoffs + 1) * 2));
					blocklists[blockoffs] = n;
					swapshort((unsigned short *) blocklists + blockoffs);
					blockoffs++;
				}
			}
			blocklists = ResizeMemory(blocklists, ((blockoffs + 1) * 2));
			blocklists[blockoffs] = -1;
			swapshort((unsigned short *) blocklists + blockoffs);
			blockoffs++;

			blocknum++;
		}
	}
	{
		long            blockmap_size = blockoffs * 2;
		char           *data = GetMemory(blockmap_size + blockptrs_size + 8);
		memcpy(data, &blockhead, 8);
		swapshort((unsigned short *) data + 0);
		swapshort((unsigned short *) data + 1);
		swapshort((unsigned short *) data + 2);
		swapshort((unsigned short *) data + 3);
		memcpy(data + 8, blockptrs, blockptrs_size);
		memcpy(data + 8 + blockptrs_size, blocklists, blockmap_size);
		free(blockptrs);
		free(blocklists);
		add_lump("BLOCKMAP", data, blockmap_size + blockptrs_size + 8);
	}
	Verbose("done.\n");
}

/*- Create blockmap (compressed) ----------------------------------------
 * Contributed by Simon "fraggle" Howard <sdh300@ecs.soton.ac.uk> 
 * Merged 2001/11/17 */

struct blocklist_s {
	int num_lines;
	unsigned short offset;
	unsigned short *lines;
	struct blocklist_s *next;          /* for hash table */
};

static struct blocklist_s **blockhash;
static int blockhash_size;
static int blocklist_size;           /* size, in bytes of the blocklists */

/* offset pointers */

static struct blocklist_s **blockptrs;
static int num_blockptrs;

/* hashkey function for search */

static int blockhash_key(struct blocklist_s *bl)
{
	int i;
	int hash = 0;

	/* this is a pretty lame hash function
	   it has to be associative though (ie, 1 2 3 == 3 2 1) */
	
	for(i=0; i<bl->num_lines; ++i)
		hash += bl->lines[i];

	return hash % blockhash_size;
}

/* compare two struct blocklist_s's
   like strcmp: a 0 return value means they are identical */

static int blocklist_cmp(struct blocklist_s *bl1, struct blocklist_s *bl2)
{
	int i;
	
	if(bl1->num_lines != bl2->num_lines)
		return 1;

	for(i=0; i<bl1->num_lines; ++i) {
		if(bl1->lines[i] != bl2->lines[i])
			return 1;
	}

	return 0;
}

/* search for an identical blocklist */

static struct blocklist_s *blockhash_search(struct blocklist_s *bl)
{
	int hash = blockhash_key(bl);
	struct blocklist_s *search;

	for(search=blockhash[hash]; search; search=search->next) {
		if(!blocklist_cmp(bl, search))
			return search;		
	}

	/* not found */
	
	return NULL;
}

/* add a new blocklist to the hashtable */

static struct blocklist_s *blockhash_add(struct blocklist_s *newbl)
{
	struct blocklist_s *bl;
	int hash;
	
	/* first, check an identical one doesnt already exist */

	bl = blockhash_search(newbl);

	if(bl)
		return bl;
	
	/* need to add new blocklist */
	
	/* make a copy */
	
	bl = GetMemory(sizeof(*bl));
	bl->num_lines = newbl->num_lines;
	bl->lines = GetMemory(sizeof(*bl->lines) * bl->num_lines);
	memcpy(bl->lines, newbl->lines,
	       sizeof(*bl->lines) * bl->num_lines);
	
	/* link into hash table */

	hash = blockhash_key(bl);

	bl->next = blockhash[hash];
	blockhash[hash] = bl;

	/* maintain blocklist count */

	blocklist_size += sizeof(*bl->lines) * bl->num_lines;
	
	return bl;
}

static void blockmap_assemble(const struct Block* blockmaphead)
{
	int i;
	int offset;

	/* build the lump itself */

	size_t blockmap_size =
		sizeof(*blockmaphead) +
		num_blockptrs * sizeof(short) +
		blocklist_size;
	register short* blockmap = GetMemory(blockmap_size);

	/* header */

	memcpy(blockmap, blockmaphead, sizeof *blockmaphead);
	swapshort(blockmap);
	swapshort(blockmap+1);
	swapshort(blockmap+2);
	swapshort(blockmap+3);
	
	/* every hash chain */
	
	for(i=0,offset=num_blockptrs+sizeof(*blockmaphead)/sizeof(short);
			i<blockhash_size; ++i) {
		struct blocklist_s *bl;

		/* every blocklist in the chain */
		
		for(bl=blockhash[i]; bl; bl = bl->next) {

			/* write */

			int l;

			/* offset is in short ints, not bytes */
			
			bl->offset = offset;

			/* write each line */

			for(l=0; l<bl->num_lines; ++l) {
				blockmap[offset] = bl->lines[l];
				swapshort(blockmap + offset);
				offset++;
			}
		}
	}

	offset = sizeof(*blockmaphead)/sizeof(short);
	
	for(i=0; i<num_blockptrs; ++i) {
		blockmap[offset+i] = blockptrs[i]->offset;
		swapshort(blockmap+offset+i);
	}

        add_lump("BLOCKMAP", blockmap, blockmap_size);
}

void
CreateBlockmap_compressed(const bbox_t bbox)
{
	int             x, y, n;
	int             blocknum = 0;
	unsigned short *templines;
	int             num_templines;
	struct Block	blockhead;

	fprintf(stderr,"Creating compressed blockmap... ");

	/* header */
	
	blockhead.minx = bbox[BB_LEFT] & -8;
	blockhead.miny = bbox[BB_BOTTOM] & -8;
	blockhead.xblocks = ((bbox[BB_RIGHT] - (bbox[BB_LEFT] & -8)) / 128) + 1;
	blockhead.yblocks = ((bbox[BB_TOP] - (bbox[BB_BOTTOM] & -8)) / 128) + 1;

	/* build hash table */

	blockhash_size = blockhead.xblocks * blockhead.yblocks;
	blockhash = GetMemory(blockhash_size * sizeof(*blockhash));
	for(n=0; n<blockhash_size; ++n)
		blockhash[n] = NULL;

	/* pointers */
	
	num_blockptrs = blockhead.xblocks * blockhead.yblocks;
	blockptrs = GetMemory(num_blockptrs * sizeof(*blockptrs));

	num_templines = 10240;
	templines = GetMemory(num_templines * sizeof(*templines));
	
	/* build all blocklists */
	
	for (y = 0; y < blockhead.yblocks; y++) {
		for (x = 0; x < blockhead.xblocks; x++) {
			struct blocklist_s tempbl;

			tempbl.num_lines = 0;			
			tempbl.lines = templines;

			progress();

			/* first line is a 0 */

			tempbl.lines[tempbl.num_lines++] = 0;

			for (n = 0; n < num_lines; n++) {
				if (IsLineDefInside(n, (blockhead.minx + (x * 128)), (blockhead.miny + (y * 128)), (blockhead.minx + (x * 128)) + 127, (blockhead.miny + (y * 128)) + 127)) {
					/*
					 * printf("found line %d in block
					 * %d\n",n,blocknum);
					 */

					if(tempbl.num_lines >= num_templines-5) {
						num_lines *= 2;
						templines = ResizeMemory(templines,
									 num_templines * sizeof(*templines));
						tempbl.lines = templines;
					}
					
					tempbl.lines[tempbl.num_lines++] = n;
				}
			}

			/* last is 0xffff */

			tempbl.lines[tempbl.num_lines++] = 0xffff;

			blockptrs[blocknum++] = blockhash_add(&tempbl);
		}
	}

	free(templines);
	
	/* assemble */

	blockmap_assemble(&blockhead);

	/* deconstruct the hash table */

	for(n=0; n<blockhash_size; ++n) {
		while(blockhash[n]) {
			struct blocklist_s *del = blockhash[n];
			blockhash[n] = blockhash[n]->next;
			free(del->lines);
			free(del);
		}
	}

	free(blockhash);
	free(blockptrs);
	
	Verbose("done.\n");

	return;
}

