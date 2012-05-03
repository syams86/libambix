/* jcommon/common.h -  common helper functions for jack-utils            -*- c -*-

   Copyright � 2003-2001 Rohan Drape
   Copyright � 2012 IOhannes m zm�lnig <zmoelnig@iem.at>.
         Institute of Electronic Music and Acoustics (IEM),
         University of Music and Dramatic Arts, Graz

   This file is based on Rohan Drape's "jack-tools" collection

   you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   ambix-jplay is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/



#ifndef FAILURE
#define FAILURE exit(1)
#endif

#define eprintf(...) fprintf(stderr,__VA_ARGS__)

static void *xmalloc(size_t size)
{
  void *p = malloc(size);
  if(p == NULL) {
    fprintf(stderr, "malloc() failed: %ld\n", (long)size);  
    FAILURE;
  }
  return p;
}

static void jack_client_minimal_error_handler(const char *desc)
{
  eprintf("jack error: %s\n", desc);
}

static void jack_client_minimal_shutdown_handler(void *arg)
{
  eprintf("jack shutdown\n");
  FAILURE;
}
