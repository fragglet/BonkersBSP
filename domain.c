/* Level domain support, used for marking certain areas in order
   to perform certain rendering tricks that exploit the BSP tree
   structure.
 */

#include <stdlib.h>

#include "structs.h"
#include "bsp.h"

#define DOMAIN_BORDER_SPECIAL   0x2000

struct Domain {
    int tag;
    struct LineDef **borders;
    int num_borders;
};

static int16_t *subsector_domains;
static struct Domain *domains;
static int num_domains;

static struct Domain *DomainForTag(int tag)
{
    struct Domain *result;
    int i;

    for (i = 0; i < num_domains; ++i) {
        if (domains[i].tag == tag) {
            return &domains[i];
        }
    }

    /* New domain that has not been recognized before. */
    domains = realloc(domains, sizeof(struct Domain) * (num_domains + 1));
    result = &domains[num_domains];
    ++num_domains;

    result->tag = tag;
    result->borders = NULL;
    result->num_borders = 0;

    return result;
}

/* Use the linedefs array to generate the list of domains. */
void GenerateDomains(void)
{
    struct Domain *d;
    int l;

    domains = NULL;
    num_domains = 0;

    for (l = 0; l < num_lines; ++l) {
        if (linedefs[l].type == DOMAIN_BORDER_SPECIAL
         && linedefs[l].tag != 0) {
            d = DomainForTag(linedefs[l].tag);
            d->borders = realloc(d->borders,
                                 (sizeof(struct Linedef *)
                                   * (d->num_borders + 1)));
            ++d->num_borders;
        }
    }
}

static int LineIntersection(struct LineDef *linedef, int *x_result, int y)
{
    struct Vertex *v1, *v2;
    double frac;

    v1 = &vertices[linedef->start];
    v2 = &vertices[linedef->end];

    if (v1->y == v2->y) {
        return 0;
    }

    frac = (double) (y - v1->y) / (v2->y - v1->y);

    *x_result = v1->x + (int) (frac * (v2->x - v1->x));
    return frac > -1 && frac < 1;
}

/* Returns true if the given point is in the given domain */
static int PointInDomain(struct Domain *d, int x, int y)
{
    int i, xi;
    int intersections = 0;

    /* Point-in-polygon algorithm. We check for intersection with
       a horizontal line going through (x, y). */
    for (i = 0; i < d->num_borders; ++i) {
        if (LineIntersection(d->borders[i], &xi, y) && xi < x) {
            ++intersections;
        }
    }

    /* Odd number of intersections to the left of (x, y) implies the
       point is inside the domain. */
    return (intersections % 2) == 1;
}

/* Given a point, find what domain it is part of, or NULL for no domain. */
static struct Domain *DomainForPoint(int x, int y)
{
    int d;

    for (d = 0; d < num_domains; ++d) {
        if (PointInDomain(&domains[d], x, y)) {
            return &domains[d];
        }
    }

    return NULL;
}

/* Given subsector's bounding box, find the domain it belongs to. */
static int SubsectorDomain(bbox_t bbox)
{
    struct Domain *d;
    int x, y;

    /* Test center point of bounding box. Because ssectors are convex
       polygons it's enough to just test a single point - either the
       entire ssector is within the domain or none of it is. */
    d = DomainForPoint((bbox[BB_LEFT] + bbox[BB_RIGHT]) / 2,
                       (bbox[BB_TOP] + bbox[BB_BOTTOM]) / 2);
    if (d == NULL) {
        return 0;
    } else {
        return d->tag;
    }
}

/* Walk the BSP tree and set the domain fields for subsectors. */
static void AnnotateNode(struct Node *node)
{
    int ss;

    if ((node->chleft & 0x8000) != 0) {
        ss = node->chleft & ~0x8000;
        subsector_domains[ss] = SubsectorDomain(node->leftbox);
    } else {
        AnnotateNode(node->nextl);
    }

    if ((node->chright & 0x8000) != 0) {
        ss = node->chright & ~0x8000;
        subsector_domains[ss] = SubsectorDomain(node->rightbox);
    } else {
        AnnotateNode(node->nextr);
    }
}

void AnnotateNodeTree(struct Node *root)
{
    int i;

    subsector_domains = calloc(num_ssectors, sizeof(int16_t));
    for (i = 0; i < num_ssectors; ++i) {
        subsector_domains[i] = 0;
    }

    AnnotateNode(root);
}

