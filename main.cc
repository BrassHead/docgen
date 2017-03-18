/* docgen -- program to extract documentation from source code

Copyright (C) 1997-2013 Brian Bray

*/

#include <fstream>
#include <iostream>
#include <list>
#include <map>

#include "bw/bwassert.h"
#include "bw/exception.h"
#include "bw/countable.h"
#include "bw/string.h"
#include "lexstream.h"
#include "docitem.h"
#include "docgen.h"

using bw::BException;
using std::cout;
using std::endl;

void usage(void);

/*: Project: docgen

	This program extracts internal program documentation by reading
	specially formatted comments in the source code.  For a description
	of these comments, see the <A HREF="../grammar.html">grammar document.</A>

*/

/*: routine: main()

  Usage:
	docgen &lt;output directory> &lt;file> [&lt;file>...]
	<DL>
	<DT>&lt;output directory>
	<DD>docgen creates html files in this directory.
	<DT>&lt;file>
	<DD>input file name (eg: *.h *.cpp *.cc)
	</DL>
	<P>
	The output directory will be filled with:
	<DL>
	<DT>index.html
	<DD>class list and definition of global routines and variables
	<DT>&lt;class>.htm
	<DD>routine descriptions for each class encountered.
	</DL>
*/
int main(int argc, char* argv[])
{

	if( argc<3 ) {
		usage();
		return 1;
	}


	// For a windows version, move the following into it's own routine, so that main()
	//		just handles the command line (and thus it can be replaced
	//		with a windows program that queries for files (or gets
	//		them dropped).
	DocGen dg;

	try {
		// Input phase
		for( int i=2; i<argc; i++ ) {
			try {
				dg.fileIn( argv[i] );
			} catch( const BException& e ) {
				cout << e.message() << endl;
				cout << "continuing with next input file..." << endl;
			}
		}

		// Output phase
		dg.filesOut( argv[1] );
	} catch( const BException& e ) {
		cout << e.message() << endl;
		return 1;
	} catch(...) {
		cout << "Unexpected error encountered" << endl;
		return 2;
	}

	return 0;
}

void
usage()
{
	cout << "Usage:\n";
	cout << "\tdocgen <directory> <file> [<file>...]\n";
	cout << "\t\t<directory> -- docgen creates .html files in this directory\n";
	cout << "\t\t<file> -- input file name (eg: *.h *.cpp *.cc)\n";
	cout << "\n";
	cout << "\tThe output directory will be filled with:\n";
	cout << "\t\tindex.html -- class index and globals.\n";
	cout << "\t\t<class>.html -- routine descriptions for each class\n";
	cout << "\n";
	cout << "Copyright (C) 1997-2013 Brian Bray.\n";
	cout << "\n";
	cout << "docgen comes with ABSOLUTELY NO WARRANTY.\n";
	cout << endl;
}

