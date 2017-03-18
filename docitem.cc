/* docitem.cc: implementation of the DocItem class.

Copyright (C) 1997-2013, Brian Bray

*/

#include <fstream>
#include <list>
#include <map>

#include "bw/bwassert.h"
#include "bw/string.h"
#include "bw/countable.h"
#include "lexstream.h"
#include "docitem.h"
#include "docgen.h"

using bw::String;

/*: DocItem::DocItem()				Constructor		*/
DocItem::DocItem()
	:	m_cRef( 1 )
{

}

/*: DocItem::~DocItem()				Destructor		*/
DocItem::~DocItem()
{

}

/*: DocItem::AddRef()				Instance counting	*/
unsigned long DocItem::AddRef()
{
	return ++m_cRef;
}

/*: DocItem::Release()				Instance counting	*/
unsigned long DocItem::Release()
{
	if (--m_cRef)
		return m_cRef;

	delete this;
	return 0;
}


/*: DocItem::clearAttributes		Initializer		*/
void DocItem::clearAttributes()
{
	m_attribs.clear();
}

/*: DocItem::addAttribute

	Adds another Keyword attribute for the DocItem.

	Prototype: void addAttribute( const String& sKeyword, const String& sValue );
	Prototype: void addAttribute( const Attribute& attr );
*/
void DocItem::addAttribute( const String& sKeyword, const String& sValue )
{
	m_attribs.push_back( Attribute(sKeyword, sValue) );
}

void DocItem::addAttribute( const Attribute& attr )
{
	m_attribs.push_back( attr );
}

/*: DocItem::find
	Finds attributes stored under keyword.

	The Keyword is case insensitive.  The attributes are returned in
	the order of original insertion.
*/
AttribIterator DocItem::find( const String& sKeyword ) const
{
	return AttribIterator( m_attribs.begin(), m_attribs.end(), sKeyword );
}

/*: DocItem::findAll()

	Returns all attributes in the original insert order.
*/
AttribIterator DocItem::findAll() const
{
	return AttribIterator( m_attribs.begin(), m_attribs.end() );
}

/*: DocItem::setPrototype()

	Sets the "*Prototype" attribute
*/
void DocItem::setPrototype( const String& sValue )
{
	addAttribute( "*Prototype", sValue );
}

/*: DocItem::setDefaultPrototype()

	Sets the "*Prototype" attribute to default value ("")
*/
void DocItem::setDefaultPrototype()
{
	addAttribute( "*Prototype", "" );
}


/*: DocItem::setLinkName()

	Sets the LinkName for this DocItem
*/
void DocItem::setLinkName( const String& sLinkName )
{
	m_sLinkName = sLinkName;
}

/*: DocItem::setDefaultLinkName()

	Sets the LinkName to a default value (ie: same as ItemName)
*/
void DocItem::setDefaultLinkName()
{
	m_sLinkName = m_sItemName;
}

/*: DocItem::setImpliedAttribute()

	Sets the "*Description" attribute
*/
void DocItem::setImpliedAttribute( const String& sValue )
{
	addAttribute( "*Description", sValue );
}

/*: DocItem::setDefaultImpliedAttribute()

	Sets the "*Description" attribute attribute to the default "".
*/
void DocItem::setDefaultImpliedAttribute()
{
	addAttribute( "*Description", "" );
}

/*: DocItem::getTitle()

	Returns the descriptive title of the item.

	This is the contents of the "Title" attribute if any.  If not defined,
	this is the first sentence of the first attribute defined.
*/
String DocItem::getTitle() const
{
	AttribIterator ai;

	// First look for predefined title
	ai = find( "Title" );
	if (!ai.atEof())
		return ai->value();

	// Not there, get first attribute
	ai = findAll();
	if (ai.atEof()) {
		// No attributes at all
		return "";
	}

	int indx = (ai->value()).indexOf( '.' );
	if (indx<0)
		return ai->value();		// Whole thing

	return (ai->value()).substring(0,++indx);
}



/*: class AttribIterator

	Iterator result of a find or findAll.

	Behaves like a pointer to an array of Attributes.
	->keyword() and ->value() return the found values, and ++ steps to the
	next found item.  The default assignment operator,
	and copy constructor can be used to create copies of the result.
*/

AttribIterator::AttribIterator()
	:	m_isAll( true ),
	    m_sKey( "" )
		// ,
	    // m_indx( 0 ),
	    // m_iEnd( 0 )
{
}

AttribIterator::AttribIterator
(
    const DocItem::Attribs::const_iterator& iBegin,
    const DocItem::Attribs::const_iterator& iEnd,
    const String& sKey
)
	:	m_isAll( false ),
	    m_sKey( sKey ),
	    m_indx( iBegin ),
	    m_iEnd( iEnd )
{
	scan();
}

AttribIterator::AttribIterator
(
    const DocItem::Attribs::const_iterator& iBegin,
    const DocItem::Attribs::const_iterator& iEnd
)
	:	m_isAll( true ),
	    m_sKey( "" ),
	    m_indx( iBegin ),
	    m_iEnd( iEnd )
{
}

/*: AttribIterator::operator++
	Prototype: ++sr
	Prototype: sr++

	Description:

	Steps to next item in found set.
*/

/*: AttribIterator::operator->

	Dereferences the iterator to return the current Attribute.

	it->value() and (*it).value() will yield the value of the
	current attribute for iterator 'it'.
	Similiarly, it->keyword() and (*it).keyword() will
	yield the keyword string.

	Prototype: Attribute* operator->()
	Prototype: Attribute& operator*()
*/

/*: AttribIterator::atEof()

	Tells if iterator points past the end of the list.  This will be true
	after 'find'ing a non-existant keyword or after stepping past the last search
	result.

	Prototype: bool AttribIterator::atEof()
*/

/*	scan -- internal routine steps m_indx forward to next matching
			keyword.  Keywords are compared case insensitive. Scan
			starts at the current item.
*/
void AttribIterator::scan()
{
	if (!m_isAll)
		while( m_indx!=m_iEnd && !((*m_indx).keyword()).equalsIgnoreCase(m_sKey) )
			m_indx++;
}

/*: Class Project

	Represents a project, a set of related files...owns DocItems for each
	class found.
*/

/*: routine Project::setName			Set's project name

	Prototype: void setName( const String& sName )
*/

/*: routine Project::getClass

	Returns a pointer to the DocClass object for a particular class.  If the
	class object already exists in this project, returns a pointer to the existing object.
	If not, it creates a new DocClass object.
*/
DocClass* Project::getClass( const String& sClass )
{
	cptr<DocClass> cp;
	ClassMap::iterator it;

	it = m_mapClasses.find( sClass );
	if (it==m_mapClasses.end()) {
		cp = new DocClass( sClass );
		m_mapClasses.insert( ClassMap::value_type(sClass, cp) );
	} else {
		cp = (*it).second;
	}
	return cp;
}

/*: routine Project::getFunction

	Returns a pointer to the Function object for the given name.
	Creates the Class object and Function object if necessary.
*/
Function* Project::getFunction( const String& sClass, const String& sName )
{
	DocClass* pcls;

	pcls = getClass( sClass );
	return pcls->getFunction( sName );
}

/*: routine Project::getVariable

	Returns a pointer to the Variable object for the given name.
	Creates the Class object and Variable object if necessary.
*/
Variable* Project::getVariable( const String& sClass, const String& sName )
{
	DocClass* pcls;

	pcls = getClass( sClass );
	return pcls->getVariable( sName );
}

/*: routine Project::getFileName()

	Returns the output filename to use.  This doesn't include a directory.
*/
String Project::getFileName() const
{
	return "index.html";
}


/*: Class DocClass

	Represents a class.  Holds the document attributes of the class.
	Owns DocItems for each Variable and Function of the class.
*/

/*: routine DocClass::getFunction

	Returns a pointer to the Function object for the given name.
	Creates the Function object if necessary.
*/
Function* DocClass::getFunction( const String& sName )
{
	cptr<Function> cp;
	FunctionMap::iterator it;

	it = m_mapFunctions.find( sName );
	if (it==m_mapFunctions.end()) {
		cp = new Function( m_sItemName, sName );
		m_mapFunctions.insert( FunctionMap::value_type(sName, cp) );
	} else {
		cp = (*it).second;
	}
	return cp;
}

/*: routine DocClass::getVariable

	Returns a pointer to the Variable object for the given name.
	Creates the Variable object if necessary.
*/
Variable* DocClass::getVariable( const String& sName )
{
	cptr<Variable> cp;
	VariableMap::iterator it;

	it = m_mapVariables.find( sName );
	if (it==m_mapVariables.end()) {
		cp = new Variable( m_sItemName, sName );
		m_mapVariables.insert( VariableMap::value_type(sName, cp) );
	} else {
		cp = (*it).second;
	}
	return cp;
}

/*: routine DocClass::getFileName

	Returns the output filename to use.  This doesn't include a directory.

	Prototype: String DocClass::getFileName()
*/

/*: class Attribute

	Represents an Keyword/Value pair.  The Keyword and Value are strings
	associated with a particular DocItem.  The default destructor,
	copy constructor, and assignment operator are used.
*/

/*:	Attribute::Attribute()

	Constructors.  Initializes keyword and value to null or given values.

	Prototype: Attribute()
	Prototype: Attribute( const String& sKeyword, const String& sValue )
*/
/*: Attribute::keyword()

	Returns keyword of the attribute.

	Prototype: String& keyword()
*/
/*: Attribute::value()

	Returns Value of the attribute.

	Prototype: String& value()
*/
/*: Attribute::operators()

	Comparison operators.  These are needed to put Attributes into
	std::lists.

	Prototype: bool operator==( const Attribute& attr ) const
	Prototype: bool operator!=( const Attribute& attr ) const
	Prototype: bool operator<( const Attribute& attr ) const
	Prototype: bool operator>( const Attribute& attr ) const

*/
