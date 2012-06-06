//
//  GridViewData.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef GRIDVIEW_DATA_INCLUDED
#define GRIDVIEW_DATA_INCLUDED

class GVData;
class GVRow;

//
//	Container class to hold one row of data
//
class GVRow
{
friend class GVData;
friend class GridView;

public:
	GVRow(int count = 0) : items(0), state(0), prev(0), next(0), parent(0), sibling(0), first(0), last(0)
	{
		if(count > 0)
		{
			items = new GVITEM[count];
			memset(items, 0, sizeof(GVITEM) * count);
		}
	}

	~GVRow()
	{
		//for(int i = 0; i < 
		delete[] items;
	}

	int Level()
	{
		int level = 0;
		for(GVRow *ptr = parent; ptr; ptr = ptr->parent)
			level++;

		return level > 1 ? level - 1 : 0;
	}

	bool Expanded()
	{
		//return true;
		return ((items[0].state & GVIS_EXPANDED) && HasChildren()) ? true : false;
	}

	bool HasChildren()
	{
		return first ? true : false;
		//return next && next->items && next->items[0].iIndent > items[0].iIndent;
	}

	bool HasSibling()
	{
		return next ? true : false;
		//return next && next->items && next->items[0].iIndent > items[0].iIndent;
	}

	bool IsChild(GVRow *gvptr)
	{
		while(gvptr)
		{
			if(gvptr->parent == this)
				return true;

			gvptr = gvptr->parent;
		}

		return false;
	}

	GVRow * NextVisible()
	{
		if(first && Expanded())
		{
			return first;
		}
		else if(next)
		{
			return next;
		}
		else
		{
			GVRow *gvptr = parent;
			while(gvptr)
			{
				if(gvptr->next)
					return gvptr->next;

				gvptr = gvptr->parent;
			}

			return 0;
		}
	}

	int TreeIndent()
	{
		return Level();
		if(items)
			return items[0].iIndent;
		else
			return 0;
	}

	void PrependChild(GVRow *rowptr);
	void AppendChild(GVRow *rowptr);

	void Append(GVRow *rowptr);
	void Prepend(GVRow *rowptr);

	void AdjustTreePtrs(GVITEM *gvitem);

private:

	//
	//	Item data
	//
	GVITEM	*items;
	UINT	 state;

	//
	//	Doubly-linked list to chain all items together
	//
	GVRow	*prev;		// sibling nodes
	GVRow	*next;

	GVRow	*first;		// child nodes
	GVRow	*last;

	//
	//	Linked-list to represent the tree-hierarchy
	//
	GVRow	*parent;
	GVRow	*sibling;
};

//
//	Container class to hold all the GridView data
//
class GVData
{
	friend class GridView;

public:
	GVData()
	{
/*		m_head.prev = 0;
		m_head.next = &m_tail;
		m_tail.prev = &m_head;
		m_tail.next = 0;*/

		m_nCacheIndex = 0;
		m_nItemCount  = 0;
	}

private:

	ULONG GetRowCount() const
	{
		return m_nItemCount;
	}

	ULONG VisibleRows();

	//
	//	Get the row at specified index
	//
	GVRow * GetRow(ULONG index);

	void SetColumns(int nColumns)
	{
		m_nColumnCount = nColumns;
	}
	GVRow * InsertRow(GVRow *gvroot, UINT gviWhere, GVITEM *gvitem);//, int nColumns);
	GVRow * InsertUniqueChild(GVRow *gvroot, GVITEM *gvitem);
	BOOL	SetItem(GVITEM *gvitem);
	BOOL	SetRowItem(GVRow *gvrow, GVITEM *gvitem);
	GVRow * GetRowItem(GVRow *gvrow, GVITEM *gvitem);
	GVRow * GetRowParentItem(GVRow *gvrow, GVITEM *gvitem);

	BOOL DeleteAll();
	BOOL DeleteRow(GVRow *gvrow);
	BOOL DeleteChildren(GVRow *gvrow);

	GVRow * FindItem(GVRow *gvrow, GVITEM *gvitem, int depth);

	UINT GetChildIndex(GVRow *gvrow);


	//
	//	Return the row-pointer from the specified index
	//
	/*GVRow * operator[] (ULONG index) const
	{
		return GetRow(index);
	}*/

	//GVRow * GetFirstVisibleRow();
	GVRow * GetNextRow(GVRow *rowptr);
	GVRow * GetNextVisibleRow(GVRow *rowptr);
	void    AdjustTreePtrs(GVRow *rowptr, GVITEM *gvitem);
//	GVRow * End() 
//	{ return &m_tail; }

	// head+tail linked-list sentinels
	GVRow	m_gvRoot;
	//GVRow	m_tail;

	mutable GVRow	*m_pgvRowCache;
	mutable ULONG	 m_nCacheIndex;
	mutable ULONG	 m_nItemCount;
	int				 m_nColumnCount;

	
};

#endif
