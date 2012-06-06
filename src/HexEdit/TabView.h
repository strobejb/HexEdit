//
//  TabView.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef TABVIEW_INCLUDED
#define TABVIEW_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define WC_TABVIEW	_T("TabView")

ATOM WINAPI RegisterTabView();

typedef struct
{
	TCHAR		* pszTipText;

} TCITEMEXTRA;

typedef struct _TCITEMEX
{
	TCITEMHEADER tcitem;
	TCITEMEXTRA  tcxtra;

} TCITEMEX, *PTCITEMEX;

// define our own custom TabCtrl messages
#define TCN_CLOSING		(TCN_FIRST - 10)
#define TCN_CLOSE		(TCN_FIRST - 11)
#define TCN_MOUSELEAVE  (TCN_FIRST - 12)
#define TCN_DROPDOWN	(TCN_FIRST - 13)

// custom tab styles
#define TCIS_CUSTOMBASE		(0)
#define TCIS_NOCLOSE		(TCIS_CUSTOMBASE + 0)
#define TCIS_DISABLECLOSE	(TCIS_CUSTOMBASE + 0x1)
#define TCIS_FILENAME		(TCIS_CUSTOMBASE + 0x2)


#ifdef __cplusplus
}
#endif

#endif