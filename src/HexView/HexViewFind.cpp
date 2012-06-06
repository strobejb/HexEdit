//
//  HexViewFind.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

//
//	Borrowed from Michael Lecuyer's Java source 1998
//
//
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <tchar.h>
#include <trace.h>
#include "HexView.h"
#include "HexViewInternal.h"

#define MAX_PAT_LEN 256
#define MAX_CHAR	256

BYTE		pattern[MAX_PAT_LEN];
unsigned	skip[MAX_CHAR];
unsigned	d[MAX_CHAR];
unsigned	patpos;
unsigned	patlen;

BOOL HexView::FindInit(BYTE *pat, size_t length, BOOL searchback, BOOL matchcase)
{
	SearchCompile(pat, length);
	return TRUE;
}

bool HexView::SearchCompile(BYTE *pat, size_t length)
{
	unsigned j, k, t, t1, q, q1;
	unsigned f[MAX_PAT_LEN];
	
	patlen = (unsigned)length;
	
	if(patlen == 0 || patlen > MAX_PAT_LEN)
		return 0;
	
	memcpy(pattern, pat, patlen);
	
	for(k = 0; k < MAXCHAR; k++)
		skip[k] = patlen;
	
	for(k = 1; k <= patlen; k++)
	{
		d[k-1] = (patlen << 1) - k;
		skip[pattern[k-1]] = patlen - k;	
	}
	
	for(t = patlen + 1, j = patlen; j > 0; j--)
	{
		f[j-1] = t;
		
		while(t <= patlen && pattern[j-1] != pattern[t-1])
		{
			d[t-1] = min(d[t-1], patlen-j);
			t = f[t-1];
		}
		
		t--;
	}
	
	
	q = t;
	t = patlen + 1 - q;
	q1 = 1;
	t1 = 0;
	
	for(j = 1; j <= t; j++)
	{
		f[j-1] = t1;
		while (t1 >= 1 && pat[j-1] != pat[t1-1])
			t1 = f[t1-1];
		t1++;
	}
	
	while(q < patlen)
	{
		for(k = q1; k <= q; k++)
			d[k-1] = min(d[k-1], patlen + q - k);
		
		q1 = q + 1;
		q  = q + t - f[t-1];
		t  = f[t-1];
	}

	return TRUE;
}

int HexView::SearchBlock(BYTE *block, int start, int length, int *partial, bool matchcase /*=true*/)
{
	int		incr	 = 0;
	int		j		 = 0;
	int		blocklen = start + length;
	int		k;

	*partial = -1;

	for(k = start + patlen - 1; k < blocklen; )
	{
		if(matchcase)
		{
			// loop over pattern from right->left
			for(j = patlen - 1; j >= 0 && block[k] == pattern[j]; j--)
				k--;
		}
		else
		{
			// loop over pattern from right->left
			for(j = patlen - 1; j >= 0 && toupper(block[k]) == toupper(pattern[j]); j--)
				k--;
		}

		// found!
		if(j < 0)
		{
			//printf("found at %d\n", k+1);
			return k+1;
		}

		incr = max(skip[block[k]], d[j]);
		k += incr;
	}

	// if we're near end of buffer
	if(k >= blocklen && j > 0)
	{
		*partial = k - incr - 1;
		return -1;
	}
	
	return -1;
}

LRESULT HexView::QueryProgressNotify(UINT code, size_w pos, size_w len)
{
	NMHVPROGRESS nmhvp;

	nmhvp.hdr.code     = code;
	nmhvp.hdr.hwndFrom = m_hWnd;
	nmhvp.hdr.idFrom   = GetWindowLongPtr(m_hWnd, GWL_ID);
	
	nmhvp.len = len;
	nmhvp.pos = pos;

	return SendMessage(GetParent(m_hWnd), WM_NOTIFY, 0, (LPARAM)&nmhvp);
}

BOOL HexView::FindNext(size_w *result, UINT options)
{
	BYTE	block[1000];

	// are we searching just the selected area, or the entire file?
	bool	selScope  = (options & HVFF_SCOPE_SELECTION) ? true : false;
	size_w  searchidx = selScope ? SelectionStart() : m_nCursorOffset;
	size_w  searchlen = selScope ? SelectionSize() : m_pDataSeq->size();

	bool    matchCase = (options & HVFF_CASE_INSENSITIVE) ? false : true;

	int		len;
	int		querycount = 0;

	if(patlen == 0)
		return FALSE;

	// search the sequence a block at a time
	while((len = (int)m_pDataSeq->render(searchidx, block, 1000, 0)) > 0)
	{
		int pos = 0;
		int partial;

		if(selScope && (searchidx < SelectionStart() || searchidx >= SelectionEnd()))
		{
			break;
		}
		
		// Boyer-moore search each block
		while((pos = SearchBlock(block, pos, len - pos, &partial, matchCase)) >= 0)
		{
			if(pos >= 0)
			{
				// found a match!
				*result = searchidx + pos;
				return TRUE;
			}
		}

		// do we have a partial match?
		if(partial >= 0)
		{
			partial = -1;
		}

		// keep the user informed as to our progress, which gives
		// them the opportunity to cancel
		if(++querycount == 1024)// * 32)
		{
			if(QueryProgressNotify(HVN_PROGRESS, searchidx, searchlen))
			{
				//seterror(HVE_USER_ABORT);
				return FALSE;
			}

			querycount = 0;
		}

		searchidx += len;
	}

	return FALSE;
}
