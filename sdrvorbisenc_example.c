/***************************************************************************
 *   Copyright (C) 2007 by Stoian Ivanov                                   *
 *   s.ivanov@teracomm.bg                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License version  *
 *   2 as published by the Free Software Foundation;                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
 * =======================
 * ===== this file require -logg -lvorbis -lvorbisenc -lsdrvorbisenc 
 * =======================
 */

/* 
	takes a mono f32 bit 44.1kHz file from test.f32 and encodes it into test.ogg 
*/

/* Note that this is POSIX, not ANSI, code */

#include <stdio.h>
#include <stdlib.h>
#include "sdrvorbisenc.h"


#define READ 1024

int test_bytesout (void *buf,unsigned bufsz, void*udata){
	FILE *outfile=(FILE *)udata;
	return fwrite(buf,1,bufsz,outfile);
}

int main(){

	float readbuffer[READ]; 
	int eos=0;
	
	FILE *infile=fopen("test.f32","r");
	if (!infile) {
		printf("can't open test.f32 for reading!\n");
		return -1;
	}
	FILE *outfile=fopen("test.ogg","w");
	if (!outfile) {
		printf("can't open test.ogg for writing!\n");
		return -1;
	}
	
	sdrvorbisenc_state *ves=sdrvorbisenc_init(outfile,test_bytesout, NULL, 1,44100,0.1);
	if (!ves) {
		printf("can't sdrvorbisenc_init(...)\n");
		return -1;
	}
	
	while(!eos){
		long floats=fread(readbuffer,sizeof(float),READ,infile);
		if(floats==0) eos=sdrvorbisenc_submit(ves,NULL,0);
		else eos=sdrvorbisenc_submit(ves,readbuffer,floats);
	}


	sdrvorbisenc_cleanup(ves);
	
	fprintf(stderr,"Done.\n");
	return(0);
}
