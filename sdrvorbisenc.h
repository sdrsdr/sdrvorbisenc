#ifndef SDRVORBISENC_H
#define SDRVORBISENC_H
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
 * ===== this file require  -logg -lvorbis -lvorbisenc
 * =======================
 */

#include <vorbis/vorbisenc.h>

//callback protos
typedef int (*sdrvorbisenc_bytesout) (void *buf,unsigned bufsz, void*udata);
typedef void (*sdrvorbisenc_addcomment) (vorbis_comment  *vc);

//state struct
typedef struct sdrvorbisenc_state_ {
	void*udata;
	sdrvorbisenc_bytesout bytesout;
	sdrvorbisenc_addcomment addcomment;
	int channels;
	
	ogg_stream_state os; /* take physical pages, weld into a logical
	stream of packets */
	ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */
	
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream
	settings */
	vorbis_comment   vc; /* struct that stores all the user comments */
	
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */
	
	
} sdrvorbisenc_state;

//alloc and init state
sdrvorbisenc_state* sdrvorbisenc_init(void*udata,sdrvorbisenc_bytesout bytesout, sdrvorbisenc_addcomment addcomment ,int channels,int rate, float q);

//Submit interleaved samples for encoding. numsamples is total number of items
//pointed to samples, numsamples shoud be a multiply of channels: if you have
//10 samples for each of the two stereo channels you shoud submit 20 values pointed
//by samples and set numsamples to 20 
//submit numsamples=0 for eos marker (samples is ignored)
int sdrvorbisenc_submit (sdrvorbisenc_state*ves, float*samples, int numsamples);

//metadata:
void sdrvorbisenc_meta(sdrvorbisenc_state*ves, const char* key,const char *value);
void sdrvorbisenc_meta_flush(sdrvorbisenc_state*ves);

//cleanup state
void sdrvorbisenc_cleanup(sdrvorbisenc_state*ves);

#endif