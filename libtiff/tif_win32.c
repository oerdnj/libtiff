/* $Id: tif_win32.c,v 1.29 2007-06-27 16:09:58 joris Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library Win32-specific Routines.  Adapted from tif_unix.c 4/5/95 by
 * Scott Wagner (wagner@itek.com), Itek Graphix, Rochester, NY USA
 */
#include "tiffiop.h"

#include <windows.h>

static tmsize_t
_tiffReadProc(thandle_t fd, void* buf, tmsize_t size)
{
	/* tmsize_t is 64bit on 64bit systems, but the WinAPI ReadFile takes
	 * 32bit sizes, so we loop through the data in suitable 32bit sized
	 * chunks */
	uint8* ma;
	uint64 mb;
	DWORD n;
	DWORD o;
	tmsize_t p;
	ma=(uint8*)buf;
	mb=size;
	p=0;
	while (mb>0)
	{
		n=0x80000000UL;
		if ((uint64)n>mb)
			n=(DWORD)mb;
		if (!ReadFile(fd,(LPVOID)ma,n,&o,NULL))
			return(0);
		ma+=o;
		mb-=o;
		p+=o;
		if (o!=n)
			break;
	}
	return ((tmsize_t)p);
}

static tmsize_t
_tiffWriteProc(thandle_t fd, void* buf, tmsize_t size)
{
	/* tmsize_t is 64bit on 64bit systems, but the WinAPI WriteFile takes
	 * 32bit sizes, so we loop through the data in suitable 32bit sized
	 * chunks */
	uint8* ma;
	uint64 mb;
	DWORD n;
	DWORD o;
	tmsize_t p;
	ma=(uint8*)buf;
	mb=size;
	p=0;
	while (mb>0)
	{
		n=0x80000000UL;
		if ((uint64)n>mb)
			n=(DWORD)mb;
		if (!WriteFile(fd,(LPVOID)ma,n,&o,NULL))
			return(0);
		ma+=o;
		mb-=o;
		p+=o;
		if (o!=n)
			break;
	}
	return ((tmsize_t)p);
}

static uint64
_tiffSeekProc(thandle_t fd, uint64 off, int whence)
{
	LARGE_INTEGER off_in, off_out;
	DWORD dwMoveMethod;

	off_in.QuadPart = off;

	switch(whence)
	{
	case SEEK_SET:
		dwMoveMethod = FILE_BEGIN;
		break;
	case SEEK_CUR:
		dwMoveMethod = FILE_CURRENT;
		break;
	case SEEK_END:
		dwMoveMethod = FILE_END;
		break;
	default:
		dwMoveMethod = FILE_BEGIN;
		break;
	}
	if (SetFilePointerEx(fd, off_in, &off_out, dwMoveMethod)==0)
		off_out.QuadPart=0;
	return(off_out.QuadPart);
}

static int
_tiffCloseProc(thandle_t fd)
{
	return (CloseHandle(fd) ? 0 : -1);
}

static uint64
_tiffSizeProc(thandle_t fd)
{
	ULARGE_INTEGER m;
	m.LowPart=GetFileSize(fd,&m.HighPart);
	return(m.QuadPart);
}

static int
_tiffDummyMapProc(thandle_t fd, void** pbase, tmsize_t* psize)
{
	(void) fd;
	(void) pbase;
	(void) psize;
	return (0);
}

/*
 * From "Hermann Josef Hill" <lhill@rhein-zeitung.de>:
 *
 * Windows uses both a handle and a pointer for file mapping,
 * but according to the SDK documentation and Richter's book
 * "Advanced Windows Programming" it is safe to free the handle
 * after obtaining the file mapping pointer
 *
 * This removes a nasty OS dependency and cures a problem
 * with Visual C++ 5.0
 */
static int
_tiffMapProc(thandle_t fd, void** pbase, tmsize_t* psize)
{
	uint64 size;
	tmsize_t sizem;
	HANDLE hMapFile;

	size = _tiffSizeProc(fd);
	sizem = (tmsize_t)size;
	if ((uint64)sizem!=size)
		return (0);
	hMapFile = CreateFileMapping(fd, NULL, PAGE_READONLY, 0, sizem, NULL);
	if (hMapFile == NULL)
		return (0);
	*pbase = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(hMapFile);
	if (*pbase == NULL)
		return (0);
	*psize = sizem;
	return(1);
}

static void
_tiffDummyUnmapProc(thandle_t fd, void* base, tmsize_t size)
{
	(void) fd;
	(void) base;
	(void) size;
}

static void
_tiffUnmapProc(thandle_t fd, void* base, tmsize_t size)
{
	(void) fd;
	(void) size;
	UnmapViewOfFile(base);
}

/*
 * Open a TIFF file descriptor for read/writing.
 * Note that TIFFFdOpen and TIFFOpen recognise the character 'u' in the mode
 * string, which forces the file to be opened unmapped.
 */
TIFF*
TIFFFdOpen(int ifd, const char* name, const char* mode)
{
	TIFF* tif;
	int fSuppressMap;
	int m;
	fSuppressMap=0;
	for (m=0; mode[m]!=0; m++)
	{
		if (mode[m]=='u')
		{
			fSuppressMap=1;
			break;
		}
	}
	tif = TIFFClientOpen(name, mode, (thandle_t)ifd,
			_tiffReadProc, _tiffWriteProc,
			_tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
			fSuppressMap ? _tiffDummyMapProc : _tiffMapProc,
			fSuppressMap ? _tiffDummyUnmapProc : _tiffUnmapProc);
	if (tif)
		tif->tif_fd = ifd;
	return (tif);
}

#ifndef _WIN32_WCE

/*
 * Open a TIFF file for read/writing.
 */
TIFF*
TIFFOpen(const char* name, const char* mode)
{
	static const char module[] = "TIFFOpen";
	thandle_t fd;
	int m;
	DWORD dwMode;
	TIFF* tif;

	m = _TIFFgetMode(mode, module);

	switch(m)
	{
	case O_RDONLY:
		dwMode = OPEN_EXISTING;
		break;
	case O_RDWR:
		dwMode = OPEN_ALWAYS;
		break;
	case O_RDWR|O_CREAT:
		dwMode = OPEN_ALWAYS;
		break;
	case O_RDWR|O_TRUNC:
		dwMode = CREATE_ALWAYS;
		break;
	case O_RDWR|O_CREAT|O_TRUNC:
		dwMode = CREATE_ALWAYS;
		break;
	default:
		return ((TIFF*)0);
	}
	fd = (thandle_t)CreateFileA(name,
		(m == O_RDONLY)?GENERIC_READ:(GENERIC_READ | GENERIC_WRITE),
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, dwMode,
		(m == O_RDONLY)?FILE_ATTRIBUTE_READONLY:FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (fd == INVALID_HANDLE_VALUE) {
		TIFFErrorExt(0, module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}

	tif = TIFFFdOpen((int)fd, name, mode);
	if(!tif)
		CloseHandle(fd);
	return tif;
}

/*
 * Open a TIFF file with a Unicode filename, for read/writing.
 */
TIFF*
TIFFOpenW(const wchar_t* name, const char* mode)
{
	static const char module[] = "TIFFOpenW";
	thandle_t fd;
	int m;
	DWORD dwMode;
	int mbsize;
	char *mbname;
	TIFF *tif;

	m = _TIFFgetMode(mode, module);

	switch(m) {
		case O_RDONLY:			dwMode = OPEN_EXISTING; break;
		case O_RDWR:			dwMode = OPEN_ALWAYS;   break;
		case O_RDWR|O_CREAT:		dwMode = OPEN_ALWAYS;   break;
		case O_RDWR|O_TRUNC:		dwMode = CREATE_ALWAYS; break;
		case O_RDWR|O_CREAT|O_TRUNC:	dwMode = CREATE_ALWAYS; break;
		default:			return ((TIFF*)0);
	}

	fd = (thandle_t)CreateFileW(name,
		(m == O_RDONLY)?GENERIC_READ:(GENERIC_READ|GENERIC_WRITE),
		FILE_SHARE_READ, NULL, dwMode,
		(m == O_RDONLY)?FILE_ATTRIBUTE_READONLY:FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (fd == INVALID_HANDLE_VALUE) {
		TIFFErrorExt(0, module, "%S: Cannot open", name);
		return ((TIFF *)0);
	}

	mbname = NULL;
	mbsize = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
	if (mbsize > 0) {
		mbname = (char *)_TIFFmalloc(mbsize);
		if (!mbname) {
			TIFFErrorExt(0, module,
			"Can't allocate space for filename conversion buffer");
			return ((TIFF*)0);
		}

		WideCharToMultiByte(CP_ACP, 0, name, -1, mbname, mbsize,
				    NULL, NULL);
	}

	tif = TIFFFdOpen((int)fd,
			 (mbname != NULL) ? mbname : "<unknown>", mode);
	if(!tif)
		CloseHandle(fd);

	_TIFFfree(mbname);

	return tif;
}

#endif /* ndef _WIN32_WCE */


void*
_TIFFmalloc(tmsize_t s)
{
	return ((void*)GlobalAlloc(GMEM_FIXED, s));
}

void
_TIFFfree(void* p)
{
	GlobalFree((HGLOBAL)p);
	return;
}

void*
_TIFFrealloc(void* p, tmsize_t s)
{
	void* pvTmp;
	tmsize_t old;

	if(p == NULL)
		return ((void*)GlobalAlloc(GMEM_FIXED, s));

	old = (tmsize_t)GlobalSize(p);

	if (old>=s) {
		if ((pvTmp = (void*)GlobalAlloc(GMEM_FIXED, s)) != NULL) {
			CopyMemory(pvTmp, p, s);
			GlobalFree((HGLOBAL)p);
		}
	} else {
		if ((pvTmp = (void*)GlobalAlloc(GMEM_FIXED, s)) != NULL) {
			CopyMemory(pvTmp, p, old);
			GlobalFree((HGLOBAL)p);
		}
	}
	return (pvTmp);
}

void
_TIFFmemset(void* p, int v, tmsize_t c)
{
	FillMemory(p, c, (BYTE)v);
}

void
_TIFFmemcpy(void* d, const void* s, tmsize_t c)
{
	CopyMemory(d, s, c);
}

int
_TIFFmemcmp(const void* p1, const void* p2, tmsize_t c)
{
	register const BYTE *pb1 = (const BYTE *) p1;
	register const BYTE *pb2 = (const BYTE *) p2;
	register tmsize_t dwTmp = c;
	register int iTmp;
	for (iTmp = 0; dwTmp-- && !iTmp; iTmp = (int)*pb1++ - (int)*pb2++)
		;
	return (iTmp);
}

#ifndef _WIN32_WCE

static void
Win32WarningHandler(const char* module, const char* fmt, va_list ap)
{
#ifndef TIF_PLATFORM_CONSOLE
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Warning";
	LPCTSTR szDefaultModule = "LIBTIFF";
	LPCTSTR szTmpModule = (module == NULL) ? szDefaultModule : module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (strlen(szTmpModule) +
		strlen(szTitleText) + strlen(fmt) + 128)*sizeof(char))) == NULL)
		return;
	sprintf(szTitle, szTitleText, szTmpModule);
	szTmp = szTitle + (strlen(szTitle)+2)*sizeof(char);
	vsprintf(szTmp, fmt, ap);
	MessageBoxA(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONINFORMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif        
}
TIFFErrorHandler _TIFFwarningHandler = Win32WarningHandler;

static void
Win32ErrorHandler(const char* module, const char* fmt, va_list ap)
{
#ifndef TIF_PLATFORM_CONSOLE
	LPTSTR szTitle;
	LPTSTR szTmp;
	LPCTSTR szTitleText = "%s Error";
	LPCTSTR szDefaultModule = "LIBTIFF";
	LPCTSTR szTmpModule = (module == NULL) ? szDefaultModule : module;
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (strlen(szTmpModule) +
		strlen(szTitleText) + strlen(fmt) + 128)*sizeof(char))) == NULL)
		return;
	sprintf(szTitle, szTitleText, szTmpModule);
	szTmp = szTitle + (strlen(szTitle)+2)*sizeof(char);
	vsprintf(szTmp, fmt, ap);
	MessageBoxA(GetFocus(), szTmp, szTitle, MB_OK | MB_ICONEXCLAMATION);
	LocalFree(szTitle);
	return;
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, ".\n");
#endif        
}
TIFFErrorHandler _TIFFerrorHandler = Win32ErrorHandler;

#endif /* ndef _WIN32_WCE */

/* vim: set ts=8 sts=8 sw=8 noet: */
