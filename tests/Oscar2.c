#include <stdio.h>
#include <stdlib.h>

#define  nil		0
#define	 false		0
#define  true		1
#define  bubblebase	1.61f
#define  dnfbase 	3.5f
#define  permbase 	1.75f
#define  queensbase 1.83f
#define  towersbase 2.39f
#define  quickbase 	1.92f
#define  intmmbase 	1.46f
#define  treebase 	2.5f
#define  mmbase 	0.0f
#define  fpmmbase 	2.92f
#define  puzzlebase	0.5f
#define  fftbase 	0.0f
#define  fpfftbase 	4.44f
    /* Towers */
#define maxcells 	 18

    /* Intmm, Mm */
#define rowsize 	 40

    /* Puzzle */
#define size	 	 511
#define classmax 	 3
#define typemax 	 12
#define d 		     8

    /* Bubble, Quick */
#define sortelements 5000
#define srtelements  500

    /* fft */
#define fftsize 	 256
#define fftsize2 	 129
/*
type */
    /* Perm */
#define permrange     10

   /* tree */
struct node
{
  struct node *left, *right;
  int val;
};

		 /* Towers *//*
		    discsizrange = 1..maxcells; */
#define    stackrange	3
/*    cellcursor = 0..maxcells; */
struct element
{
  int discsize;
  int next;
};
/*    emsgtype = packed array[1..15] of char;
*/
		    /* Intmm, Mm *//*
		       index = 1 .. rowsize;
		       intmatrix = array [index,index] of integer;
		       realmatrix = array [index,index] of real;
		     */
		 /* Puzzle *//*
		    piececlass = 0..classmax;
		    piecetype = 0..typemax;
		    position = 0..size;
		  */
			/* Bubble, Quick *//*
			   listsize = 0..sortelements;
			   sortarray = array [listsize] of integer;
			 */
    /* FFT */
struct complex
{
  float rp, ip;
};
/*
    carray = array [1..fftsize] of complex ;
    c2array = array [1..fftsize2] of complex ;
*/

float value, fixed, floated;

    /* global */
long seed;			/* converted to long for 16 bit WR */

    /* Perm */
int permarray[permrange + 1];
/* converted pctr to unsigned int for 16 bit WR*/
unsigned int pctr;

    /* tree */
struct node *tree;

    /* Towers */
int stack[stackrange + 1];
struct element cellspace[maxcells + 1];
int freelist, movesdone;

    /* Intmm, Mm */

int ima[rowsize + 1][rowsize + 1], imb[rowsize + 1][rowsize + 1],
  imr[rowsize + 1][rowsize + 1];
float rma[rowsize + 1][rowsize + 1], rmb[rowsize + 1][rowsize + 1],
  rmr[rowsize + 1][rowsize + 1];

    /* Puzzle */
int piececount[classmax + 1], class[typemax + 1], piecemax[typemax + 1];
int puzzl[size + 1], p[typemax + 1][size + 1], n, kount;

    /* Bubble, Quick */
int sortlist[sortelements + 1], biggest, littlest, top;

    /* FFT */
struct complex z[fftsize + 1], w[fftsize + 1], e[fftsize2 + 1];
float zr, zi;
		/* uniform */

void
Exptab (int n, struct complex e[])
{				/* exptab */
  float h[26];
  int i, j, k, l, m;


        do
  	{
  	  e[k + 1].rp = h[j] * (e[k + i + 1].rp + e[k - i + 1].rp);
  	}
        while (k <= m);

}				/* exptab */
