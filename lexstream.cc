/* lexstream.cc -- Simplified lexical scanner for C++ comments

Copyright (C) 1997-2013, Brian Bray

*/

#include <ios>
#include <fstream>
#include <cctype>

#include "bw/bwassert.h"
#include "bw/string.h"
#include "bw/exception.h"
#include "lexstream.h"

using bw::BFileException;
using bw::String;

static const String scOpSyms("+-*/%^&|~!=<>[]");
static const String scWhiteSpace(" \t\f\v*/");

///////////////////////////////////////////////////////////////////////////////
/*: class Token
		 A single token from the input stream.

	Note: uses default destructor, copy constructor, and assignment.
*/

/*:	routine Token::Token		Constructor		*/
Token::Token()
	:	m_ttType( NullToken )
{
}


/*:	routine Token::clear		Empties token of previous contents	*/
void
Token::clear()
{
	m_ttType = NullToken;
	m_sToken = "";
}

/*: routine Token::value		Returns pointer to string containing token value	*/
const String&
Token::value() const
{
	return m_sToken;
}

/*: routine Token::type			Returns type of token	*/
Token::TokenType
Token::type() const
{
	return m_ttType;
}


///////////////////////////////////////////////////////////////////////////////
/*: class LexStream

	An input stream of tokens attached to a file.

	Tokenizes input steam for parsing inline documentation.
	Ignores preprocessor directives.  Goal directed.
*/

/*: routine LexStream::LexStream #ctor1
	Opens file for Lexical scanning.

	Throws: if file does not exist
*/
LexStream::LexStream( const char* fileName )
	:	m_fileInput( 0 ),
	    // m_isCallerOwned( false ),
	    m_isPeeked( false )
{
	m_fileInput = new std::ifstream( fileName, std::ios_base::in /* | std::ios_base::nocreate | ios::binary*/ );
	if (!m_fileInput->is_open())
		throw BFileException( BFileException::FileNotFound );
}

/*: routine LexStream::LexStream #ctor2

	Opens token stream on existing istream.

	Requires: fInput is an open istream positioned for reading, no other code should manipulate
			'fInput' until the LexStream is deleted.  Caller must close/delete the stream after use.
*/
LexStream::LexStream
(
    std::ifstream& fInput
)
	:	// m_isCallerOwned( true ),
	    m_isPeeked( false )
{
	m_fileInput = &fInput;
}


/*: routine LexStream::getStartSymbol

	Finds the next special comment start symbol "/ * :" in the text.

*/
void LexStream::getStartSymbol( Token& tok )
{
	bwassert( m_fileInput );

	// Since this routine is called after bailing out from a syntax
	// error, we'll start by clearing the peek buffer.
	m_isPeeked = false;
	m_tokPeekBuffer.clear();

	// Get there
	while( m_fileInput->eof()==0 ) {
		if (m_fileInput->get()=='/') {
			if (m_fileInput->peek()=='*') {		// allows " / / * : "
				m_fileInput->get();
				if (m_fileInput->get()==':')
					break;
			}
		}
	}

	tok.clear();

	if (m_fileInput->eof()) {
		tok.m_ttType = Token::EndOfFile;
	} else {
		tok.m_ttType = Token::Symbol;
		tok.m_sToken = "/*" ":";
	}
}


/*: routine LexStream::getToken

	Returns the next token in the input stream.

	A token is one of:
	<UL>
	<LI>	End of file
	<LI>	A symbol used within comments (ie: "* /", ":", "::" "()" or "#")
//	<LI>	A keyword used within comments (eg: "Description: at the start of a line)
	<LI>	An identifier (an alphanumeric sequence, or "operator" followed by some symbols)
	<LI>	Text (a sequence of arbitrary characters terminated by a keyword if
			scanning AttributeText or a ":", "{" or ";"  if scanning a prototype.)
	</UL>

	See the Grammar rules document for details.
*/
void
LexStream::getToken
(
    Token& tok
)
{
	// Check the peek buffer first
	if (m_isPeeked) {
		m_isPeeked = false;
		tok = m_tokPeekBuffer;
		return;
	}


	// No luck, I guess I'll have to do it.

	char ch;
	char ch2;

	tok.clear();

	// Get a significant character
	bool isSignificant = false;
	bool inLeadingWhitespace = false;
	while (!isSignificant) {
		ch = m_fileInput->get();
		switch (ch) {
		case ' ':
		case '\t':
		case '\f':
		case '\v':
			break;			// Whitespace ignored
		case '\n':
		case '\r':
			inLeadingWhitespace = true;
			break;

		case '*':
			ch2 = m_fileInput->peek();
			if (inLeadingWhitespace && ch2!='/')
				break;					// Stars ignored in leading whitespace
			isSignificant = true;		// but not / *
			break;

		case '/':
			if (inLeadingWhitespace)
				break;
			// otherwise fall through

		default:
			isSignificant = true;
		}
	}

	if (m_fileInput->eof()) {
		tok.m_ttType = Token::EndOfFile;
		return;
	}

	// Start to assemble output token
	tok.m_sToken = ch;
	ch2 = m_fileInput->peek();

	// Is it some kind of Identifier ?
	if (ch=='_' || ch=='~' || isalpha(ch) ) {
		// Yes
		tok.m_ttType = Token::Identifier;
		while (ch2=='_' || isalnum(ch2)) {
			tok.m_sToken.append( m_fileInput->get() );
			ch2 = m_fileInput->peek();
		}

		// We now have a basic Identifier,  there are a few special cases
		//if (inLeadingWhitespace && ch==':')
		//{
		//	// It's Keyword
		//	tok.m_ttType = Keyword;
		//	m_fileInput->get();
		//}

		if (tok.m_sToken == "operator") {
			// It's a operator FunctionIdentifier
			if (ch2=='(') {
				// for a cast or an operator()
				do {
					tok.m_sToken.append( m_fileInput->get() );
					ch2 = m_fileInput->peek();
				} while (ch2=='_' || isalnum(ch2) || ch2=='*' || ch2=='&');

				if (ch2==')') {
					tok.m_sToken.append( m_fileInput->get() );
				}
			} else {
				// with operator symbols
				while ( scOpSyms.indexOf(ch2)>=0 ) {
					tok.m_sToken.append( m_fileInput->get() );
					ch2 = m_fileInput->peek();
				}
			}
		}
		return;
	}

	// Must be some kind of Symbol
	tok.m_ttType = Token::Symbol;

	// Check for double symbols
	switch (ch) {
	case '*':
		if (ch2=='/') {
			tok.m_sToken.append( m_fileInput->get() );
		}
		break;

	case ':':
		if (ch2==':') {
			tok.m_sToken.append( m_fileInput->get() );
		}
		break;

	case '(':
		if (ch2==')') {
			tok.m_sToken.append( m_fileInput->get() );
		}
		break;
	}
	return;
}

/*: routine LexStream::peekToken

	Returns the next token in the input stream without advancing the stream.
*/
void LexStream::peekToken( Token& tok )
{
	if (!m_isPeeked) {
		getToken( m_tokPeekBuffer );
		m_isPeeked = true;
	}
	tok = m_tokPeekBuffer;
}



/*: routine LexStream::getAttributeText

	Returns all characters up to, but not including next Keyword,
	EndSymbol, or EOF.
*/
void LexStream::getAttributeText( Token& tok )
{
	char ch;
	char ch2;

	enum {Copying, LeadingWhitespace, CheckingForKeyword, Finished} state;

	tok.clear();
	state = LeadingWhitespace;
	ch = m_fileInput->peek();
	if( m_isPeeked ) {
		switch (m_tokPeekBuffer.type()) {
		case Token::Identifier:
			if ( ch==':' ) {
				// This is already the keyword we're looking for
				ch = m_fileInput->get();		// Eat :
				ch = m_fileInput->peek();
				if (ch!=':')
					return;			// return NullToken and don't advance
				else
					m_tokPeekBuffer.m_sToken.append(':');		// We read this
			}
			break;

		case Token::EndOfFile:
			tok = m_tokPeekBuffer;
			return;				// return end of file

		case Token::Symbol:
			if (m_tokPeekBuffer.m_sToken == "*" ||
			        m_tokPeekBuffer.m_sToken == "/" ) {
				m_isPeeked = false;		// Ignore in leading whitespace
				break;
			}
			if (m_tokPeekBuffer.m_sToken == "*/") {
				return;					// Were at the end, return NullToken
			}

		default:
			// Otherwise fall through to start the output
			;
		}

		if (m_isPeeked) {		// Still?
			// The peek buffer contains the start of the AttributeText
			tok = m_tokPeekBuffer;
			m_isPeeked = false;
			state = Copying;
		}

	}

	tok.m_ttType = Token::Text;
	m_tokPeekBuffer.clear();

	while (state!=Finished) {
		ch = m_fileInput->get();
		ch2 = m_fileInput->peek();

		// Handle end of comment (since it's nearly the same in all states)
		if (m_fileInput->eof() || (ch=='*' && ch2=='/')) {
			if (state==CheckingForKeyword) {
				tok.m_sToken.append( m_tokPeekBuffer.m_sToken );
			}
			m_tokPeekBuffer.m_sToken = "*/";
			m_tokPeekBuffer.m_ttType = Token::Symbol;
			ch = m_fileInput->get();		// Eat "/"
			m_isPeeked = true;
			state = Finished;
		}

		switch (state) {
		case Copying:
			tok.m_sToken.append( ch );
			if (ch == '\r' || ch=='\n')
				state = LeadingWhitespace;
			break;

		case LeadingWhitespace:
			if ( scWhiteSpace.indexOf(ch)>=0 )
				break;
			if (ch=='\r' || ch=='\n') {
				tok.m_sToken.append( ch );
				break;
			}
			// Otherwise fall through to CheckingForKeyword
			state = CheckingForKeyword;

		case CheckingForKeyword:
			m_tokPeekBuffer.m_sToken.append(ch);
			if ( ch2=='_' || isalnum(ch2) )
				break;			// Still in Identifier
			if (ch2==':') {
				// It may be a keyword
				ch = m_fileInput->get();
				ch2 = m_fileInput->peek();
				if (ch2==':') {
					// No, it's a ::
					tok.m_sToken.append( m_tokPeekBuffer.m_sToken );
					m_tokPeekBuffer.clear();
					state = Copying;
					tok.m_sToken.append(ch);
					break;
				} else {
					m_tokPeekBuffer.m_ttType = Token::Identifier;
					m_isPeeked = true;
					state = Finished;
					break;
				}
			}
			// Otherwise, it's the end of the Identifier without a ':'
			// so it's just part of the AttributeText
			tok.m_sToken.append( m_tokPeekBuffer.m_sToken );
			m_tokPeekBuffer.clear();
			state = Copying;
			break;

		case Finished:
			break;
		}
	}
	return;
}


/*: LexStream::getPrototype()

	Get function prototype following comment block.

	This gets all characters until the next ';', ':', EndOfFile or '{',
	but it's not terminated by '::'.
*/
void LexStream::getPrototype( Token& tok )
{
	char ch;

	tok.clear();
	bool isFinished = false;
	while (!isFinished) {
		ch = m_fileInput->peek();
		if (m_fileInput->eof()) {
			isFinished = true;
			break;
		}

		switch (ch) {
		case ':':
			m_fileInput->get();
			ch = m_fileInput->peek();
			if (ch==':') {
				tok.m_sToken.append( "::" );
				tok.m_ttType = Token::Text;
				m_fileInput->get();
			} else
				isFinished = true;
			break;

		case ';':
		case '{':
			isFinished = true;
			break;

		default:
			tok.m_sToken.append( m_fileInput->get() );
			tok.m_ttType = Token::Text;
		}
	}
	return;
}

/*: routine LexStream::atEof()			End of File indicator	*/
bool LexStream::atEof()
{
	return m_fileInput->eof();
}
