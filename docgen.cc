/* docgen.cc  -- main class to extract documentation from source code

Copyright (C) 1997-2013 Brian Bray

*/

#define NOTRACE
#include <bw/trace.h>

#include <fstream>
#include <list>
#include <map>

#include "bw/bwassert.h"
#include "bw/countable.h"
#include "bw/string.h"
#include "lexstream.h"
#include "docitem.h"
#include "docgen.h"

using bw::String;
using std::endl;

// Constant strings
sconsti(	scEndSymbol,		"*/"	);
sconsti(	scHashSymbol,		"#"	);
sconsti(	scColonSymbol,		":"	);
sconsti(	scParensSymbol,		"()"	);
sconsti(	scDblColonSymbol,	"::"	);
sconst(		scGlobal,		""	);	// Class name used for Global Symbols

static const String sDocItemTypes[] = {
	"Project",
	"Class",
	"Variable",
	"Function",
	"Member",
	"Routine"
};
static const DocGen::DocItemType typeList[] = {
	DocGen::tProject,
	DocGen::tClass,
	DocGen::tVariable,
	DocGen::tFunction,
	DocGen::tFunction,
	DocGen::tFunction
};
static const int numDITs=6;

// This helper routine searched the DocItem type list and returns
// an Index into the typeList if found or -1 if not found
static int findType( const String& s )
{
	for (int i=0; i<numDITs; i++)
		if (s.equalsIgnoreCase(sDocItemTypes[i]))
			return i;

	return -1;
}


DocGen::DocGen()
	:	m_plex( 0 ),
	    m_diCurrent( 0 )
{
}


DocGen::~DocGen()
{
}


void
DocGen::fileIn
(
    const char* fileName
)
{
	trace << "fileIn ( \"" << fileName << "\" );" << endl;

	m_plex = new LexStream( fileName );

	while (!m_plex->atEof()) {
		if (foundDocItem()) {
			trace << "Found DocItem" << endl;
		}
	}

	delete m_plex;
	m_plex = 0;

}

void
DocGen::filesOut( const char* dirName )
{
	trace << "filesOut ( \"" << dirName << "\" );" << endl;
	m_project.filesOut( dirName );
}

bool DocGen::foundDocItem()
{
	Token tok;
	bwassert( m_plex );
	bwassert( m_diCurrent==0 );		// No current DocItem

	if (foundStarter()) {
		bwassert( m_diCurrent );
		// Now inside a special comment block
		if (foundAttributeList()) {
			trace << "Found AttributeList" << endl;
		}

		// Now expecting EndSymbol
		m_plex->getToken(tok);
		if (tok.type()!=Token::Symbol || tok.value()!=scEndSymbol) {
			// Didn't get it -- warn and skip till found
			reportSyntaxError( "EndSymbol", tok );

			while (tok.type()!=Token::EndOfFile ||
			        tok.type()!=Token::Symbol || tok.value()!=scEndSymbol) {
				m_plex->getToken(tok);
			}
		}

		// Right now, I can't check to see if I need to look for
		// a prototype, so I'll always check for functions and variables.
		if (tok.type()!=Token::EndOfFile && m_diCurrent->needPrototype()) {
			m_plex->getPrototype( tok );
			if (tok.type()==Token::Text)
				m_diCurrent->setPrototype( tok.value() );
			else
				m_diCurrent->setDefaultPrototype();
		}

		m_diCurrent = 0;			// No longer a current DocItem
		return true;
	}
	return false;
}

/*	DocGen::foundStarter()

	Parse Starter Symbol (includes type and name)

	Returns: true if found and m_diCurrent will be set.
*/
bool DocGen::foundStarter()
{
	Token tok;

	m_plex->getStartSymbol( tok );
	if (tok.type()==Token::Symbol) {
		// Got start of new comment block
		//trace << "Found StartSymbol" << endl;

		if (foundDocItemTypeAndName()) {
			bwassert( m_diCurrent );
			// OK, I can work with this

			bool hasLinkName = false;

			// Check for LinkName
			m_plex->peekToken( tok );
			if (tok.type()==Token::Symbol && tok.value()==scHashSymbol) {
				// Yes, there is a linkname
				m_plex->getToken( tok );		// Eat the hash mark
				bwassert( tok.type()==Token::Symbol );
				bwassert( tok.value()==scHashSymbol );
				m_plex->getToken( tok );		// My link name
				if (tok.type()!=Token::Identifier) {
					reportSyntaxError( "LinkName", tok );
				} else {
					hasLinkName = true;
					m_diCurrent->setLinkName( tok.value() );
				}
			}
			if (!hasLinkName) {
				m_diCurrent->setDefaultLinkName();
			}
			return true;
		}
	}
	return false;
}

/*	DocGen::foundDocItemTypeAndName()

	Parses type (eg: class, routine) and full name of item.

	Returns: true if found and sets m_diCurrent
*/
bool DocGen::foundDocItemTypeAndName()
{
	Token tok;
	DocItemType typeCurDocItem;

	// This first section determines the DocItem type and leaves the
	// LexStream positioned at the name

	m_plex->peekToken( tok );
	typeCurDocItem = tFunction;		// This is the default

	// See if there's a type keyword
	if (tok.type()==Token::Identifier) {
		int indx = findType( tok.value() );
		if (indx>=0) {
			typeCurDocItem = typeList[indx];
			m_plex->getToken( tok );	// Eat the keyword

			m_plex->peekToken( tok );
			if (tok.type()==Token::Symbol && tok.value()==scColonSymbol) {
				m_plex->getToken( tok );	// Eat the ":"
			}
		}
	}

	// This second section picks up the name.
	//
	switch (typeCurDocItem) {
	case tProject:
		return foundProjectName();

	case tClass:
		return foundClassName();

	case tVariable:
		return foundVariableName();

	case tFunction:
		return foundFunctionName();
	}

	bwassert( false );
	return false;
}

bool DocGen::foundProjectName()
{
	Token tok;

	m_plex->getToken( tok );
	if (tok.type()!=Token::Identifier) {
		reportSyntaxError( "ProjectName", tok );
		return false;
	}
	m_diCurrent = &m_project;
	m_project.setName( tok.value() );
	//trace << "Project name set to \"" << tok.value() << "\"" << endl;
	return true;
}

bool DocGen::foundClassName()
{
	Token tok;

	m_plex->getToken( tok );
	if (tok.type()!=Token::Identifier) {
		reportSyntaxError( "ClassName", tok );
		return false;
	}
	m_diCurrent = m_project.getClass( tok.value() );
	//trace << "Class name set to \"" << tok.value() << "\"" << endl;
	return true;
}

bool DocGen::foundFunctionName()
{
	Token tok;
	String sClassName;
	String sFunctionName;

	if (foundMemberName( sClassName, sFunctionName )) {
		// Eat optional trailing ()
		m_plex->peekToken( tok );
		if (tok.type()==Token::Symbol && tok.value()==scParensSymbol)
			m_plex->getToken( tok );

		m_diCurrent = m_project.getFunction( sClassName, sFunctionName );
		//trace << "Member name reset to " << m_sCurClassName << "::" << m_sCurMemberName << endl;
		return true;
	}
	return false;
}

bool DocGen::foundVariableName()
{
	String sClassName;
	String sVariableName;

	if (foundMemberName( sClassName, sVariableName )) {
		m_diCurrent = m_project.getVariable( sClassName, sVariableName );
		return true;
	}
	return false;
}


bool DocGen::foundMemberName( String& sClassName, String& sMemberName  )
{
	Token tok;
	Token	tok2;

	m_plex->getToken( tok );
	m_plex->peekToken( tok2 );

	// This first part determines the class name
	if (tok.value()==scDblColonSymbol) {
		sClassName = scGlobal;
		m_plex->getToken( tok );				// Contains MemberName
	} else if(tok2.value()==scDblColonSymbol) {
		sClassName = tok.value();
		m_plex->getToken( tok );				// Eat "::"
		m_plex->getToken( tok );				// Contains MemberName
	}
	// If not, the membername is already in tok

	// This second part processes the member name
	if (tok.type()!=Token::Identifier) {
		reportSyntaxError( "MemberName", tok );
		return false;
	}
	sMemberName = tok.value();
	//trace << "Member name set to " << m_sCurClassName << "::" << m_sCurMemberName << endl;
	return true;
}

bool DocGen::foundAttributeList()
{
	if (foundImpliedDescriptionAttribute()) {
		//trace << "Found ImpliedDescriptionAttribute" << endl;
	}

	return foundKeywordAttributeList();
}

bool DocGen::foundImpliedDescriptionAttribute()
{
	Token tok;

	m_plex->getAttributeText( tok );

	if (tok.type()==Token::Text) {
		m_diCurrent->setImpliedAttribute( tok.value() );
		//trace << "Found Description: \"" << tok.value() << "\"" << endl;

		return true;
	}

	m_diCurrent->setDefaultImpliedAttribute();
	return false;
}

bool DocGen::foundKeywordAttributeList()
{
	bool anyFound = false;

	while (foundKeywordAttribute()) {
		anyFound = true;
	}
	return anyFound;
}

bool DocGen::foundKeywordAttribute()
{
	Token tok;

	m_plex->peekToken( tok );
	if (tok.type()==Token::Identifier) {
		m_plex->getToken( tok );
		String sKeywordName = tok.value();
		// Note: This is a hack, but the following ":" has already been read.

		m_plex->getAttributeText( tok );

		m_diCurrent->addAttribute( sKeywordName, tok.value() );
		//trace << "Found Attribute " << m_sCurKeywordName << ": " << endl
		//		<< "\"" << tok.value() << "\"" << endl;
		return true;
	}
	return false;
}

void DocGen::reportSyntaxError( const String& sExpecting, const Token& tok ) const
{
	trace << endl;
	trace << "Syntax error: Expected " << sExpecting;
	trace << ", but got \"" << tok.value() << "\"" << endl << endl;

	// Syntax errors are ignored in the release version.
}
