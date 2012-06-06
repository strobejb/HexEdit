//
//  ToolPanel.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef TOOLPANEL_INCLUDED
#define TOOLPANEL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define WC_TOOLPANEL TEXT("ToolPanel")
#define MAX_TOOLS 64

void ToolPanel_AddItem(HWND hwndPanel, HWND hwndCtrl, UINT uStyle);
HWND ToolPanel_Create(HWND hwndParent, WNDPROC userproc);
void ToolPanel_AddGripper(HWND hwndPanel);
void ToolPanel_AddNewLine(HWND hwndPanel, int vspace);
void ToolPanel_AddAnchor(HWND hwndPanel, int type, int height);
void ToolPanel_AutoSize(HWND hwndPanel);
void ToolPanel_AddVSpace(HWND hwndPanel, int height);

#define TIS_CONTROL			0x01
#define TIS_GRIPPER			0x02
#define TIS_LAST			0x04
#define TIS_NEWLINE			0x08
#define TIS_ANCHOR_BOTTOM	(TIS_NEWLINE|TIS_LAST)
#define TIS_FIXEDHEIGHT		0x10
#define TIS_SPACER			0x20

#define ITEM_BORDER			2


#ifdef __cplusplus
}
#endif

#endif
