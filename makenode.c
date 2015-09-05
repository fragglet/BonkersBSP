/*- MAKENODE.C --------------------------------------------------------------*
 Recursively create nodes and return the pointers.
*---------------------------------------------------------------------------*/
#include "structs.h"
#include "bsp.h"

signed short node_x, node_y, node_dx, node_dy;

/*--------------------------------------------------------------------------*/

int SplitDist(struct Seg *ts)
{
	double t,dx,dy;

	if(ts->flip==0)
		{
		dx = (double)(vertices[linedefs[ts->linedef].start].x)-(vertices[ts->start].x);
		dy = (double)(vertices[linedefs[ts->linedef].start].y)-(vertices[ts->start].y);

		if(dx == 0 && dy == 0) 
			fprintf(stderr,"Trouble in SplitDist %f,%f\n",dx,dy);
		t = sqrt((dx*dx) + (dy*dy));
		return (int)t;
		}
	else
		{
		dx = (double)(vertices[linedefs[ts->linedef].end].x)-(vertices[ts->start].x);
		dy = (double)(vertices[linedefs[ts->linedef].end].y)-(vertices[ts->start].y);

		if(dx == 0 && dy == 0) 
			fprintf(stderr,"Trouble in SplitDist %f,%f\n",dx,dy);
		t = sqrt((dx*dx) + (dy*dy));
		return (int)t;
		}
}

/*---------------------------------------------------------------------------*
 Split a list of segs (ts) into two using the method described at bottom of
 file, this was taken from OBJECTS.C in the DEU5beta source.

 This is done by scanning all of the segs and finding the one that does
 the least splitting and has the least difference in numbers of segs on either
 side.
 If the ones on the left side make a SSector, then create another SSector
 else put the segs into lefts list.
 If the ones on the right side make a SSector, then create another SSector
 else put the segs into rights list.
*---------------------------------------------------------------------------*/

static inline void 
DivideSegs(struct Seg *ts, struct Seg **rs, struct Seg **ls, const bbox_t bbox)
{
	struct Seg *rights,*lefts;
	struct Seg *tmps,*best,*news,*prev;
	struct Seg *add_to_rs,*add_to_ls;
  	struct Seg *new_best=NULL,*new_rs,*new_ls;

	struct Seg *strights,*stlefts;
        int num_new=0;
	short int x,y,val;

	best = PickNode(ts,bbox);			/* Pick best node to use.*/

	if(best == NULL) ProgError("Couldn't pick nodeline!");

	node_x = vertices[best->start].x;
	node_y = vertices[best->start].y;
	node_dx = vertices[best->end].x-vertices[best->start].x;
	node_dy = vertices[best->end].y-vertices[best->start].y;

/* When we get to here, best is a pointer to the partition seg.
	Using this partition line, we must split any lines that are intersected
	into a left and right half, flagging them to be put their respective sides
	Ok, now we have the best line to use as a partitioning line, we must
   split all of the segs into two lists (rightside & leftside).				 */

	rights = NULL;									/* Start off with empty*/
	lefts = NULL;									/* lists.*/
	strights = NULL;								/* Start off with empty*/
	stlefts = NULL;								/* lists.*/

	psx = vertices[best->start].x;			/* Partition line coords*/
	psy = vertices[best->start].y;
	pex = vertices[best->end].x;
	pey = vertices[best->end].y;
	pdx = psx - pex;								/* Partition line DX,DY*/
	pdy = psy - pey;

	for(tmps=ts;tmps;tmps=tmps->next)
		{
		progress();									/* Something for the user to look at.*/
		add_to_rs = NULL;
		add_to_ls = NULL;
		if(tmps != best)
			{
			lsx = vertices[tmps->start].x;	/* Calculate this here, cos it doesn't*/
			lsy = vertices[tmps->start].y;	/* change for all the interations of*/
			lex = vertices[tmps->end].x;		/* the inner loop!*/
			ley = vertices[tmps->end].y;
			val = DoLinesIntersect();
			if((val&2 && val&64) || (val&4 && val&32))	/* If intersecting !!*/
				{
				ComputeIntersection(&x,&y);
/*				printf("Splitting Linedef %d at %d,%d\n",tmps->linedef,x,y);*/
 			        vertices = ResizeMemory(vertices, sizeof(struct Vertex) * (num_verts+1));
				vertices[num_verts].x = x;
				vertices[num_verts].y = y;

				news = GetMemory(sizeof( struct Seg));
				*news = *tmps;
				tmps->next = news;
				news->start = num_verts;
				tmps->end = num_verts;
				news->dist = SplitDist(news);
/*				printf("splitting dist = %d\n",news->dist);*/
/*				printf("splitting vertices = %d,%d,%d,%d\n",tmps->start,tmps->end,news->start,news->end);*/
				if(val&32) add_to_ls = tmps;
				if(val&64) add_to_rs = tmps;
				if(val&2) add_to_ls = news;
				if(val&4) add_to_rs = news;
				tmps = news;
				num_verts++;
				num_new++;
				}
			else
				{											/* Not split, which side ?*/
				if(val&34) add_to_ls = tmps;
				if(val&68) add_to_rs = tmps;
				if(val&1 && val&16)					/* On same line*/
					{
/* 06/01/97 Lee Killough: this fixes a bug ever since 1.2x,
   probably 1.0, of BSP: when partitioning a parallel seg,
   you must take its vertices' orientation into account, NOT the
   flip bits, to determine which side of the partitioning line a
   parallel seg should go on. If you simply flip the linedef in
   question, you will be flipping both its vertices and sidedefs,
   and the flip bits as well, even though the basic geometry has
   not changed. Thus you need to use the vertices' orientation
   (whether the seg is in the same direction or not, regardless
   of its original linedef's being flipped or not), into account.

   Originally, some segs were partitioned backwards, and if
   it happened that there were different sectors on either
   side of the seg being partitioned, it could leave holes
   in space, causing either invisible barriers or disappearing
   Things, because the ssector would be associated with the
   wrong sector.

   The old logic of tmps->flip != best->flip seems to rest on
   the assumption that if two segs are parallel, they came
   from the same linedef. This is clearly not always true.   */

              /*  if (tmps->flip != best->flip)   old logic -- wrong!!! */

              /* We know the segs are parallel or nearly so, so take their
                 dot product to determine their relative orientation. */

		if ( (lsx-lex)*pdx + (lsy-ley)*pdy < 0)
  	         add_to_ls = tmps;
	 	else
		 add_to_rs = tmps;
					}
				}
			}
		else add_to_rs = tmps;						/* This is the partition line*/

/*		printf("Val = %X\n",val);*/

		if(add_to_rs)							/* CHECK IF SHOULD ADD RIGHT ONE */
			{
			new_rs = GetMemory(sizeof(struct Seg));
			*new_rs = *add_to_rs;
			if(add_to_rs == best) new_best = new_rs;
			new_rs->next = NULL;
			if(!rights) strights = rights = new_rs;
			else
				{
				rights->next = new_rs;
				rights = new_rs;
				}
			}
				
		if(add_to_ls)							/* CHECK IF SHOULD ADD LEFT ONE */
			{
			new_ls = GetMemory(sizeof(struct Seg));
			*new_ls = *add_to_ls;
			if(add_to_ls == best) new_best = new_ls;
			new_ls->next = NULL;
			if(!lefts) stlefts = lefts = new_ls;
			else
				{
				lefts->next = new_ls;
				lefts = new_ls;
				}
			}
		}

	if(strights == NULL)
		{
/*		printf("No right side, moving partition into right side\n");*/
		strights = rights = new_best;
		prev = NULL;
		for(tmps=stlefts;tmps;tmps=tmps->next)
			{
			if(tmps == new_best)
				{
				if(prev != NULL) prev->next=tmps->next;
				else stlefts=tmps->next;
				}
			prev=tmps;
			}
		prev->next = NULL;
		}
	
	if(stlefts == NULL)
		{
/*		printf("No left side, moving partition into left side\n");*/
		stlefts = lefts = new_best;
		prev = NULL;
		for(tmps=strights;tmps;tmps=tmps->next)
			{
			if(tmps == new_best)
				{
				if(prev != NULL) prev->next=tmps->next;
				else strights=tmps->next;
				}
			prev=tmps;
			}
		stlefts->next = NULL;
		prev->next = NULL;								/* Make sure end of list = NULL*/
		}

	if(rights->next != NULL) rights->next = NULL;
	if(lefts->next != NULL) lefts->next = NULL;

	for(tmps=ts;tmps;tmps=best)
		{
		best=tmps->next;
		free(tmps);
		}

/*	printf("Made %d new Vertices and Segs\n",num_new);*/

	*rs = strights ; *ls = stlefts;
}

/*--------------------------------------------------------------------------*/

static inline int IsItConvex( struct Seg *ts)
{
   struct Seg *line=ts,*check;
   int   sector,val;

/* All ssectors must come from same sector unless it's marked
   "special" with sector tag >= 900. Original idea, Lee Killough */

   sector = sidedefs[ts->flip ? linedefs[ts->linedef].sidedef2 :
                                linedefs[ts->linedef].sidedef1].sector;
   if (sectors[sector].tag < 900)
     while ((line=line->next)!=0)
      {
       int ts=sidedefs[line->flip ? linedefs[line->linedef].sidedef2 :
                                    linedefs[line->linedef].sidedef1].sector;
       if (ts != sector && sectors[ts].tag < 900)
           return TRUE;
      }

   /* all of the segs must be on the same side all the other segs */

	for(line=ts;line;line=line->next)
		{
		psx = vertices[line->start].x;
		psy = vertices[line->start].y;
		pex = vertices[line->end].x;
		pey = vertices[line->end].y;
		pdx = (psx - pex);									/* Partition line DX,DY*/
		pdy = (psy - pey);
		for(check=ts;check;check=check->next)
			{
			if(line!=check)
				{
				lsx = vertices[check->start].x;	/* Calculate this here, cos it doesn't*/
				lsy = vertices[check->start].y;	/* change for all the interations of*/
				lex = vertices[check->end].x;		/* the inner loop!*/
				ley = vertices[check->end].y;
				val = DoLinesIntersect();
				if(val&34) return TRUE;
				}
			}
		}

	/* no need to split the list: these Segs can be put in a SSector */
   return FALSE;
}

/*--------------------------------------------------------------------------*/

static inline int CreateSSector(struct Seg *tmps)
{
	struct Seg *next;
	int n;

	if(num_ssectors == 0)
		{
		ssectors = GetMemory(sizeof(struct SSector));
		}
	else
		{
		ssectors = ResizeMemory(ssectors,sizeof(struct SSector)*(num_ssectors+1));
		}
	
	ssectors[num_ssectors].first = num_psegs;

	n = num_psegs;
	
/*	printf("\n");*/

	for(;tmps;tmps=next)
		{
		if(num_psegs == 0)
			{
			psegs = GetMemory(sizeof(struct Pseg));
			}
		else
			{
			psegs = ResizeMemory(psegs,sizeof(struct Pseg)*(num_psegs+1));
			}

		psegs[num_psegs].start = tmps->start;
		psegs[num_psegs].end = tmps->end;
		psegs[num_psegs].angle = tmps->angle;
		psegs[num_psegs].linedef = tmps->linedef;
		psegs[num_psegs].flip = tmps->flip;
		psegs[num_psegs].dist = tmps->dist;
/*
		printf("%d,%d,%u,%d,%d,%u\n",
			psegs[num_psegs].start,
			psegs[num_psegs].end,
			psegs[num_psegs].angle,
			psegs[num_psegs].linedef,
			psegs[num_psegs].flip,
			psegs[num_psegs].dist);
*/
		num_psegs++;
		next = tmps->next;
		free(tmps); /* This seg is done */
		}

	ssectors[num_ssectors].num = num_psegs-n;

	num_ssectors++;

	return num_ssectors-1;
}

/*- translate (dx, dy) into an integer angle value (0-65535) ---------------*/

inline unsigned int ComputeAngle(int dx, int dy) {
   double w;

	w = (atan2( (double) dy , (double) dx) * (double)(65536/(M_PI*2)));

	if(w<0) w = (double)65536+w;

	return (unsigned) w;
}

struct Node *CreateNode(struct Seg *ts, const bbox_t bbox)
{
	struct Node *tn;
	struct Seg *rights = NULL;
	struct Seg *lefts = NULL;

	tn = GetMemory( sizeof( struct Node));				/* Create a node*/
 
	DivideSegs(ts,&rights,&lefts,bbox);						/* Divide node in two*/

	num_nodes++;

	tn->x = node_x;											/* store node line info*/
	tn->y = node_y;
	tn->dx = node_dx;
	tn->dy = node_dy;

	FindLimits(lefts,tn->leftbox);				/* Find limits of vertices	*/

	if(IsItConvex(lefts))	  								/* Check lefthand side*/
		{
	        if (verbosity > 1) Verbose("L");
		tn->nextl = CreateNode(lefts,tn->leftbox);	/* still segs remaining*/
		tn->chleft = 0;
	        if (verbosity > 1) Verbose("\b");
		}
	else
		{
		tn->nextl = NULL;
		tn->chleft = CreateSSector(lefts) | 0x8000;
		}

	FindLimits(rights, tn->rightbox);										/* Find limits of vertices*/
	
	if(IsItConvex(rights))									/* Check righthand side*/
		{
	        if (verbosity > 1) Verbose("R");
		tn->nextr = CreateNode(rights, tn->rightbox);	/* still segs remaining*/
		tn->chright = 0;
	        if (verbosity > 1) Verbose("\b");
		}
	else
		{
		tn->nextr = NULL;
		tn->chright =  CreateSSector(rights) | 0x8000;
		}

	return tn;
}

/*---------------------------------------------------------------------------*
   
	This message has been taken, complete, from OBJECTS.C in DEU5beta source.
	It outlines the method used here to pick the nodelines.

	IF YOU ARE WRITING A DOOM EDITOR, PLEASE READ THIS:

   I spent a lot of time writing the Nodes builder.  There are some bugs in
   it, but most of the code is OK.  If you steal any ideas from this program,
   put a prominent message in your own editor to make it CLEAR that some
   original ideas were taken from DEU.  Thanks.

   While everyone was talking about LineDefs, I had the idea of taking only
   the Segs into account, and creating the Segs directly from the SideDefs.
   Also, dividing the list of Segs in two after each call to CreateNodes makes
   the algorithm faster.  I use several other tricks, such as looking at the
   two ends of a Seg to see on which side of the nodeline it lies or if it
   should be split in two.  I took me a lot of time and efforts to do this.

   I give this algorithm to whoever wants to use it, but with this condition:
   if your program uses some of the ideas from DEU or the whole algorithm, you
   MUST tell it to the user.  And if you post a message with all or parts of
   this algorithm in it, please post this notice also.  I don't want to speak
   legalese; I hope that you understand me...  I kindly give the sources of my
   program to you: please be kind with me...

   If you need more information about this, here is my E-mail address:
   Raphael.Quinet@eed.ericsson.se (Rapha‰l Quinet).

   Short description of the algorithm:
     1 - Create one Seg for each SideDef: pick each LineDef in turn.  If it
	 has a "first" SideDef, then create a normal Seg.  If it has a
	 "second" SideDef, then create a flipped Seg.
     2 - Call CreateNodes with the current list of Segs.  The list of Segs is
	 the only argument to CreateNodes.
     3 - Save the Nodes, Segs and SSectors to disk.  Start with the leaves of
	 the Nodes tree and continue up to the root (last Node).

   CreateNodes does the following:
     1 - Pick a nodeline amongst the Segs (minimize the number of splits and
	 keep the tree as balanced as possible).
     2 - Move all Segs on the right of the nodeline in a list (segs1) and do
	 the same for all Segs on the left of the nodeline (in segs2).
     3 - If the first list (segs1) contains references to more than one
	 Sector or if the angle between two adjacent Segs is greater than
	 180ø, then call CreateNodes with this (smaller) list.  Else, create
	 a SubSector with all these Segs.
     4 - Do the same for the second list (segs2).
     5 - Return the new node (its two children are already OK).

   Each time CreateSSector is called, the Segs are put in a global list.
   When there is no more Seg in CreateNodes' list, then they are all in the
   global list and ready to be saved to disk.

*---------------------------------------------------------------------------*/

