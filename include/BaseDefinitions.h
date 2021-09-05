// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <regex>



namespace BCForth
{


	using std::cout, std::endl;
	using std::string;
	using std::vector;





	using CellType	= size_t;
	using RawByte	= unsigned char;
	using Char		= char;

	using size_type = size_t;

	using SignedIntType = long long;		// it is good and efficient to have the SignedIntType 
											// the same length as the basic CellType (otherwise e.g. address
											// arithmetic causes address crop to 4 bytes if int is 4, etc.)
	static_assert( sizeof( CellType ) == sizeof( SignedIntType ) );

	using FloatType		= double;
	static_assert( sizeof( CellType ) == sizeof( FloatType ) );


	constexpr auto CellTypeSize = sizeof( CellType );


	constexpr auto FORTH_IS_CASE_INSENSITIVE { true };		// set to false to make Forth case sensitive (anyway, all built-in words are uppercase)


	template< typename T >
	constexpr T kTrue = T( 1 );  

	template< typename T >
	constexpr T kFalse = T( 0 );  


	constexpr CellType kBoolTrue	= static_cast< CellType >( kTrue< bool > );
	constexpr CellType kBoolFalse	= static_cast< CellType >( kFalse< bool > );



	enum EIntCompBase { kBin = 2, kOct = 8, kDec = 10, kHex = 16 };


	// 8 kB for the PAD temporary storage area
	inline const size_type k_PAD_Size { 8 * 1024 };



	using Name = std::string;			// maybe std::wstring in the future?


	using Letter = Name::value_type;


	constexpr	const Letter kSpace		{ ' ' };
	constexpr	const Letter kTab		{ '\t' };
				const Name kCR			{ "\n" };

	constexpr	const Letter kColon		{ ':' };
	constexpr	const Letter kSemColon	{ ';' };


	constexpr	const Letter kLeftParen		{ '(' };
	constexpr	const Letter kRightParen	{ ')' };
	constexpr	const Letter kBackSlash		{ '\\' };


				const Name kBlanks { " \t\n" };


				const std::regex kBlanksRe { "[[:s:]]+" }; // any whitespace, one or more times

				const Name		kDotQuote	{ ".\"" };
	constexpr	const Letter	kQuote		{ '\"' };

				const Name		kAbortQuote	{ "ABORT\"" };
				const Name		kCommaQuote	{ ",\"" };
				const Name		kSQuote		{ "S\"" };
				const Name		kCQuote		{ "C\"" };

	constexpr	const Letter kPlus			{ '+' };




	using Names = std::vector< Name >;



	////////////////////////////////////////////////////////////////
	// Auxiliary functions


	// Does simple re-interpretation rather than a value conversion.
	template < typename S, typename T >
	constexpr S BlindValueReInterpretation( T t_val )
	{
		if constexpr ( sizeof( S ) > sizeof( T ) )
			return ( * reinterpret_cast< S * >( & t_val ) ) & ( ( 1ull << sizeof( T ) * 8 ) - 1 );		// cut out excessive value (i.e. clear the bits from MSB up to bit_len( T )
		else
			return * reinterpret_cast< S * >( & t_val );
	}




	auto Letter_2_Name( const Letter letter )
	{
		return Name( sizeof( Letter ), letter );
	}


	auto ContainsSubstrAt( const Name & n, const Name & substr )
	{
		return n.find( substr );
	}

	auto ContainsSubstrAt( const Name & n, const Letter letter )
	{
		return ContainsSubstrAt( n, Letter_2_Name( letter ) );
	}


	auto SplitAt( const Name & n, auto pos )
	{
		return std::make_tuple< Name, Name >( Name( n.begin(), n.begin() + pos ), Name( n.begin() + pos, n.end() ) );
	}




	// A helper to push a cell type object into the raw byte array
	template < typename V >
	constexpr void PushValTo( std::vector< RawByte > & byte_ar, V val )
	{
		for( auto i { 0 }; i < sizeof( val ); ++ i )
			byte_ar.push_back( val & 0xFF ), val >>= 8;
	}



	// Custom error type - the message will be displayed to the user
	struct ForthError : std::runtime_error
	{
		ForthError( const std::string & what_arg ) : runtime_error( what_arg ) {}
		ForthError( const char * what_arg ) : runtime_error( what_arg ) {}
	};


}	// The end of the BCForth namespace


