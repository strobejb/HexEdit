//
//  DlgModify.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "..\HexView\HexView.h"
#include "HexUtils.h"
#include "HexEdit.h"

static BYTE operandData[8];
static int  operandLen;
BOOL UpdateSearchData(HWND hwndSource, int searchType, BOOL fBigEndian, BYTE *searchData, int *searchLen);

void transform( BYTE * buf, size_t len, int operation, BYTE * operand, int basetype, int endian );

TCHAR * szOpList[] = 
{
	TEXT("Byte Swap (Endian Change)"),
	TEXT("Bit Flip"),
	TEXT("Shift Left"),
	TEXT("Shift Right"),
	TEXT("Add"),
	TEXT("Subtract"),
	TEXT("Multiply"),
	TEXT("Divide"),
	TEXT("Modulus"),
	TEXT("Bitwise AND"),
	TEXT("Bitwise OR"),
	TEXT("Bitwise XOR"),
	0,
};

TCHAR * szTypeList[] = 
{
	TEXT("byte"),
	TEXT("word"),
	TEXT("dword"),
	TEXT("qword"),
	TEXT("signed byte"),
	TEXT("signed word"),
	TEXT("signed dword"),
	TEXT("signed qword"),
	TEXT("float"),
	TEXT("double"),
	0
};


BOOL ModifyData(BYTE * buf, size_t len, int operation, BYTE * operand, int ty, BOOL fEndian)
{
	transform(buf, len, operation, operand, ty, fEndian);

/*	return TRUE;

	for(i = 0; i < len; i++)
	{
		switch(operation)
		{
		case 0: break;
		case 1: buf[i] = ~buf[i]; break;
		case 2: buf[i] = buf[i] >> operand; break;
		case 3: buf[i] = buf[i] << operand; break;	
		case 4: buf[i] = buf[i] + operand; break;
		case 5: buf[i] = buf[i] - operand; break;
		case 6: buf[i] = buf[i] * operand; break;
		case 7: buf[i] = buf[i] / operand; break;
		case 8: buf[i] = buf[i] & operand; break;
		case 9: buf[i] = buf[i] | operand; break;
		case 10: buf[i] = buf[i] ^ operand; break;
		}
	}
*/
	return TRUE;
}

BOOL ModifyHexViewData(HWND hwndHV, int nOperation, BYTE * operand, size_w nLength, int nType, BOOL fEndian)
{
	size_w start, offset;
	size_w remaining = nLength;
	size_w length	 = nLength;

	BYTE buf[0x100];

	HexView_GetSelStart(hwndHV, &start);
	offset = start;

	// turn off redraw so the cursor/display doesn't update whilst we are
	// writing data to the hexview
	SendMessage(hwndHV, WM_SETREDRAW, FALSE, 0);

	while(nLength > 0)
	{
		ULONG len = (ULONG)min(nLength, 0x100);

		// get data at current cursor position!
		if(HexView_GetData(hwndHV, offset, buf, len))
		{
			// do the operation!
			if(ModifyData(buf, len, nOperation, operand, nType, fEndian))
			{
				// write the data back to the hexview
				HexView_SetData(hwndHV, offset, buf, len);
			}
		}
		else
		{
			return FALSE;
		}

		offset += len;
		nLength -= len;
	}

	HexView_SetSelStart(hwndHV, start);
	HexView_SetSelEnd(hwndHV, start+length);

	SendMessage(hwndHV, WM_SETREDRAW, TRUE, 0);
	
	return TRUE;
}

INT_PTR CALLBACK ModifyDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static size_w len;
	HWND hwndHV = g_hwndHexView;

	static BOOL fHexLength = FALSE;
	static int  nLastOperand   = 0;
	static int  nLastOperation = 0;
	static BOOL fBigEndian	   = FALSE;
	int basetype;
	int searchtype;

	static const int SearchTypeFromBaseType[] = 
	{
		SEARCHTYPE_BYTE, SEARCHTYPE_WORD, SEARCHTYPE_DWORD, SEARCHTYPE_QWORD,
		SEARCHTYPE_BYTE, SEARCHTYPE_WORD, SEARCHTYPE_DWORD, SEARCHTYPE_QWORD,
		SEARCHTYPE_FLOAT, SEARCHTYPE_DOUBLE, 
	};
		
	switch (iMsg)
	{
	case WM_INITDIALOG:

		AddComboStringList(GetDlgItem(hwnd, IDC_MODIFY_DATATYPE), szTypeList, 0);

		AddComboStringList(GetDlgItem(hwnd, IDC_MODIFY_OPERATION), szOpList, nLastOperation);
		SetDlgItemBaseInt(hwnd, IDC_MODIFY_OPERAND, nLastOperand, fHexLength ? 16 : 10, FALSE);

		CheckDlgButton(hwnd, IDC_HEX, fHexLength ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, IDC_ENDIAN, fBigEndian ? BST_CHECKED : BST_UNCHECKED);

		//len = HexView_GetSelSize(hwndHV);
		//SetDlgItemBaseInt(hwnd, IDC_MODIFY_NUMBYTES, len, fHexLength ? 16 : 10, FALSE);
		

		CenterWindow(hwnd);
		return TRUE;
			
	case WM_CLOSE:
		EndDialog(hwnd, FALSE);
		return TRUE;
	
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_MODIFY_OPERATION:
		case IDC_MODIFY_OPERAND:
		case IDC_MODIFY_NUMBYTES:
			nLastOperation = (int)SendDlgItemMessage(hwnd, IDC_MODIFY_OPERATION, CB_GETCURSEL, 0, 0);
			nLastOperand   = (int)GetDlgItemBaseInt(hwnd, IDC_MODIFY_OPERAND, fHexLength ? 16 : 10);
			len            = GetDlgItemBaseInt(hwnd, IDC_MODIFY_NUMBYTES, fHexLength ? 16 : 10);
			return TRUE;

		case IDC_ENDIAN:
			fBigEndian	   = IsDlgButtonChecked(hwnd, IDC_ENDIAN);
			return TRUE;

		case IDC_HEX:
			fHexLength = IsDlgButtonChecked(hwnd, IDC_HEX);

		/*	len = HexView_GetSelSize(hwndHV);
			SetDlgItemBaseInt(hwnd, IDC_MODIFY_NUMBYTES, len, fHexLength ? 16 : 10, FALSE);
			*/
			
			SetDlgItemBaseInt(hwnd, IDC_MODIFY_OPERAND,  nLastOperand, fHexLength ? 16 : 10, FALSE);

			SendDlgItemMessage(hwnd, IDC_MODIFY_OPERAND, EM_SETSEL, 0, -1);
			SetDlgItemFocus(hwnd, IDC_MODIFY_OPERAND);
			return TRUE;

		case IDOK:
			
			// get the basetype we are using
			basetype   = (int)SendDlgItemMessage(hwnd, IDC_INSERT_DATATYPE, CB_GETCURSEL, 0, 0);
			searchtype = SearchTypeFromBaseType[basetype];

			// get the operand in raw-byte format, ensure it is always little-endian
			// as we must do these calculations using the native byte ordering format
			operandLen = sizeof(operandData);
			UpdateSearchData(GetDlgItem(hwnd, IDC_MODIFY_OPERAND), searchtype, FALSE, operandData, &operandLen);

			HexView_GetSelSize(hwndHV, &len);
			ModifyHexViewData(hwndHV, nLastOperation, operandData, len, basetype, fBigEndian);
			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		default:
			return FALSE;
		}

	case WM_HELP: 
		return HandleContextHelp(hwnd, lParam, IDD_TRANSFORM);

	default:
		break;
	}
	return FALSE;

}


BOOL ShowModifyDlg(HWND hwnd)
{
	return (BOOL)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TRANSFORM), hwnd, ModifyDlgProc);
}
