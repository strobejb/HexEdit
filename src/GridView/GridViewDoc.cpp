//
//  GridViewDoc.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//



#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "GridViewInternal.h"

bool SetItemText(GVITEM *gvitem, TCHAR *szText)
{
	int len = lstrlen(szText) + 1;

	// does the text require extra space?
	if(len > gvitem->cchTextMax)
	{
		TCHAR *tmp;
		
		if((tmp = new TCHAR[len]) == 0)
			return false;

		gvitem->cchTextMax	= len;
		gvitem->pszText		= tmp;
	}

	lstrcpy(gvitem->pszText, szText);
	return true;
}

ULONG GetItemText(GVITEM *dest, GVITEM *src)
{
	lstrcpyn(dest->pszText, src->pszText, dest->cchTextMax);
	return lstrlen(src->pszText);
}

BOOL GridView::InsertColumn(int index, GVCOLUMN *gvcol)
{
	HDITEM hditem	= { HDI_TEXT | HDI_WIDTH };

	GVCOLUMN * gvdup = new GVCOLUMN;
	*gvdup = *gvcol;

	// cannot add columns when the grid already contains data
	if(m_nNumLines > 0)
		return FALSE;

	hditem.pszText	= gvcol->pszText;
	hditem.cxy		= gvcol->xWidth;
	hditem.lParam	= (LPARAM)gvdup;

	if(Header_InsertItem(m_hWndHeader, index, &hditem) != -1)
	{
		//m_gvColumn[m_nNumColumns]			= *gvcol;
		//m_gvColumn[m_nNumColumns].pszText	= 0;		// we don't own the text
		m_nNumColumns++;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

GVRow * GVData::GetRow(ULONG index) 
{
	GVRow *ptr = m_gvRoot.first;
	ULONG i = 0;

	while(i < index && ptr)
	{
		ptr = ptr->NextVisible();
		i++;
	}

	return ptr;

/*	// insert at top
	if(index == 0)
	{
		m_pgvRowCache = m_head.next;
		m_nCacheIndex = index;
	}
	// insert at bottom
	else if(index == m_nItemCount)
	{
		m_pgvRowCache = &m_tail;
		m_nCacheIndex = index;
	}
	// insert in the middle - start search from cached position
	else if(index < m_nItemCount)
	{
		// search fowards
		while(m_nCacheIndex < index)
		{
			m_pgvRowCache = m_pgvRowCache->next;
			m_nCacheIndex++;
		}
		
		// search backwards
		while(m_nCacheIndex > index)
		{
			m_pgvRowCache = m_pgvRowCache->prev;
			m_nCacheIndex--;
		}
	}
	// anywhere else is an error
	else
	{
		return 0;
	}
	
	return m_pgvRowCache;*/
}


void GVData::AdjustTreePtrs(GVRow *gvrow, GVITEM *gvitem)
{
/*	if(gvrow->prev == &m_head)
	{
		gvrow->parent  = 0;
		gvrow->sibling = 0;
	}

	//
	//	Find the parent node
	//

	// sibling of the previous (adjacent) node
	if(gvrow->TreeIndent() == gvrow->prev->TreeIndent())
	{
		gvrow->parent = gvrow->prev->parent;
		gvrow->prev->sibling = gvrow;
	}
	// child of previous node
	else if(gvrow->TreeIndent() == gvrow->prev->TreeIndent() + 1)
	{
		gvrow->parent = gvrow->prev;
	}
	// not related
	else if(gvrow->TreeIndent() < gvrow->prev->TreeIndent())
	{
		GVRow *rowptr = gvrow->prev;
		while(rowptr)
		{
			if(gvrow->TreeIndent() == rowptr->TreeIndent())
			{
				gvrow->parent = rowptr->parent;
				rowptr->sibling = gvrow;
				break;
			}
			else if(gvrow->TreeIndent() < rowptr->TreeIndent())
			{
				rowptr->sibling = gvrow;
			}

			rowptr = rowptr->parent;
		}
	}*/

	/*if(gvrow->next != m_tail && gvrow->parent)
	{
		/if(gvrow->TreeIndent() > gvrow->parent->TreeIndent())
			gvrow->parent->
	}*/
}

void GVRow::PrependChild(GVRow *rowptr)
{
	if(first == 0)
	{
		first = rowptr;
		last  = rowptr;
	}
	else
	{
		first->prev = rowptr;
		first = rowptr;
	}

	rowptr->prev  = 0;
	rowptr->next  = first;
	rowptr->first = 0;
	rowptr->last  = 0;
}

void GVRow::AppendChild(GVRow *rowptr)
{
	rowptr->next  = 0;
	rowptr->prev = last;

	if(last == 0)
	{
		first = rowptr;
		last  = rowptr;
	}
	else
	{
		last->next	= rowptr;
		last		= rowptr;
	}

	rowptr->first = 0;
	rowptr->last  = 0;

	rowptr->parent = this;
}

void GVRow::Prepend(GVRow *rowptr)
{
	rowptr->next   = this;
	rowptr->prev   = prev;
	rowptr->parent = parent;
	rowptr->first  = 0;
	rowptr->last   = 0;

	if(prev)
		prev->next = rowptr;

	prev = rowptr;

	if(parent->first == this)
		parent->first = rowptr;
}

void GVRow::Append(GVRow *rowptr)
{
	rowptr->next   = next;
	rowptr->prev   = this;
	rowptr->parent = parent;
	rowptr->first  = 0;
	rowptr->last   = 0;

	if(next)
		next->prev = rowptr;

	next = rowptr;

	if(parent->last == this)
		parent->last = rowptr;
}

BOOL GVData::SetRowItem(GVRow *rowptr, GVITEM *gvitem)
{
	if(gvitem == 0)
		return FALSE;

	if(rowptr == 0 && (rowptr = GetRow(gvitem->iItem)) == 0)
		return FALSE;

	if(gvitem->iSubItem >= m_nColumnCount)
		return FALSE;

	GVITEM *dest = &rowptr->items[gvitem->iSubItem];

	// set the text if specified
	if(gvitem->mask & GVIF_TEXT)
	{
		if(!SetItemText(dest, gvitem->pszText))
			return FALSE;
	}

	if(gvitem->mask & GVIF_PARAM)
		dest->param = gvitem->param;

	if(gvitem->mask & GVIF_STATE)
		dest->state = gvitem->state;

	if(gvitem->mask & GVIF_IMAGE)
		dest->iImage = gvitem->iImage;

	if(gvitem->mask & GVIF_INDENT)
		dest->iIndent = gvitem->iIndent;

	if(gvitem->mask & GVIF_FONT)
		dest->iFont = gvitem->iFont;

	if(gvitem->mask & GVIF_COLOR)
		dest->color = gvitem->color;

	dest->mask |= gvitem->mask;

	return TRUE;
}

GVRow * GVData::GetRowItem(GVRow *gvrow, GVITEM *gvitem)
{
	if(gvitem == 0)
		return 0;

	if(gvrow == 0)
		gvrow = GetRow(gvitem->iItem);

	if(gvrow == 0 || gvrow->items == 0)
		return 0;

	GVITEM d = { 0};
	GVITEM *src = &d;

	if(gvitem->iSubItem < m_nColumnCount)
		src = &gvrow->items[gvitem->iSubItem];

	if(gvitem->mask & GVIF_TEXT)
		GetItemText(gvitem, src);

	if(gvitem->mask & GVIF_PARAM)
		gvitem->param = src->param;

	if(gvitem->mask & GVIF_STATE)
		gvitem->state = src->state;

	if(gvitem->mask & GVIF_IMAGE)
		gvitem->iImage = src->iImage;

	if(gvitem->mask & GVIF_INDENT)
		gvitem->iIndent = src->iIndent;

	if(gvitem->mask & GVIF_FONT)
		gvitem->iFont = src->iFont;

	if(gvitem->mask & GVIF_COLOR)
		gvitem->color = src->color;

	return gvrow;
}

GVRow * GVData::GetRowParentItem(GVRow *gvrow, GVITEM *gvitem)
{
	if(gvitem == 0 || gvrow == 0)
		return 0;

	if(gvrow->parent == 0)
		return 0;

	return GetRowItem(gvrow->parent, gvitem);
}

GVRow * GVData::InsertUniqueChild(GVRow *baseItem, GVITEM *gvitem)
{
	GVRow *gvrow;

	// working at root?
	if(baseItem == NULL)
		baseItem = &m_gvRoot;

	GVITEM gvfind = *gvitem;
	
	if(gvitem->mask & GVIF_TEXT)
		gvfind.mask = GVIF_TEXT;
	else if(gvitem->mask & GVIF_PARAM)
		gvfind.mask = GVIF_PARAM;
	
	// find the child!
	if((gvrow = FindItem(baseItem, &gvfind, 1)) == 0)
	{
		// if it doesn't exist then create a new child
		return InsertRow(baseItem, GVI_CHILD, gvitem);
	}
	else
	{
		// otherwise return the existing node
		return gvrow;
	}
}

GVRow * GVData::InsertRow(GVRow *baseItem, UINT gviWhere, GVITEM *gvitem)//, int nColumns)
{
	GVRow * gvnew;
	
	// get the row after the index
//	if((gvrow = GetRow(gvitem->iItem)) == 0)
//		return 0;

	// create the new tree-node
	gvnew				= new GVRow(m_nColumnCount);

	// insert at root
	if(baseItem == NULL)
	{
		baseItem = &m_gvRoot;
		gviWhere = GVI_CHILD;
	}
	
	if(gviWhere == GVI_CHILD)
	{
		baseItem->AppendChild(gvnew);
	}
	else if(gviWhere == GVI_AFTER)
	{
		baseItem->Append(gvnew);
		//baseItem->parent->AppendChild(gvnew);
	}
	else if(gviWhere == GVI_BEFORE)
	{
		baseItem->Prepend(gvnew);
	}
	//else if(gviWhere == 
	

	/*gvnew->next			= gvrow;
	gvnew->prev			= gvrow->prev;
	gvrow->prev->next	= gvnew;
	gvrow->prev			= gvnew;*/
	
	// todo: needs to have 0 col-index?
	gvnew->items[0] = *gvitem;

	if(gvitem->pszText)
	{
		gvnew->items[0].pszText = _tcsdup(gvitem->pszText);
	}

	//AdjustTreePtrs(gvnew, gvitem);

	m_nItemCount++;

	return gvnew;
}

BOOL GVData::SetItem(GVITEM *gvitem)
{
	GVRow * gvrow = 0;
	
/*	if((gvrow = GetRow(gvitem->iItem)) == 0)
		return FALSE;*/

	gvrow->items[gvitem->iSubItem]			= *gvitem;
	
	//SetItemText(gvitem->pszText)
	//gvrow->items[gvitem->iSubItem].pszText	= ;

	return TRUE;
}

//
//	Find child-node of specified row
//
GVRow * GVData::FindItem(GVRow *gvrow, GVITEM *gvitem, int depth)
{
	GVRow *child;

	if(gvitem == 0 || depth == 0)
		return 0;

	if(gvrow == NULL)
		gvrow = &m_gvRoot;

	for(gvrow = gvrow->first; gvrow; gvrow = gvrow->next)
	{
		GVITEM *g0 = &gvrow->items[0];
		GVITEM *g2 = &gvrow->items[2];

		if(gvrow->items)
		{
			if(gvitem->mask & GVIF_PARAM)
			{
				if(gvrow->items[gvitem->iSubItem].param == gvitem->param)
					return gvrow;
			}
			else if((gvitem->mask & GVIF_TEXT) && gvitem->pszText)
			{
				if(lstrcmpi(gvrow->items[gvitem->iSubItem].pszText, gvitem->pszText) == 0)
					return gvrow;
			}
		}

		if(depth != -1)
			depth--;

		if(gvrow->first && (child = FindItem(gvrow, gvitem, depth)) != 0)
			return child;
	}

	return 0;
}

GVRow * GridView::FindChild(GVRow *gvrow, GVITEM *gvitem, int depth)
{
	//if(gvrow) gvrow = gvrow->first;
	return m_gvData.FindItem(gvrow, gvitem, depth);
}

BOOL GVData::DeleteRow(GVRow *gvrow)
{
	if(gvrow == 0)
		return FALSE;

	// delete from sibling
	if(gvrow->prev)
		gvrow->prev->next = gvrow->next;

	if(gvrow->next)
		gvrow->next->prev = gvrow->prev;

	GVRow *parent = gvrow->parent;

	if(gvrow == parent->first)
		parent->first = parent->first->next;

	if(gvrow == parent->last)
		parent->last = parent->last->prev;

	DeleteChildren(gvrow);
	delete gvrow;
	return TRUE;
}

BOOL GVData::DeleteChildren(GVRow *gvrow)
{
	if(gvrow == 0)
		return FALSE;

	GVRow *ptr, *tmp;
	for(ptr = gvrow->first; ptr; ptr = tmp)
	{
		tmp = ptr->next;
		DeleteRow(ptr);
	}

	return TRUE;
}

BOOL GridView::DeleteRow(GVRow *gvrow)
{
	m_gvData.DeleteRow(gvrow);
	return TRUE;
}

BOOL GridView::DeleteColumn(int index)
{
	if(Header_DeleteItem(m_hWndHeader, index))
	{
		m_nNumColumns--;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

HGRIDITEM GridView::InsertItem(HGRIDITEM hItem, UINT gviWhere, GVITEM *gvitem)
{
	GVRow *gvrow = (GVRow *)hItem;

	m_gvData.SetColumns(m_nNumColumns);

	if(m_nNumColumns > 0 && (gvrow = m_gvData.InsertRow(gvrow, gviWhere, gvitem)) != 0)
	{
		m_nNumLines++;
		
		return gvrow;
	}
	else
	{
		return 0;
	}
}

HGRIDITEM GridView::InsertUniqueChild(HGRIDITEM hItem, GVITEM *gvitem)
{
	GVRow *gvrow = (GVRow *)hItem;
	m_gvData.SetColumns(m_nNumColumns);

	if(m_nNumColumns > 0 && (gvrow = m_gvData.InsertUniqueChild(gvrow, gvitem)) != 0)
		return gvrow;
	else
		return 0;
}
	
BOOL GridView::DeleteItem(GVITEM *gvitem)
{
	return TRUE;
}

GVRow * GridView::GetFirstVisibleRow()
{
//	return m_gvData.GetRow(m_nVScrollPos);
	return m_gvData.m_gvRoot.first;
}

ULONG GVData::VisibleRows()
{
	ULONG count = 0;

	GVRow *ptr = m_gvRoot.first;

	while(ptr)
	{
		ptr = ptr->NextVisible();
		count++;
	}

	return count;
}

GVRow * GridView::GetRowItem(ULONG nLine, ULONG nColumn, GVITEM **gvitem)
{
	GVRow *gvrow;
	
	if((gvrow = m_gvData.GetRow(nLine)) == 0)
		return 0;

	if(gvitem)
	{
		//LONG idx = Header_OrderToIndex(m_hWndHeader, nColumn);
		*gvitem = &gvrow->items[nColumn];
	}

	return gvrow;
}

GVRow *	GridView::GetCurRow()
{
	return GetRowItem(m_nCurrentLine, 0, 0);
}

GVCOLUMN * GridView::GetColumn(ULONG nColumn, int *nHeaderIndex)
{
	//ULONG idx = Header_IndexToOrder(m_hWndHeader, nColumn);
	HDITEM hditem = { HDI_LPARAM }; 
	Header_GetItem(m_hWndHeader, nColumn, &hditem);

//	if(nHeaderIndex)
	//	*nHeaderIndex = idx;

	return (GVCOLUMN *)hditem.lParam;
}


/*GVRow *	GridView::GetCurrentRow()
{
	return m_gvData[m_nVScrollPos];
}*/
	
/*GVRow * GVData::GetNextRow(GVRow *rowptr)
{
	if(rowptr)
		return rowptr->next;
	else
		return rowptr;
}

bool GVData::IsExpanded(GVRow *rowptr)
{
	return rowptr->items[0].state & GVIS_EXPANDED ? true : false;
}

GVRow * GVData::GetNextVisibleRow(GVRow *rowptr)
{
	if(rowptr)
	{
		if(rowptr->Expanded())
			return rowptr->next;
		else
			return rowptr->sibling;
	}
	else
	{
		return rowptr;
	}
}

GVRow * GridView::GetRow(int idx)
{
	return m_gvData[idx];
}*/

BOOL GVData::DeleteAll()
{
	m_gvRoot.first = 0;
	m_gvRoot.parent = 0;
	m_gvRoot.last = 0;
	m_gvRoot.next = 0;
	m_gvRoot.prev = 0;
	m_nItemCount = 0;
	m_pgvRowCache = 0;
	return TRUE;
}

UINT GVData::GetChildIndex(GVRow *child)
{
	if(child == 0 || child->parent == 0)
		return -1;

	GVRow *gvrow;
	GVRow *parent = child->parent;
	UINT idx = 0;
	
	for(gvrow = parent->first; gvrow; gvrow = gvrow->next)
	{
		if(child == gvrow)
			return idx;		

		idx++;
	}

	return -1;
}