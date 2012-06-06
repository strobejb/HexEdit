//
//  ResizeWnd.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>

HDWP RightJustifyWindow(HWND hwndCtrl, HWND hwndParent, int margin, HDWP hdwp)
{
	RECT ctrl, parent;
	GetWindowRect(hwndCtrl, &ctrl);
	ctrl.right -= ctrl.left;
	ctrl.bottom -= ctrl.top;
	ctrl.left = 0;
	ctrl.top = 0;

	//make the ctrl rect relative to the parent
	MapWindowPoints(hwndCtrl, hwndParent, (POINT *)&ctrl, 2);

	GetClientRect(hwndParent, &parent);

	//MoveWindow(hwndCtrl, parent.right - (ctrl.right-ctrl.left) - margin, ctrl.top, ctrl.right-ctrl.left, ctrl.bottom-ctrl.top, TRUE);

	if(hdwp)
	{
		return DeferWindowPos(hdwp, hwndCtrl, 0, 
				parent.right- (ctrl.right-ctrl.left) - margin, ctrl.top, 
				ctrl.right-ctrl.left, ctrl.bottom-ctrl.top, SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hwndCtrl, 0, parent.right- (ctrl.right-ctrl.left) - margin, ctrl.top, 
				ctrl.right-ctrl.left, ctrl.bottom-ctrl.top, SWP_NOZORDER);
		return 0;
	}
}

HDWP StretchWindowToRight(HWND hwndCtrl, HWND hwndParent, int margin, HDWP hdwp)
{
	RECT ctrl, parent;

	GetWindowRect(hwndCtrl, &ctrl);
	ctrl.right -= ctrl.left;
	ctrl.bottom -= ctrl.top;
	ctrl.left = 0;
	ctrl.top = 0;

	//make the ctrl rect relative to the parent
	MapWindowPoints(hwndCtrl, hwndParent, (POINT *)&ctrl, 2);

	GetClientRect(hwndParent, &parent);

	//MoveWindow(hwndCtrl, ctrl.left, ctrl.top, parent.right - (ctrl.left + margin), ctrl.bottom- ctrl.top, TRUE);
	
	return DeferWindowPos(hdwp, hwndCtrl, 0, 
		ctrl.left, ctrl.top, 
		parent.right - (ctrl.left + margin), ctrl.bottom- ctrl.top, 
		SWP_NOZORDER);

}
