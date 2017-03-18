/* docitem.h: interface for the DocItem class and it's subclasses and
//            related classes.

Copyright (C) 1997-2013, Brian Bray

*/

/* Needs:
#include <list>
#include <map>
#include <fstream>
#include "bw/string.h"
#include "bw/countable.h"
*/


// Ignore warning about the expanded template names being longer than 256 characters (MSVC)
//#pragma warning( disable : 4786 )

// Since the constructors look like alternative ways to make a String print
inline std::ostream& operator<<( std::ostream& os, const bw::String& s)
{
	os << (const char*)s;
	return os;
}

// Forward definitions
class Project;
class DocClass;
class Member;
class Function;
class Variable;
class AttribIterator;

class Attribute {
public:
	Attribute() {};
	Attribute( const bw::String& sKeyword, const bw::String& sValue )
		: m_sKeyword(sKeyword), m_sValue(sValue) {}

	bw::String keyword() const {
		return m_sKeyword;
	}
	bw::String value() const {
		return m_sValue;
	}

	// Necessary operators for <list>
	bool operator==( const Attribute& attr ) const {
		return this==&attr;
	}
	bool operator!=( const Attribute& attr ) const {
		return this!=&attr;
	}
	bool operator<( const Attribute& attr ) const {
		return this<&attr;
	}
	bool operator>( const Attribute& attr ) const {
		return this>&attr;
	}

private:
	bw::String		m_sKeyword;
	bw::String		m_sValue;
};

class DocItem {
public:
	friend class AttribIterator;

	DocItem();
	virtual ~DocItem();

public:		// Countable object
	unsigned long AddRef();
	unsigned long Release();

public:		// Attributes
	virtual void clearAttributes();
	virtual void addAttribute( const bw::String& sKeyword, const bw::String& sValue );
	virtual void addAttribute( const Attribute& attr );
	virtual AttribIterator find( const bw::String& sKeyword ) const;
	virtual AttribIterator findAll() const;

public:		// Common routines
	friend std::ostream& operator<<( std::ostream& ost, const DocItem& di );
	virtual bool needPrototype() const =0;
	virtual void setPrototype( const bw::String& );
	virtual void setDefaultPrototype();
	virtual void setLinkName( const bw::String& );
	virtual void setDefaultLinkName();
	virtual void setImpliedAttribute( const bw::String& sV );
	virtual void setDefaultImpliedAttribute();
	virtual bw::String getName() const {
		return m_sItemName;
	}
	virtual bw::String getFullDisplayName() const = 0;
	virtual bw::String getDisplayName() const = 0;
	virtual bw::String getTitle() const;
	virtual bw::String getLinkName() const {
		return m_sLinkName;
	}

public:		// Comparisons so that these objects can be put in sets, etc.
//	static bool isLess( const DocItem& diA, const DocItem& diB)
//		{return diA.getName()<diB.getName();}

protected:
	bw::String m_sLinkName;
	bw::String m_sItemName;			// Set by subclasses

private:
	unsigned long	m_cRef;

	typedef std::list< Attribute > Attribs;
	Attribs m_attribs;

};

class AttribIterator {
public:		// Attributes
	AttribIterator();
	AttribIterator( const DocItem::Attribs::const_iterator&, const DocItem::Attribs::const_iterator&, const bw::String& );
	AttribIterator( const DocItem::Attribs::const_iterator&, const DocItem::Attribs::const_iterator& );

	AttribIterator& operator++() {
		++m_indx;
		scan();
		return *this;
	}
	void operator++(int) {
		++m_indx;
		scan();
	}
	const Attribute* operator->() const {
		return &*m_indx;
	}
	const Attribute& operator*() const {
		return *m_indx;
	}
	bool atEof() const {
		return m_indx==m_iEnd;
	}

private:	// Attributes
	void scan();

	bool						m_isAll;
	bw::String					m_sKey;
	DocItem::Attribs::const_iterator	m_indx;
	DocItem::Attribs::const_iterator	m_iEnd;
};

class Member : public DocItem {
public:
	Member( const bw::String& sClassName, const bw::String& sName )
		: m_sClassName( sClassName ) {
		m_sItemName = sName;
	}
	virtual ~Member()
	{}

public:		// Inherited virtual functions implemented here
	virtual bool needPrototype() const {
		return find("Prototype").atEof();
	}

protected:
	bw::String		m_sClassName;
};

class Function : public Member {
public:
	Function( const bw::String& sClassName, const bw::String& sName )
		: Member( sClassName, sName ) {}
	virtual ~Function()
	{}

	virtual bw::String getFullDisplayName() const {
		return m_sClassName + "::" + m_sItemName + "()";
	}
	virtual bw::String getDisplayName() const {
		return m_sItemName + "()";
	}
};

class Variable : public Member {
public:
	Variable( const bw::String& sClassName, const bw::String& sName )
		: Member( sClassName, sName ) {}
	virtual ~Variable()
	{}

	virtual bw::String getFullDisplayName() const {
		return m_sClassName + "::" + m_sItemName;
	}
	virtual bw::String getDisplayName() const {
		return m_sItemName;
	}
};

class DocClass : public DocItem {
public:
	DocClass( const bw::String& sName ) {
		m_sItemName = sName;
	}
	virtual ~DocClass()
	{}

public:		// Inherited virtual functions implemented here
	virtual bool needPrototype() const {
		return false;
	}
	virtual bw::String getFullDisplayName() const {
		return m_sItemName;
	}
	virtual bw::String getDisplayName() const {
		return m_sItemName;
	}

public:		// Accessed by parsers
	Function* getFunction( const bw::String& sName );
	Variable* getVariable( const bw::String& sName );

public:		// Output routines
	friend std::ostream& operator<<( std::ostream& ost, const DocClass& dclass );
	bw::String getFileName() const {
		return getName()+".html";
	}

private:
	typedef std::map< bw::String, cptr<Function>, std::less<bw::String> >	FunctionMap;
	typedef std::map< bw::String, cptr<Variable>, std::less<bw::String> >	VariableMap;

	FunctionMap		m_mapFunctions;
	VariableMap		m_mapVariables;
};

class Project : public DocItem {
public:
	Project()
	{}
	virtual ~Project()
	{}

public:		// Inherited virtual functions implemented here
	virtual bool needPrototype() const {
		return false;
	}
	virtual bw::String getFullDisplayName() const {
		return "Project " + m_sItemName;
	}
	virtual bw::String getDisplayName() const {
		return m_sItemName;
	}

public:		// Accessed by parsers
	void setName( const bw::String& sName ) {
		m_sItemName = sName;
	}
	DocClass* getClass( const bw::String& sClass );
	Function* getFunction( const bw::String& sClass, const bw::String& sName );
	Variable* getVariable( const bw::String& sClass, const bw::String& sName );

public:		// Output routines
	friend std::ostream& operator<<( std::ostream& ost, const Project& proj );
	void filesOut( const bw::String& sDir );
	bw::String getFileName() const;

private:
	typedef std::map< bw::String, cptr<DocClass>, std::less<bw::String> >	ClassMap;
	ClassMap	m_mapClasses;
};

