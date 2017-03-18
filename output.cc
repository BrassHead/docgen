/* output.cc  -- Output routines for DocGen

Copyright (C) 1997-2013, Brian Bray

*/

#include <fstream>
#include <list>
#include <map>

#include "bw/bwassert.h"
#include "bw/countable.h"
#include "bw/exception.h"
#include "bw/string.h"
#include "bw/html.h"
#include "docitem.h"

using bw::BFileException;
using bw::html;
using bw::String;
using std::ostream;
using std::ofstream;


/*: routine Project::filesOut

	Outputs all documentation files for the project into the given
	directory.

	This routine looks after all the file opening and HTML prolog
	and epilog.  The overloaded << operators output individual DocItems
	into the BODY of the HTML output.

	Throws if file(s) cannot be opened.
*/
void Project::filesOut( const String& sDir )
{
	// First, create project file

	{
		ofstream os( sDir + "/" + getFileName() );
		if (!os.is_open())
			throw BFileException( BFileException::SystemError );

		os << html::prolog( getFullDisplayName(), "docgen by Brian Bray" );
		os << *this;
		os << html::epilog;
	}

	// Now write each Class file.

	ClassMap::iterator it;
	it = m_mapClasses.begin();
	while (it!=m_mapClasses.end()) {
		if ((*it).second->getName()!="") {		// Globals already done
			ofstream os( sDir + "/" + (*it).second->getFileName() );
			if (!os.is_open())
				throw BFileException( BFileException::SystemError );

			os << html::prolog( (*it).second->getFullDisplayName(),
			                    "docgen by Brian Bray" );
			os << *((*it).second);
			os << html::epilog;
		}
		++it;
	}
}


/*: routine Project::operator<<

	Output the body of a project file.
*/
ostream& operator<<( ostream& os, const Project& proj )
{
	os << *((DocItem*)&proj);			// Generic attribute output

	// Now output the index of classes
	Project::ClassMap::const_iterator it;
	it = proj.m_mapClasses.begin();
	if (it!=proj.m_mapClasses.end()) {
		os << html::heading3( proj.getFullDisplayName() + " classes" );
	}
	os << html::beginTable(2);

	while (it!=proj.m_mapClasses.end()) {
		os << html::beginRow;
		os << html::beginCell;
		os << html::beginLink((*it).second->getFileName());
		os << (*it).second->getDisplayName();
		os << html::endLink;
		os << html::nextCell;
		os << (*it).second->getTitle();
		os << html::endCell;
		os << html::endRow;
		++it;
	}

	os << html::endTable;
	os << html::rule;

	// Now output the index of Globals (if any)

	os << html::heading3( proj.getFullDisplayName() + " globals" );

	it = proj.m_mapClasses.find( "" );			// Global "Class"
	if (it!=proj.m_mapClasses.end()) {
		os << *((*it).second);					// Output Class documentation
	} else {
		os << html::boldOn << "No Global functions or variables" << html::boldOff;
	}
	return os;
}


/*: DocClass::operator<<

	Output the body of a class documentation file.
*/
ostream& operator<<( ostream& os, const DocClass& cls )
{
	os << *((DocItem*)&cls);			// Generic attribute output

	// Now output the index of routines
	{
		DocClass::FunctionMap::const_iterator it;
		it = cls.m_mapFunctions.begin();
		if (it!=cls.m_mapFunctions.end()) {
			os << html::heading3( cls.getFullDisplayName() + " member functions" );
		}
		os << html::beginTable(2);

		while (it!=cls.m_mapFunctions.end()) {
			os << html::beginRow;
			os << html::beginCell;
			os << html::beginLink2Link((*it).second->getLinkName());
			os << (*it).second->getDisplayName();
			os << html::endLink;
			os << html::nextCell;
			os << (*it).second->getTitle();
			os << html::endCell;
			os << html::endRow;
			++it;
		}

		os << html::endTable;
	}

	// Now output the index of Variables
	{
		DocClass::VariableMap::const_iterator it;
		it = cls.m_mapVariables.begin();
		if (it!=cls.m_mapVariables.end()) {
			os << html::heading3( cls.getFullDisplayName() + " member variables" );
		}
		os << html::beginTable(2);

		while (it!=cls.m_mapVariables.end()) {
			os << html::beginRow;
			os << html::beginCell;
			os << html::beginLink2Link((*it).second->getLinkName());
			os << (*it).second->getDisplayName();
			os << html::endLink;
			os << html::nextCell;
			os << (*it).second->getTitle();
			os << html::endCell;
			os << html::endRow;
			++it;
		}

		os << html::endTable;
	}

	// Now output the Function details
	{
		DocClass::FunctionMap::const_iterator it;

		os << html::rule;

		it = cls.m_mapFunctions.begin();
		while (it!=cls.m_mapFunctions.end()) {
			os << *((*it).second);
			os << html::rule;
			++it;
		}
	}

	// Now output the Variable details
	{
		DocClass::VariableMap::const_iterator it;

		it = cls.m_mapVariables.begin();
		while (it!=cls.m_mapVariables.end()) {
			os << *((*it).second);
			os << html::rule;
			++it;
		}
	}

	return os;
}

/*: DocItem::operator<<

	Generic output of a DocItem attributes
*/
ostream& operator<<( ostream& os, const DocItem& di )
{
	AttribIterator it;

	// Heading
	os << html::defineLink( di.getLinkName() );
	os << html::heading1( di.getFullDisplayName() );

	// First, prototype
	it = di.find( "*Prototype" );
	while (!it.atEof()) {
		os << html::newPara << html::italicOn;
		os << html::literal( (*it).value() );
		os << html::italicOff;
		++it;
	}
	it = di.find( "Prototype" );
	while (!it.atEof()) {
		os << html::newPara << html::italicOn;
		os << html::literal( (*it).value() );
		os << html::italicOff;
		++it;
	}

	// Now, description
	it = di.find( "*Description" );
	while (!it.atEof()) {
		os << html::newPara;
		os << html::smartFormat( (*it).value() );
		++it;
	}
	it = di.find( "Description" );
	while (!it.atEof()) {
		os << html::newPara;
		os << html::smartFormat( (*it).value() );
		++it;
	}

	// Now, every other attribute
	os << html::beginDefinitionList;

	it = di.findAll();
	while (!it.atEof()) {
		String sKey = (*it).keyword();
		if (! (	sKey.equalsIgnoreCase("*Prototype") ||
		        sKey.equalsIgnoreCase("Prototype") ||
		        sKey.equalsIgnoreCase("*Description") ||
		        sKey.equalsIgnoreCase("Description") ) ) {
			os << html::definition( sKey + ":" );
			os << (*it).value();
		}
		++it;
	}

	os << html::endDefinitionList;

	return os;
}
