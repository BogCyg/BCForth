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
#include <algorithm>
#include <concepts>
#include <tuple>
#include <iterator>
#include <functional>
#include <fstream>


#include "BaseDefinitions.h"
#include "TheStack.h"







namespace BCForth
{




	
	template< typename T >
	concept valid_forth_env = requires ( T & t )
	{	
		typename T::DataStack;		// Forth environment has to define DataStack type
		t.GetDataStack();			// and we can access it from every word
	};





	// The TWord objects are organized as a composite DP
	// See my book: Introduction to Programming with C++ for Engineers, Wiley 2021

	template < typename Base >
	class TWord
	{
		Base &	fForth;		// a ref to the Forth base class with data structures

	public:

		using WordPtr = typename Base::WordPtr;


	protected:


		auto & GetDataStack( void ) { return fForth.GetDataStack(); }

		auto & GetForth( void ) { return fForth; }

	public:

		TWord( Base & f ) : fForth( f ) {}
		virtual ~TWord() = default;

	public:


		///////////////////////////////////////////////////////////
		// The main "action" of each Forth's word
		///////////////////////////////////////////////////////////
		//
		// INPUT:
		//			none
		// OUTPUT:
		//			none
		//
		// REMARKS:
		//
		//
		virtual void operator () ( void ) = 0;

	};





	// Generic operation that affects the data stack
	// This is useful if the supplied u_op lambda has a non-empty caption
	template < typename Base >
	requires  valid_forth_env< Base >		// it is sufficient to have this constraint in one word from the hierarchy
	class GenericStackOp : public TWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;


		std::function< bool ( DataStack & ) >	fStackOp;

	public:

		GenericStackOp( Base & f, auto u_op ) : TWord< Base >( f ), fStackOp( u_op ) {}

	public:

		void operator () ( void ) override
		{
			if( fStackOp( GetDataStack() ) == false )
				throw ForthError( "stack overflow" );
		}

	};



	// A minimalistic and (probably) the fastest access to the stack operations
	// Requires the lambda F but with an empty caption
	template < typename Base, auto F >
	requires  valid_forth_env< Base >		// it is sufficient to have this constraint in one word from the hierarchy
	class ExGenericStackOp : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;

	public:

		ExGenericStackOp( Base & f ) : TWord< Base >( f ) {}

	public:

		void operator () ( void ) override
		{
			if( F( GetDataStack() ) == false )
				throw ForthError( "stack overflow" );
		}

	};




	// StackOp is a suite of classes for all types of data stack operations,
	// such as +, -, etc.
	// There are all variants of the input / output parameters, as follows:
	// 0, 1, 2 input arguments
	// 0 or 1 return type (void or other)
	
	// Just a starter with a variadic template
	template < typename...  >
	class StackOp 
	{
		public:
			StackOp() = delete;
	};


	// 0 input arg specialization which is a valid Forth's word
	// 2 variants of the return type:
	// - void - no stack operation
	// - other - a result is cast and pushed onto the stack
	template < typename Base, typename RetType >
	class StackOp< Base, RetType > : public TWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;


		std::function< RetType () >	fOp;

	public:

		StackOp( Base & f, auto u_op ) : TWord< Base >( f ), fOp( u_op ) {}

	public:

		void operator () ( void ) override
		{
			if constexpr ( std::is_same< RetType, void >::value )
				fOp();														// just a call 0 : 0
			else
				GetDataStack().Push( BlindValueReInterpretation< CellType >( fOp() ) );	// don't pop the stack, call fOp with no argument, then push the result
		}
	};



	// 1 input arg specialization
	// 2 variants of the return type:
	// - void - no stack operation
	// - other - a result is cast and pushed onto the stack
	template < typename Base, typename RetType, typename Arg_x >
	class StackOp< Base, RetType, Arg_x > : public TWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;


		std::function< RetType ( Arg_x ) >	fOp;

	public:

		StackOp( Base & f, auto u_op ) : TWord< Base >( f ), fOp( u_op ) {}

	public:

		void operator () ( void ) override
		{
			auto & ds = GetDataStack();

			if( typename DataStack::value_type x {}; ds.Pop( x ) )

				if constexpr ( std::is_same< RetType, void >::value )
					fOp( BlindValueReInterpretation< Arg_x >( x ) );	// just a call
				else
					ds.Push( BlindValueReInterpretation< CellType >( fOp( BlindValueReInterpretation< Arg_x >( x ) ) ) );	// call and push the result

			else
				throw ForthError( "unexpectedly empty stack" );
		}
	};


	// 2 input arg specialization
	// 2 variants of the return type:
	// - void - no stack operation
	// - other - a result is cast and pushed onto the stack
	template < typename Base, typename RetType, typename Arg_x, typename Arg_y >
	class StackOp< Base, RetType, Arg_x, Arg_y > : public TWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;


		std::function< RetType ( Arg_x, Arg_y ) >	fOp;

	public:

		StackOp( Base & f, auto u_op ) : TWord< Base >( f ), fOp( u_op ) {}

	public:

		void operator () ( void ) override
		{
			auto & ds = GetDataStack();

			if( typename DataStack::value_type x {}, y {}; ds.Pop( y ) && ds.Pop( x ) )

				if constexpr ( std::is_same< RetType, void >::value )
					fOp( BlindValueReInterpretation< Arg_x >( x ), BlindValueReInterpretation< Arg_y >( y ) );		// only a call
				else
					ds.Push( BlindValueReInterpretation< CellType >( fOp( BlindValueReInterpretation< Arg_x >( x ), BlindValueReInterpretation< Arg_y >( y ) ) ) );	// call & push the result
			else
				throw ForthError( "unexpectedly empty stack" );
		}
	};







	// Print the entire stack not changing its content - for diagnostic
	template < typename Base, typename DispType >
	class Stack_Dump : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;

		std::ostream & fOutStream;

		Name fSeparator;
		Name fEndMark;

	public:

		Stack_Dump( Base & f, std::ostream & o, Name sep = Name( 1, kSpace ), Name endMark = kCR ) 
			: TWord< Base >( f ), fOutStream( o ), fSeparator( sep ), fEndMark( endMark ) {}

	public:

		void operator () ( void ) override
		{
			const auto ds { GetDataStack().data() };
			const auto base { GetForth().ReadTheBase() };
			fOutStream << std::setbase( base ) << ( base == EIntCompBase::kDec ? std::noshowbase : std::showbase );
			std::transform( ds, ds + GetDataStack().size(),	std::ostream_iterator< DispType >( fOutStream, fSeparator.c_str() ), 
							[] ( const auto & v ) { return BlindValueReInterpretation< DispType >( v ); } );
			fOutStream << fEndMark;
		}

	};






	// For values of the simple types, such as int, float, double, etc.
	template < typename Base, typename VType >
	class TValFor : public TWord< Base >
	{
	public:

		using value_type = VType;

	protected:

		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;

		value_type	fData {};

	public:

		TValFor( Base & f, value_type v = value_type() ) : TWord< Base >( f ), fData( v ) {}

	public:

		value_type	GetVal( void ) const { return fData; }
		void		SetVal( value_type v ) { fData = v; }


	public:

		// Push on the data stack
		void operator () ( void ) override	
		{
			assert( sizeof( value_type ) <= sizeof( typename DataStack::value_type ) );
			if constexpr ( std::is_same< value_type, Name >::value )
				GetDataStack().Push( BlindValueReInterpretation< CellType >( fData.data() ) ), GetDataStack().Push( BlindValueReInterpretation< CellType >( fData.size() ) );	// addr n
			else
				GetDataStack().Push( BlindValueReInterpretation< CellType >( fData ) );
		}

	};





	// Specializations for some known value types

	template < typename Base >
	using IntValWord = TValFor< Base, SignedIntType >;


	template < typename Base >
	using DblValWord = TValFor< Base, FloatType >;


	template < typename Base >
	using CellValWord = TValFor< Base, CellType >;


	template < typename Base >
	using CharValWord = TValFor< Base, Char >;	
	







	// Pop and print top of the data stack
	template < typename Base, typename DispType >
	class Dot : public TWord< Base >
	{
	protected:

		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;

	protected:

		std::ostream & fOutStream;


	public:

		Dot( Base & f, std::ostream & o ) : TWord< Base >( f ), fOutStream( o ) {}

	public:

		void operator () ( void ) override
		{
			if( typename DataStack::value_type t {}; GetDataStack().Pop( t ) )
			{
				const auto base { GetForth().ReadTheBase() };
				fOutStream << std::setbase( base ) << ( base == EIntCompBase::kDec ? std::noshowbase : std::showbase ) << BlindValueReInterpretation< DispType >( t );
			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );
			}
		}

	};


	// Peek and print top of the data stack
	template < typename Base, typename DispType >
	class Dot_S : public Dot< Base, DispType >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;

		using Dot< Base, DispType >::fOutStream;

	public:

		Dot_S( Base & f, std::ostream & o ) : Dot< Base, DispType >( f, o ) {}

	public:

		void operator () ( void ) override
		{
			if( typename DataStack::value_type t {}; GetDataStack().Peek( t ) )
			{
				const auto base { GetForth().ReadTheBase() };
				fOutStream << std::setbase( base ) << ( base == EIntCompBase::kDec ? std::noshowbase : std::showbase ) << BlindValueReInterpretation< DispType >( t );
			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );
			}
		}

	};



	// Just display the contained text
	template < typename Base >
	class DotQuote : public TWord< Base >
	{
		std::ostream &	fOutStream;
		Name			fText;

	public:

		DotQuote( Base & f, std::ostream & o, Name s ) : TWord< Base >( f ), fOutStream( o ), fText( s ) {}

	public:

		void operator () ( void ) override
		{
			fOutStream << fText;
		}

	};




	// A suite for: ,"	S"	C"	."
	//
	template < typename Base >
	class QuoteSuite : public TWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;


		std::function< bool ( const Name &, DataStack & ) >	fQuoteOp;

		Name		fText;

	public:

		QuoteSuite( Base & f, Name s, auto u_op ) : TWord< Base >( f ), fText( s ), fQuoteOp( u_op ) {}

	public:

		void operator () ( void ) override
		{
			if( fQuoteOp( fText, GetDataStack() ) == false )
				throw ForthError( "string processing" );
		}

	};







	// Similar to TValFor but used to store an array
	// Provides: allocate, access, clear, etc.
	template < typename Base, typename VType >
	class TDataContainerFor : public TWord< Base >
	{

		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;

		using ContainerType = std::vector< VType >;

		ContainerType	fContainer {};

	public:

		using value_type	= typename ContainerType::value_type;
		using size_type		= typename ContainerType::size_type;

		TDataContainerFor( Base & f, const size_type init_alloc_size = size_type() ) : TWord< Base >( f ), fContainer( init_alloc_size )
		{
			static_assert( sizeof( value_type ) <= sizeof( typename DataStack::value_type ) );
			assert( fContainer.size() == init_alloc_size );
		}

	public:


		ContainerType & GetContainer( void ) { return fContainer; }


	public:

		// Push the address of the data buffer onto the data stack
		void operator () ( void ) override	
		{
			assert( fContainer.size() > 0 );
			GetDataStack().Push( (typename DataStack::value_type) fContainer.data() );
		}

	};


	// Partial specialization
	template < typename Base >
	using RawByteArray = TDataContainerFor< Base, RawByte >;




}	// The end of the BCForth namespace








