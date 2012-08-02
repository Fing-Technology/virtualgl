/* Copyright (C)2009-2011 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "rrtransport.h"
#include "vgltransconn.h"


static rrerror err;
char errstr[MAXSTR];

static FakerConfig *_fconfig=NULL;
static Window _win=0;


FakerConfig *fconfig_instance(void) {return _fconfig;}


/* This just wraps the vgltransconn class in order to demonstrate how to
   build a custom transport plugin for VGL and also to serve as a sanity
   check for the plugin API */

extern "C" {

void *RRTransInit(Display *dpy, Window win, FakerConfig *fconfig)
{
	void *handle=NULL;
	try
	{
		_fconfig=fconfig;
		_win=win;
		handle=(void *)(new vgltransconn());
	}
	catch(rrerror &e)
	{
		err=e;  return NULL;
	}
	return handle;
}


int RRTransConnect(void *handle, char *receiver_name, int port)
{
	int ret=0;
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		vglconn->connect(receiver_name, port);
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}


RRFrame *RRTransGetFrame(void *handle, int width, int height, int format,
	int stereo)
{
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		RRFrame *frame=new RRFrame;
		if(!frame) _throw("Memory allocation error");
		memset(frame, 0, sizeof(RRFrame));
		int compress=_fconfig->compress;
		if(compress==RRCOMP_PROXY || compress==RRCOMP_RGB) compress=RRCOMP_RGB;
		else compress=RRCOMP_JPEG;
		int flags=RRFRAME_BOTTOMUP, pixelsize=3;
		if(compress!=RRCOMP_RGB)
		{
			switch(format)
			{
				case RRTRANS_BGR:
					flags|=RRFRAME_BGR;  break;
				case RRTRANS_RGBA:
					pixelsize=4;  break;
				case RRTRANS_BGRA:
					flags|=RRFRAME_BGR;  pixelsize=4;  break;
				case RRTRANS_ABGR:
					flags|=(RRFRAME_BGR|RRFRAME_ALPHAFIRST);  pixelsize=4;  break;
				case RRTRANS_ARGB:
					flags|=RRFRAME_ALPHAFIRST;  pixelsize=4;  break;
			}
		}
		rrframe *f=vglconn->getframe(width, height, pixelsize, flags,
			(bool)stereo);
		f->_h.compress=compress;
		frame->opaque=(void *)f;
		frame->w=f->_h.framew;
		frame->h=f->_h.frameh;
		frame->pitch=f->_pitch;
		frame->bits=f->_bits;
		frame->rbits=f->_rbits;
		for(int i=0; i<RRTRANS_FORMATOPT; i++)
		{
			if(rrtrans_bgr[i]==(f->_flags&RRFRAME_BGR? 1:0)
				&& rrtrans_afirst[i]==(f->_flags&RRFRAME_ALPHAFIRST? 1:0)
				&& rrtrans_ps[i]==f->_pixelsize)
				{frame->format=i;  break;}
		}
		return frame;
	}
	catch(rrerror &e)
	{
		err=e;  return NULL;
	}
}


int RRTransReady(void *handle)
{
	int ret=-1;
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		ret=(int)vglconn->ready();
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransSynchronize(void *handle)
{
	int ret=0;
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		vglconn->synchronize();
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransSendFrame(void *handle, RRFrame *frame, int sync)
{
	int ret=0;
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		rrframe *f;
		if(!frame || (f=(rrframe *)frame->opaque)==NULL)
			_throw("Invalid frame handle");
		f->_h.qual=_fconfig->qual;
		f->_h.subsamp=_fconfig->subsamp;
		f->_h.winid=_win;
		vglconn->sendframe(f);
		delete frame;
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}


int RRTransDestroy(void *handle)
{
	int ret=0;
	try
	{
		vgltransconn *vglconn=(vgltransconn *)handle;
		if(!vglconn) _throw("Invalid handle");
		delete vglconn;
	}
	catch(rrerror &e)
	{
		err=e;  return -1;
	}
	return ret;
}


const char *RRTransGetError(void)
{
	snprintf(errstr, MAXSTR-1, "Error in %s -- %s",
		err.getMethod(), err.getMessage());
	return errstr;
}


} // extern "C"