/* lexstream.h -- Interface to simplified lexical scanner for C++

Copyright (C) 1997-2013 Brian Bray

*/


//#include <fstream.h>
//#include <bw/string.h>


//	 Tokens represents a single token from the input stream
class Token {
public:	// Friends
	friend class LexStream;

public:	// Ctor, dtor
	Token();

public: // Data Access

	enum TokenType {
		NullToken,
		EndOfFile,
		Symbol,
		Keyword,
		Identifier,
		Text,
	};

	void clear();
	const bw::String& value() const;
	TokenType type() const;

private:	// data members
	TokenType	m_ttType;
	bw::String	m_sToken;
};



//	Represents an input steam of tokens.
class LexStream {
public:	// Initializers

	LexStream( const char* fileName );
	LexStream( std::ifstream& fInput );

public:	// Input member functions

	void getStartSymbol( Token& tok );
	void getToken( Token& tok );
	void peekToken( Token& tok );
	void getAttributeText( Token& tok );
	void getPrototype( Token& tok );
	bool atEof();

private:
	std::ifstream*	m_fileInput;
	// bool			m_isCallerOwned;
	Token			m_tokPeekBuffer;
	bool			m_isPeeked;
};

