#include <stdio.h>
#include <stdlib.h>

struct complex
{
  float rp, ip;
};

void
Exptab (int n, struct complex e[])
{				/* exptab */
  int i = 1,
      k = 2,
      m = 3;


        do
  	{
  	  e[k + 1].rp = e[k + i + 1].rp + e[k - i + 1].rp;
  	}
        while (k <= m);

}				/* exptab */
