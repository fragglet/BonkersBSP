/*- PICKNODE.C --------------------------------------------------------------*

 To be able to divide the nodes down, this routine must decide which is the
 best Seg to use as a nodeline. It does this by selecting the line with least
 splits and has least difference of Segs on either side of it.

 Credit to Raphael Quinet and DEU, this routine is a copy of the nodeline
 picker used in DEU5beta. I am using this method because the method I
 originally used was not so good.

 Rewritten by Lee Killough to significantly improve performance, while
 not affecting results one bit in >99% of cases (some tiny differences
 due to roundoff error may occur, but they are insignificant).
*---------------------------------------------------------------------------*/

#include "structs.h"
#include "bsp.h"

/* This is the original "factor" used by previous versions of the code -- it
   must be maintained in a macro to avoid mistakes if we are to keep the 
   tradition of using it, and being able to modify it.*/

#define FACTOR 8  

int factor=2*FACTOR+1;

struct Seg *PickNode_traditional(struct Seg *ts, const bbox_t bbox)
{
 struct Seg *best = ts;
 long bestcost=LONG_MAX;
 struct Seg *part;
 int cnt=0;

 for (part=ts;part;part=part->next) /* Count once and for all */
   cnt++;

 for (part=ts;part;part = part->next)	/* Use each Seg as partition*/
  {
   struct Seg *check;
   long cost=0,tot=0,diff=cnt;

   progress();           	        /* Something for the user to look at.*/

   for (check=ts;check;check=check->next) /* Check partition against all Segs*/
    {         /*     get state of lines' relation to each other    */
     long a = part->pdy * check->psx - part->pdx * check->psy + part->ptmp;
     long b = part->pdy * check->pex - part->pdx * check->pey + part->ptmp;
     if ((a^b) < 0)
       if (a && b)
	{                    /* Line is split; a,b nonzero, opposite sign */
         long l=check->len;
         long d=(l*a)/(a-b); /* Distance from start of intersection */
         if (d>=2)
          {
        /* If the linedef associated with this seg has a sector tag >= 900,
           treat it as precious; i.e. don't split it unless all other options
           are exhausted. This is used to protect deep water and invisible
           lifts/stairs from being messed up accidentally by splits. */

           if (linedefs[check->linedef].tag >= 900)
	     cost += factor*64;

           cost += factor;

           if (cost > bestcost)   /* This is the heart of my pruning idea - */
	     goto prune;          /* it catches bad segs early on. Killough */

           tot++;
          }
         else
          if (l-d<2 ? check->pdx*part->pdx+check->pdy*part->pdy<0 : b<0)
            goto leftside;
        }
       else
         goto leftside;
     else
       if (a<=0 && (a || (!b && check->pdx*part->pdx+check->pdy*part->pdy<0)))
        {
         leftside:
           diff-=2;
        }
    }

   /* Take absolute value. diff is being used to obtain the */
   /* min/max values by way of: min(a,b)=(a+b-abs(a-b))/2   */

   if ((diff-=tot) < 0)
     diff= -diff;

   /* Make sure at least one Seg is on each side of the partition*/

   if (tot+cnt > diff && (cost+=diff) < bestcost)
    {                   /* We have a new better choice */
     bestcost = cost;
     best = part;   	/* Remember which Seg*/
    }
   prune:;              /* Early exit and skip past the tests above */
  }
 return best;		/* All finished, return best Seg*/
}


/* Lee Killough 06/1997:

   The chances of visplane overflows can be reduced by attemping to
   balance the number of distinct sector references (as opposed to
   SEGS) that are on each side of the node line, and by rejecting
   node lines that cut across wide open space, as measured by the
   proportion of the node line which is incident with segs, inside
   the bounding box.

   Node lines which are extensions of linedefs whose vertices are
   on the boundary of the bounding box, are therefore preferable,
   as long as the number of sectors referenced on either side is
   not too unbalanced.

   Contrary to what many say, visplane overflows are not simply
   caused by too many sidedefs, linedefs, light levels, etc. All
   of those factors are correlated with visplane overflows, but
   most importantly, so is how the node builder selects node
   lines. The number of visible changes in flats is the main
   cause of visplane overflows, with visible changes not being
   counted if only invisible regions separate the visible areas.
*/

struct Seg *PickNode_visplane(struct Seg *ts, const bbox_t bbox)
{
 struct Seg *best = ts;
 long bestcost=LONG_MAX;
 struct Seg *part;
 int cnt=0;

 for (part=ts;part;part=part->next)     /* Count once and for all */
   cnt++;

 for (part=ts;part;part=part->next)	/* Use each Seg as partition*/
  {
   struct Seg *check;
   long cost=0,slen=0;
   int tot=0,diff=cnt;
   memset(SectorHits,0,num_sects);
   progress();           	        /* Something for the user to look at.*/

   for (check=ts;check;check=check->next) /* Check partition against all Segs*/
    {
        /*     get state of lines' relation to each other    */
     long a = part->pdy * check->psx - part->pdx * check->psy + part->ptmp;
     long b = part->pdy * check->pex - part->pdx * check->pey + part->ptmp;
     unsigned char mask=2;

     if ((a^b) < 0)
       if (a && b)
	{                /* Line is split; a,b nonzero, opposite sign */
         long l=check->len;
         long d=(l*a)/(a-b);    /* Distance from start of intersection */
         if (d>=2)
          {
        /* If the linedef associated with this seg has a sector tag >= 900,
           treat it as precious; i.e. don't split it unless all other options
           are exhausted. This is used to protect deep water and invisible
           lifts/stairs from being messed up accidentally by splits. */

           if (linedefs[check->linedef].tag >= 900)
	     cost += factor*64;

           cost += factor;

	   if (cost > bestcost)    /* This is the heart of my pruning idea - */
	     goto prune;           /* it catches bad segs early on. Killough */

           tot++;                  /* Seg is clearly split */
           mask=4;
          }
         else         /* Distance from start < 2; check distance from end */
           if (l-d<2 ? check->pdx*part->pdx+check->pdy*part->pdy<0 : b<0)
             goto leftside;
        }
       else
         goto leftside;
     else
       if (a<=0 && (a || (!b && (slen+=check->len,
                     check->pdx*part->pdx+check->pdy*part->pdy<0))))
        {
         leftside:
           diff-=2;
           mask=1;
        }
     SectorHits[check->sector] |= mask;
    }

   /* Take absolute value. diff is being used to obtain the */
   /* min/max values by way of: min(a,b)=(a+b-abs(a-b))/2   */

   if ((diff-=tot) < 0)
     diff= -diff;

   /* Make sure at least one Seg is on each side of the partition*/

   if (tot+cnt <= diff)
     continue;

   /* Compute difference in number of sector
      references on each side of node line */

   for (diff=tot=0;tot<num_sects;tot++)
     switch (SectorHits[tot])
      {
       case 1:
         diff++;
         break;
       case 2:
         diff--;
         break;
      }

   if (diff<0)
     diff= -diff;

   if ((cost+=diff) >= bestcost)
     continue;

   /* If the node line is incident with SEGS in less than 1/2th of its
      length inside the bounding box, increase the cost since this is
      likely a node line cutting across a large room but only sharing
      space with a tiny SEG in the middle -- this is another contributor
      to visplane overflows. */

      {
       long l;
       if (!part->pdx)
         l=bbox[BB_TOP]-bbox[BB_BOTTOM];
       else
         if (!part->pdy)
           l=bbox[BB_RIGHT]-bbox[BB_LEFT];
         else
          {
           double t1=(part->psx-bbox[BB_RIGHT ])/(double) part->pdx;
           double t2=(part->psx-bbox[BB_LEFT  ])/(double) part->pdx;
           double t3=(part->psy-bbox[BB_TOP   ])/(double) part->pdy;
           double t4=(part->psy-bbox[BB_BOTTOM])/(double) part->pdy;
           if (part->pdx>0)
            {
             double t=t1;
             t1=t2;
             t2=t;
            }
           if (part->pdy>0)
            {
             double t=t3;
             t3=t4;
             t4=t;
            }
           l=((t1 > t3 ? t3 : t1) - (t2 < t4 ? t4 : t2))*part->len;
          }
         if (slen < l && (cost+=factor) >= bestcost)
           continue;
        }

   /* We have a new better choice */

   bestcost = cost;
   best = part;   	/* Remember which Seg*/
   prune:;              /* Early exit and skip past the tests above */
  }
 return best;		/* All finished, return best Seg*/
}


/*---------------------------------------------------------------------------*
 Calculate the point of intersection of two lines. ps?->pe? & ls?->le?
 returns int xcoord, int ycoord
*---------------------------------------------------------------------------*/

void ComputeIntersection(short int *outx,short int *outy)
{
	double a,b,a2,b2,l2,w,d;

	long dx,dy,dx2,dy2;

	dx = pex - psx;
	dy = pey - psy;
	dx2 = lex - lsx;
	dy2 = ley - lsy;

	if (dx == 0 && dy == 0) ProgError("Trouble in ComputeIntersection dx,dy");
/*	l = (long)sqrt((double)((dx*dx) + (dy*dy)));  unnecessary - killough */
	if(dx2 == 0 && dy2 == 0) ProgError("Trouble in ComputeIntersection dx2,dy2");
	l2 = (long)sqrt((double)((dx2*dx2) + (dy2*dy2)));

	a = dx /* / l */;  /* no normalization of a,b necessary,   */
	b = dy /* / l */;  /* since division by d in formula for w */
	a2 = dx2 / l2;     /* cancels it out. */
	b2 = dy2 / l2;
	d = b * a2 - a * b2;
	if (d)
		{
		w = ((a*(lsy-psy))+(b*(psx-lsx))) / d;

/*		printf("Intersection at (%f,%f)\n",x2+(a2*w),y2+(b2*w));*/

		a = lsx+(a2*w);
		b = lsy+(b2*w);
                *outx=(a<0) ? -(int)(.5-a) : (int)(.5+a);
                *outy=(b<0) ? -(int)(.5-b) : (int)(.5+b);
/*
		modf(a + ((a<0)?-0.5:0.5) ,&w);
		modf(b + ((b<0)?-0.5:0.5) ,&d);
                *outx = w;
                *outy = d;
*/
		}
              else
               {
         	*outx = lsx;
        	*outy = lsy;
               }
}

/*---------------------------------------------------------------------------*
 Because this is (was) used a horrendous amount of times in the inner loops, the
 coordinate of the lines are setup outside of the routine in global variables
 psx,psy,pex,pey = partition line coordinates
 lsx,lsy,lex,ley = checking line coordinates
 The routine returns 'val' which has 3 bits assigned to the the start and 3
 to the end. These allow a decent evaluation of the lines state.
 bit 0,1,2 = checking lines starting point and bits 4,5,6 = end point
 these bits mean 	0,4 = point is on the same line
 						1,5 = point is to the left of the line
						2,6 = point is to the right of the line
 There are some failsafes in here, these mainly check for small errors in the
 side checker.
*---------------------------------------------------------------------------*/

int DoLinesIntersect(void)
{
	short int x,y,val = 0;

	long dx2,dy2,dx3,dy3,a,b,l;

	dx2 = psx - lsx;									/* Checking line -> partition*/
	dy2 = psy - lsy;
	dx3 = psx - lex;
	dy3 = psy - ley;

	a = pdy*dx2 - pdx*dy2;
	b = pdy*dx3 - pdx*dy3;
	if ( (a ^ b) < 0 && a && b)								/* Line is split, just check that*/
		{
		ComputeIntersection(&x,&y);
		dx2 = lsx - x;									/* Find distance from line start*/
		dy2 = lsy - y;									/* to split point*/
		if(dx2 == 0 && dy2 == 0) a = 0;
		else
			{
			l = (long) dx2*dx2+(long) dy2*dy2;		                        /* If either ends of the split*/
			if (l < 4) a = 0;							/* are smaller than 2 pixs then*/
			}												/* assume this starts on part line*/
		dx3 = lex - x;									/* Find distance from line end*/
		dy3 = ley - y;									/* to split point*/
		if(dx3 == 0 && dy3 == 0) b = 0;
		else
			{
			l = (long) dx3*dx3 + (long) dy3*dy3;					/* same as start of line*/
			if (l < 4) b = 0;
			}
		}

	if(a == 0) val = val | 16;								/* start is on middle*/
         else
	if(a < 0) val = val | 32;						/* start is on left side*/
        else
	/* if(a > 0) */ val = val | 64;						/* start is on right side*/

	if (b == 0) val = val | 1;						/* end is on middle*/
        else
	if(b < 0) val = val | 2;						/* end is on left side*/
        else
	/* if(b > 0) */ val = val | 4;						/* end is on right side*/

	return val;
}
