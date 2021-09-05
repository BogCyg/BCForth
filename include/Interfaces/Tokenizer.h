// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once




#include "BaseDefinitions.h"









namespace BCForth
{





// The IO reader
template < bool CaseInsensitive = FORTH_IS_CASE_INSENSITIVE >
class TForthReader
{

	// Some grep definitions

protected:


	Name StripEndingComment( Name & ln )
	{
		if( auto pos = ln.find( kBackSlash ); pos != Name::npos )
			ln.erase( ln.begin() + pos, ln.end() );

		return ln;
	}


public:


	// Read a line or lines and return the names
	virtual Names operator() ( std::istream & i )
	{

		Name lines;

		// This is a simple state machine:
		//		Take the first line and check if it beginns with the colon (i.e. it is a word definition);
		//			If so, then read all lines up and including the one that contains a semicolon (i.e. a word end).
		//		If no colon, then it is a word call


		if( Name ln; std::getline( i, ln ) )
		{
			lines = StripEndingComment( ln );

			if( auto pos = ln.find_first_not_of( kBlanks ); pos != Name::npos && ln[ pos ] == kColon )
				while( ln.find( kSemColon, pos ) == Name::npos && std::getline( i, ln )  )		// read new lines until a one with ; is found
					lines += kCR + StripEndingComment( ln );		// add CR since getline swallows it

		}

		// Split all these into the nonempty tokens
		Names outNames;
		std::copy_if(	std::sregex_token_iterator( lines.cbegin(), lines.cend(), kBlanksRe, -1 ), std::sregex_token_iterator(), 
						std::back_inserter( outNames ), [] ( const auto & s ) { return s.length() > 0; } );



		if constexpr( CaseInsensitive )
		{

			// Toggle cur_skip_flag on discovering inStr in n or outStr in n
			// cur_skip_flag == true means we are in the special "don't case change" mode
			// returned true means we enter the "special' mode
			auto assessSkipFlag = [] ( bool cur_skip_flag, const Name & n, const Name & inStr, const Name & outStr ) {

				if( cur_skip_flag == false && n == inStr )		// Toggle flag - to prevent entering for the second inStr
					return true;
				else
					if( cur_skip_flag == true && ContainsSubstrAt( n, outStr ) != Name::npos )
						return false;

				return cur_skip_flag;
			};



			// Here we convert each token to the UPPERCASE, however there are exceptions (e.g. the user entered text)
			for( bool skip_flags [] = { false, false }; auto & n : outNames )		// convert all to uppercase
			{

				if( skip_flags[ 0 ] == false && skip_flags[ 1 ] == false )
					std::transform( n.begin(), n.end(), n.begin(), [] ( const auto c ) { return static_cast< Char >( std::toupper( c ) ); } );

				if( skip_flags[ 1 ] == false )															// Find first such that
					for( auto prev_skip_flag_0 { skip_flags[ 0 ] }; const auto & specModeEnterToken : { kDotQuote, kCommaQuote, kAbortQuote, kCQuote, kSQuote } )
						if( prev_skip_flag_0 ^ ( skip_flags[ 0 ] = assessSkipFlag( skip_flags[ 0 ], n, specModeEnterToken, Name( 1, kQuote ) ) ) )
							break;	// if there was a toggle from the "spec mode" to the "normal one", or vice versa, then do NOT check anything more (to prevent checking the "trigerring" token twice, such as ,")


				if( skip_flags[ 0 ] == false )
					skip_flags[ 1 ] = assessSkipFlag( skip_flags[ 1 ], n, Name( 1, kLeftParen ), Name( 1, kRightParen ) );

			}

		}



		return outNames;

	}




};




}	// The end of the BCForth namespace




