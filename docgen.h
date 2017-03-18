/* docgen.h  -- header file for program to extract docs from source code

Copyright (C) 1997-2013, Brian Bray

*/

//#include <bw/string.h>
//#include "lexstream.h"
//#include "docitem.h"

class DocGen {
public:
	DocGen();
	~DocGen();

public:
	void fileIn( const char* fileName );

	void filesOut( const char* dirName );
	enum DocItemType {tProject, tClass, tFunction, tVariable};

protected:	// Parsing routines
	bool foundDocItem();
	bool foundStarter();
	bool foundDocItemTypeAndName();
	bool foundProjectName();
	bool foundClassName();
	bool foundFunctionName();
	bool foundVariableName();
	bool foundMemberName( bw::String&, bw::String& );
	bool foundAttributeList();
	bool foundImpliedDescriptionAttribute();
	bool foundKeywordAttributeList();
	bool foundKeywordAttribute();

	void reportSyntaxError( const bw::String& sExpecting, const Token& tok ) const;

private:	// Internal Variables
	LexStream*	m_plex;

	Project		m_project;
	DocItem*	m_diCurrent;
};
