//
//  sequence.h
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#ifndef SEQUENCE_INCLUDED
#define SEQUENCE_INCLUDED

#include <vector>

#include "seqbase.h"

const size_w MAX_SEQUENCE_LENGTH = ((size_w)(-1) / sizeof(seqchar));

const size_w MEM_BLOCK_SIZE = 0x40000;	// 256kb view size

struct seqchar_info
{
	size_t	buffer;
	size_t	userdata;
};
/*
class seqfile
{
	seqfile();
	virtual ~seqfile();

	virtual open(LPCTSTR szFileName) = 0;
	virtual save() = 0;
	virtual read(size_w ) = 0;
};*/

//
//	sequence class!
//
class sequence
{
public:
	// forward declare the nested helper-classes
	class			span;
	class			span_range;
	class			buffer_control;
	class			iterator;
	class			ref;
	struct			span_desc;
	struct			span_desc_list;
	enum			action;

public:

	// sequence construction
	sequence();
	~sequence();

	long addref()
	{
		return InterlockedIncrement(&refCount);
	}

	long release()
	{
		long count = InterlockedDecrement(&refCount);
		
		if(count == 0)
			delete this;

		return count;
	}

	//
	// initialize with a file
	//
	bool		init();
	bool		open(const TCHAR *filename, bool readonly = false, bool quickload = false);
	////bool		link(const TCHAR *filename);
	size_w		replace_file(const TCHAR *filename, size_w index, bool linkto);
	size_w		insert_file(const TCHAR *filename, size_w index, bool linkto);

	//bool		link(const TCHAR *filename);
	bool		save(const TCHAR * filename = 0);
	bool		clear();
	bool		isreadonly();

	//
	// initialize from an in-memory buffer
	//
	bool		init(const seqchar *buffer, size_t length, bool duplicate_buf = false);

	//
	//	sequence statistics
	//
	size_w		size() const;
	
	//
	// sequence manipulation 
	//
	bool		insert (size_w index, const seqchar *buf, size_w length);
	bool		insert (size_w index, const seqchar  val, size_w count);
	bool		insert (size_w index, const seqchar  val);
	bool		replace(size_w index, const seqchar *buf, size_w length, size_w erase_length);
	bool		replace(size_w index, const seqchar *buf, size_w length);
	bool		replace(size_w index, const seqchar  val, size_w count);
	bool		replace(size_w index, const seqchar  val);
	bool		erase  (size_w index, size_w len);
	bool		erase  (size_w index);
	bool		append (const seqchar *buf, size_w len);
	bool		append (const seqchar val);
	void		breakopt();

	//
	// snapshot
	//
	bool		takesnapshot(size_w index, size_w length, span_desc *desclist, size_t *desclen);
	bool		injectsnapshot(size_w index, span_desc *desclist, size_t desclen);
	bool		rendersnapshot(size_t count, span_desc *desclist, size_w offset, seqchar *buffer, size_t length);
	bool		insert_snapshot(size_w index, size_w length, span_desc *desclist, size_t desclen);
	bool		replace_snapshot(size_w index, size_w length, span_desc *desclist, size_t desclen);


	//
	// undo/redo support
	//
	bool		undo();
	bool		redo();
	bool		canundo() const;
	bool		canredo() const;
	void		group();
	void		ungroup();
	void		clear_undo();
	size_w		event_index() const  { return undoredo_index; }
	size_w		event_length() const { return undoredo_length; }
	size_w		event_datalength() const { return undoredo_datalength; }

	// print out the sequence
	void		debug1();
	void		debug2();

	//
	// access and iteration
	//
	size_t		render(size_w index, seqchar *buf, size_t len, seqchar_info *infobuf = 0) const;
	seqchar		peek(size_w index) const;
	bool		poke(size_w index, seqchar val);
	seqchar&    getlastmodref(size_w *offset = 0);

	seqchar		operator[] (size_w index) const;
	ref			operator[] (size_w index);

	HANDLE		_handle();
	size_t		origfileid() { return origfile_id; }

private:

	long			refCount;

	typedef			std::vector<span_range*>	  eventstack;
	typedef			std::vector<buffer_control*>  bufferlist;
	template <class type> void clear_vector(type &source);

	//
	//	Span-table management
	//
	void			deletefromsequence(span **sptr);
	span		*	spanfromindex(size_w index, size_w *spanindex) const;
	void			scan(span *sptr);
	seqchar		*	getptr(span *sptr);


	size_w			sequence_length;
	span		*	head;
	span		*	tail;	
	span		*	frag1;
	span		*	frag2;

	
	//
	//	Undo and redo stacks
	//
	span_range *	initundo(size_w index, size_w length, action act);
	void			restore_spanrange(span_range *range, bool undo_or_redo);
	void			swap_spanrange(span_range *src, span_range *dest);
	bool			undoredo(eventstack &source, eventstack &dest);
	void			clearstack(eventstack &source);
	span_range *	stackback(eventstack &source, size_t idx);

	eventstack		undostack;
	eventstack		redostack;
	size_t			group_id;
	size_t			group_refcount;
	size_w			undoredo_index;
	size_w			undoredo_length;
	size_w			undoredo_datalength;

	//
	//	File and memory buffer management
	//
	span *			loadspan(const TCHAR *filename, bool readonly = false, bool keepondisk = false);
	//span *			linkspan(const TCHAR *filename, bool readonly);

	buffer_control *alloc_buffer(size_t size);
	buffer_control *alloc_modifybuffer(size_t size);
	bool			import_buffer(const seqchar *buf, size_t len, size_w *buffer_offset);
	bool			saveto(HANDLE hFile, bool quicksave);
	bool			writespan(HANDLE hFile, size_w index, span *sptr);


	bufferlist		buffer_list;
	size_t			modifybuffer_id;
	size_t			modifybuffer_pos;
	size_t			origfile_id;
	TCHAR			origfile_name[MAX_PATH];

	//
	//	Sequence manipulation
	//
	bool			insert_worker (size_w index, const seqchar *buf, size_w len, action act);
	bool			erase_worker  (size_w index, size_w len, action act);
	bool			can_optimize  (action act, size_w index);
	void			record_action (action act, size_w index);

	size_w			lastaction_index;
	action			lastaction;
	bool			can_quicksave;
	
	void			LOCK();
	void			UNLOCK();


};


//
//	sequence::action
//
//	enumeration of the type of 'edit actions' our sequence supports.
//	only important when we try to 'optimize' repeated operations on the
//	sequence by coallescing them into a single span.
//
enum sequence::action
{ 
	action_invalid, 
	action_insert, 
	action_erase, 
	action_replace 
};

//
//	sequence::span
//
//	private class to the sequence
//
class sequence::span
{
	friend class sequence;
	friend class span_range;
	
public:
	// constructor
	span(size_w off, size_w len, size_t buf, span *nx = 0, span *pr = 0) 
			:
			offset(off), 
			length(len), 
			buffer(buf),
			next(nx), 
			prev(pr)
	  {
		  static int count=-2;
		  id = count++;
	  }

	  
private:

	span   *next;
	span   *prev;	// double-link-list 
	
	size_w  offset;
	size_w  length;
	size_t  buffer;

	size_t	id;
};	
	


//
//	sequence::span_range
//
//	private class to the sequence. Used to represent a contiguous range of spans.
//	used by the undo/redo stacks to store state. A span-range effectively represents
//	the range of spans affected by an event (operation) on the sequence
//  
//
class sequence::span_range
{
	friend class sequence;

public:

	// constructor
	span_range(	size_w	seqlen = 0, 
				size_w	idx    = 0, 
				size_w	len    = 0, 
				action	a      = action_invalid,
				bool	qs     = false, 
				size_t	id     = 0
			) 
		: 
		first(0), 
		last(0), 
		boundary(true), 
		sequence_length(seqlen), 	
		index(idx),
		length(len),
		act(a),
		quicksave(qs),
		group_id(id)
	{
	}
		
	// destructor does nothing - because sometimes we don't want
	// to free the contents when the span_range is deleted. e.g. when
	// the span_range is just a temporary helper object. The contents
	// must be deleted manually with span_range::free
	~span_range()
	{
	}

	// separate 'destruction' used when appropriate
	void free()
	{
		span *sptr, *next, *term;
		
		if(boundary == false)
		{
			// delete the range of spans
			for(sptr = first, term = last->next; sptr && sptr != term; sptr = next)
			{
				next = sptr->next;
				delete sptr;
			}
		}
	}

	// add a span into the range
	void append(span *sptr)
	{
		if(sptr != 0)
		{
			// first time a span has been added?
			if(first == 0)
			{
				first = sptr;
			}
			// otherwise chain the spans together.
			else
			{
				last->next = sptr;
				sptr->prev = last;
			}
			
			last     = sptr;
			boundary = false;
		}
	}

	// join two span-ranges together
	void append(span_range *range)
	{
		if(range->boundary == false)
		{	
			if(boundary)
			{
				first       = range->first;
				last        = range->last;
				boundary    = false;
			}
			else
			{
				range->first->prev = last;
				last->next  = range->first;
				last		= range->last;
			}
		}
	}

	// join two span-ranges together. used only for 'back-delete'
	void prepend(span_range *range)
	{
		if(range->boundary == false)
		{
			if(boundary)
			{
				first       = range->first;
				last        = range->last;
				boundary    = false;
			}
			else
			{
				range->last->next = first;
				first->prev	= range->last;
				first		= range->first;
			}
		}
	}
	
	// An 'empty' range is represented by storing pointers to the
	// spans ***either side*** of the span-boundary position. Input is
	// always the span following the boundary.
	void spanboundary(span *before, span *after)
	{
		first    = before;
		last     = after;
		boundary = true;
	}

	
private:
	
	// the span range
	span	*first;
	span	*last;
	bool	 boundary;

	// sequence state
	size_w	 sequence_length;
	size_w	 index;
	size_w	 length;
	action	 act;
	bool	 quicksave;
	size_t	 group_id;
};

//
//	sequence::ref
//
//	temporary 'reference' to the sequence, used for
//  non-const array access with sequence::operator[]
//
class sequence::ref
{
public:
	ref(sequence *s, size_w i) 
		:  
		seq(s),  
		index(i) 
	{
	}

	operator seqchar() const		
	{ 
		return seq->peek(index);	          
	}
	
	ref & operator= (seqchar rhs)	
	{ 
		seq->poke(index, rhs); 
		return *this;	
	}

private:
	size_w		index;
	sequence *	seq;
};

//
//	buffer_control
//
class sequence::buffer_control
{
	friend class sequence;
#define MAX_VIEWS 4
	
	class buffer_view
	{
		friend class buffer_control;
		friend class sequence;
	public:

		buffer_view() : initialized(false), buffer(0)
		{
		};

	private:
		seqchar	   *buffer;
		size_w		offset;
		size_w		length;
		bool		initialized;
	};

public:
	buffer_control() : hFile(0), readonly(false)
	{
	}

	~buffer_control() 
	{ 
		free(); 
	}

	bool		init(const seqchar * buf, size_w len, bool copybuf);
	bool		init(size_t max);
	bool		append(const seqchar * buf, size_t len);
	bool		load(const TCHAR * filename, bool reado, bool quickload);
	void		free();

	seqchar *	getptr(size_w off, size_w len);

private:
	buffer_view		viewlist[MAX_VIEWS];
	//seqchar		*	buffer;
	size_w		length;
	size_w		maxsize;
	HANDLE		hFile;
	size_t		id;
	bool		ownbuf;
	bool		readonly;
};

class sequence::iterator
{
public:

};


struct sequence::span_desc
{
	size_w  offset;
	size_w  length;
	size_t	buffer;
};


#endif