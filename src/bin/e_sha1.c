/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e_sha1.h"
#include <string.h>
#include <arpa/inet.h>

/* externally accessible functions */

#define SHSH(n, v) ((((v) << (n)) & 0xffffffff) | ((v) >> (32 - (n))))

/* this is ALMOST canonical sha1 - i took some shortcuts to make it smaller
 * when handling the last 64 bytes in a buffer to just keep it short. it
 * still generates decent hashes either way... 
 */
EAPI int
e_sha1_sum(unsigned char *data, int size, unsigned char *dst)
{
   unsigned int digest[5], word[80], wa, wb, wc, wd, we, t;
   unsigned char buf[64], *d;
   int index, left, i;
   const unsigned int magic[4] = 
     {
        0x5a827999,
	  0x6ed9eba1,
	  0x8f1bbcdc,
	  0xca62c1d6
     };
   
   index = 0;
   digest[0] = 0x67452301;
   digest[1] = 0xefcdab89;
   digest[2] = 0x98badcfe;
   digest[3] = 0x10325476;
   digest[4] = 0xc3d2e1f0;

   for (left = size, d = data; left > 0; left--, d++)
     {
	if ((index == 0) && (left < 64))
	  {
	     memset(buf, 0, 60);
	     buf[60] = (size >> 24) & 0xff;
	     buf[61] = (size >> 16) & 0xff;
	     buf[62] = (size >>  8) & 0xff;
	     buf[63] = (size      ) & 0xff;
	  }
	buf[index] = *d;
	index++;
	if ((index == 64) || (left == 1))
	  {
	     if ((left == 1) && (index < 64)) buf[index] = 0x80;
	     for (i = 0; i < 16; i++)
	       {
		  word[i]  = (unsigned int)buf[(i * 4)    ] << 24;
		  word[i] |= (unsigned int)buf[(i * 4) + 1] << 16;
		  word[i] |= (unsigned int)buf[(i * 4) + 2] << 8;
		  word[i] |= (unsigned int)buf[(i * 4) + 3];
	       }
	     for (i = 16; i < 80; i++)
	       word[i] = SHSH(1, 
			       word[i - 3 ] ^ word[i - 8 ] ^
			       word[i - 14] ^ word[i - 16]);
	     wa = digest[0];
	     wb = digest[1];
	     wc = digest[2];
	     wd = digest[3];
	     we = digest[4];
             for (i = 0; i < 80; i++)
	       {
		  if (i < 20)
		    t = SHSH(5, wa) + ((wb & wc) | ((~wb) & wd)) + 
		    we + word[i] + magic[0];
		  else if (i < 40)
		    t = SHSH(5, wa) + (wb ^ wc ^ wd) + 
		    we + word[i] + magic[1];
		  else if (i < 60)
		    t = SHSH(5, wa) + ((wb & wc) | (wb & wd) | (wc & wd)) + 
		    we + word[i] + magic[2];
		  else if (i < 80)
		    t = SHSH(5, wa) + (wb ^ wc ^ wd) + 
		    we + word[i] + magic[3];
		  we = wd;
		  wd = wc;
		  wc = SHSH(30, wb);
		  wb = wa;
		  wa = t;
	       }
	     digest[0] += wa;
	     digest[1] += wb;
	     digest[2] += wc;
	     digest[3] += wd;
	     digest[4] += we;
	     index = 0;
	  }
     }
   
   t = htonl(digest[0]); digest[0] = t;
   t = htonl(digest[1]); digest[1] = t;
   t = htonl(digest[2]); digest[2] = t;
   t = htonl(digest[3]); digest[3] = t;
   t = htonl(digest[4]); digest[4] = t;
   
   memcpy(dst, digest, 5 * 4);
   return 1;
}

/* local subsystem functions */
