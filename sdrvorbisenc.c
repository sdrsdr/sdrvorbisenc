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


#include <stdlib.h>
#include <string.h>

#include "sdrvorbisenc.h"


sdrvorbisenc_state* sdrvorbisenc_init(void*udata,sdrvorbisenc_bytesout bytesout, sdrvorbisenc_addcomment addcomment ,int channels,int rate, float q) {
	sdrvorbisenc_state* ves=malloc(sizeof(sdrvorbisenc_state));
	if (!ves) return NULL;
	ves->udata=udata;
	ves->bytesout=bytesout;
	ves->addcomment=addcomment;
	ves->channels=channels;
	
	vorbis_info_init(&ves->vi);
	
	int ret=vorbis_encode_init_vbr(&ves->vi,channels,rate,q);
	
	/* do not continue if setup failed; this can happen if we ask for a
	 *     mode that libVorbis does not support (eg, too low a bitrate, etc,
	 *     will return 'OV_EIMPL') */
	
	if(ret) {
		free(ves);
		return NULL;
	}
	
	/* add a comment */
	vorbis_comment_init(&ves->vc);
	vorbis_comment_add_tag(&ves->vc,"ENCODER","encoder_test.c");
	if (ves->addcomment) ves->addcomment(&ves->vc);
	
	/* set up the analysis state and auxiliary encoding storage */
	vorbis_analysis_init(&ves->vd,&ves->vi);
	vorbis_block_init(&ves->vd,&ves->vb);
	
	/* set up our packet->stream encoder */
	/* pick a random serial number; that way we can more likely build
	 *     chained streams just by concatenation */
	ogg_stream_init(&ves->os,0);
	
	/* Vorbis streams begin with three headers; the initial header (with
	 *     most of the codec setup parameters) which is mandated by the Ogg
	 *     bitstream spec.  The second header holds any comment fields.  The
	 *     third header holds the bitstream codebook.  We merely need to
	 *     make the headers, then pass them to libvorbis one at a time;
	 *     libvorbis handles the additional Ogg bitstream constraints */
	
	{
		ogg_packet header;
		ogg_packet header_comm;
		ogg_packet header_code;
		
		vorbis_analysis_headerout(&ves->vd,&ves->vc,&header,&header_comm,&header_code);
		ogg_stream_packetin(&ves->os,&header); /* automatically placed in its own
		page */
		ogg_stream_packetin(&ves->os,&header_comm);
		ogg_stream_packetin(&ves->os,&header_code);
		
		/* This ensures the actual
		 * audio data will start on a new page, as per spec
		 */
		while(ogg_stream_flush(&ves->os,&ves->og)!=0){
			ves->bytesout(ves->og.header,ves->og.header_len,ves->udata);
			ves->bytesout(ves->og.body,ves->og.body_len,ves->udata);
		}
		
	}
	return ves;
}

int sdrvorbisenc_submit (sdrvorbisenc_state*ves, float*samples, int numsamples){
	if (numsamples==0){
		vorbis_analysis_wrote(&ves->vd,0);
		return 1;
	}

	if (ves->channels==1) {
		float **buffer=vorbis_analysis_buffer(&ves->vd,numsamples);
		
		memcpy(buffer[0],samples,numsamples*sizeof(float));
		
		/* tell the library how much we actually submitted */
		vorbis_analysis_wrote(&ves->vd,numsamples);
	} else {
		int perchansamples=numsamples/ves->channels;
		float **buffer=vorbis_analysis_buffer(&ves->vd,perchansamples);
		
		//deinterlace:
		int curchan=0,curchanidx=0,samplesleft=numsamples;
		float *cursamp=samples;
		
		while (samplesleft){
			buffer[curchan][curchanidx]=*cursamp;
			samplesleft--; cursamp++; 
			int maxchani=ves->channels-1;
			if (curchan==maxchani){
				curchan=0; curchanidx++;
			} else curchan++;
		}
		vorbis_analysis_wrote(&ves->vd,perchansamples);
	}
	
	/* vorbis does some data preanalysis, then divvies up blocks for
	 *       more involved (potentially parallel) processing.  Get a single
	 *       block for encoding now */
	while(vorbis_analysis_blockout(&ves->vd,&ves->vb)==1){
		
		/* analysis, assume we want to use bitrate management */
		vorbis_analysis(&ves->vb,NULL);
		vorbis_bitrate_addblock(&ves->vb);
		
		while(vorbis_bitrate_flushpacket(&ves->vd,&ves->op)){
			
			/* weld the packet into the bitstream */
			ogg_stream_packetin(&ves->os,&ves->op);
			
			/* write out pages (if any) */
			int eos=0;
			while(!eos){
				int result=ogg_stream_pageout(&ves->os,&ves->og);
				if(result==0)break;
				ves->bytesout(ves->og.header,ves->og.header_len,ves->udata);
				ves->bytesout(ves->og.body,ves->og.body_len,ves->udata);
				
				/* this could be set above, but for illustrative purposes, I do
				 *             it here (to show that vorbis does know where the stream ends) */
				
				if(ogg_page_eos(&ves->og)) eos=1;
			}
			if (eos) return 1;
		}
	}
	return 0;
	
}
void sdrvorbisenc_cleanup(sdrvorbisenc_state*ves){
	/* clean up.  vorbis_info_clear() must be called last */
	ogg_stream_clear(&ves->os);
	vorbis_block_clear(&ves->vb);
	vorbis_dsp_clear(&ves->vd);
	vorbis_comment_clear(&ves->vc);
	vorbis_info_clear(&ves->vi);
	free (ves);
}