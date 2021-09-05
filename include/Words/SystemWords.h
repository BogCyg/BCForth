// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once



#include "StructWords.h"






namespace BCForth
{




	// ------------------------
	// System words


	// ' <name>
	template < typename Base >
	class Tick : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::WordPtr;
		using TWord< Base >::GetForth;


		const Name & fValueName;	// a name of the VALUE word 

	public:

		Tick( Base & f, const Name & value_name ) : TWord< Base >( f ), fValueName( value_name ) {}

	public:

		// Find a name and put its "token" onto the stack (i.e. its node ptr TWord< TForth > *, so it can be executed)
		void operator () ( void ) override
		{
			if( auto word_entry = GetForth().GetWordEntry( fValueName ) )
				GetDataStack().Push( reinterpret_cast< CellType >( ( * word_entry )->fWordUP.get()  ) );				
			else
				throw ForthError( "This word has not been defined" );
		}

	};


	// EXECUTE
	template < typename Base >
	class Execute : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using DataStack = typename Base::DataStack;

	public:

		Execute( Base & f ) : TWord< Base >( f ) {}

	public:

		void operator () ( void ) override
		{
			// Take an address from the stack and execute the word
			if( typename DataStack::value_type word_addr {}; GetDataStack().Pop( word_addr ) )
				( * reinterpret_cast< TWord< Base > * >( word_addr ) )();
			else
				throw ForthError( "unexpectedly empty stack" );
		}

	};





	// Used to change content of the VALUE, e.g.
	// 50 VALUE ACCELERATOR
	// 100 TO ACCELERATOR
	template < typename Base >
	class To : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::WordPtr;
		using TWord< Base >::GetForth;

		using DataStack = typename Base::DataStack;


		const Name & fValueName;	// a name of the VALUE word 



	public:

		To( Base & f, const Name & value_name ) : TWord< Base >( f ), fValueName( value_name ) {}

	public:


		void operator () ( void ) override
		{
			if( auto word_entry = GetForth().GetWordEntry( fValueName ) )
			{

				if( auto * compo_wrd = dynamic_cast< CompoWord< Base > * >(  ( * word_entry )->fWordUP.get() ); compo_wrd && compo_wrd->GetWordsVec().size() > 0 )	// Ok, the word is found but check if this is a proper node

					if( auto * val_array = dynamic_cast< RawByteArray< Base > * >(  compo_wrd->GetWordsVec()[ 0 ] ) )	// Access the array in the compo word				

						if( typename DataStack::value_type val {}; GetDataStack().Pop( val ) )		// Ok, try to pop the stack 
						{							
							assert( val_array->GetContainer().size() == sizeof( CellType ) );
							* reinterpret_cast< typename DataStack::value_type * >( val_array->GetContainer().data() ) = val;
							return;
						}
						else
						{
							throw ForthError( "unexpectedly empty stack" );
						}


				throw ForthError( "This word cannot be used in this context" );

			}
			else
			{
				throw ForthError( "This VALUE has not been defined" );
			}

		}


	};




	// Used in context like this 
	// CREATE TENS  1 , 10 , 100 , 1000 , 10000 ,
	// each value is set into a cell (not a byte)
	// 
	// VAL is a type of a value to push, e.g.
	// it can be as above, or as in
	// CREATE TEST 123 C, ALIGN 1234 ,
	//
	template < typename Base, typename VAL >
	class Comma : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;


		using DataStack = typename Base::DataStack;


		using VAL_TYPE = VAL;

	public:

		Comma( Base & f ) : TWord< Base >( f ) {}

	public:

		void operator () ( void ) override
		{

			// CellVal node is on the node repo, whereas value to set is on the data stack - go for them
			if( typename DataStack::value_type val {}; GetDataStack().Pop( val ) )
			{

				if( const auto & nrepo { GetForth().GetNodeRepo() }; nrepo.size() > 0 )

					if( auto * cell_val_node = dynamic_cast< RawByteArray< Base > * >( nrepo[ nrepo.size() - 1 ].get() ) )
					{
						PushValTo( cell_val_node->GetContainer(), static_cast< VAL_TYPE >( val ) );
						return;
					}


				throw ForthError( "no preceding CREATE to , (comma)" );

			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );	
			}

		}


	};


	template < typename Base >
	class CommaQuote : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;


		using DataStack = typename Base::DataStack;


		const Name & fStr;

	public:

		CommaQuote( Base & f, const Name & str ) : TWord< Base >( f ), fStr( str ) {}

	public:

		void operator () ( void ) override
		{
			if( const auto & nrepo { GetForth().GetNodeRepo() }; nrepo.size() > 0 )

				if( auto * cell_val_node = dynamic_cast< RawByteArray< Base > * >( nrepo[ nrepo.size() - 1 ].get() ) )
				{
						
					auto str_len { fStr.length() };
					assert( str_len <= 255 );
					PushValTo( cell_val_node->GetContainer(), static_cast< RawByte >( str_len ) );
					std::for_each( fStr.begin(), fStr.end(), [ & arr = cell_val_node->GetContainer() ] ( const auto c ) { PushValTo( arr, static_cast< RawByte >( c ) ); } );

					return;
				}


			throw ForthError( "no preceding CREATE to ,\" (comma-quote)" );
		}


	};




	// Allocate n bytes for the array - however we use the cell-type array
	// Used in context like this 
	// 
	// CREATE DATA  100 ALLOT
	// DATA  100 ERASE
	// 
	// CREATE CELL-DATA  100 CELLS ALLOT
	//
	template < typename Base >
	class Allot : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using TWord< Base >::WordPtr;
		using TWord< Base >::GetForth;


		using DataStack = typename Base::DataStack;


	public:

		Allot( Base & f ) : TWord< Base >( f ) {}

	public:


		void operator () ( void ) override
		{	
			// CellVal node is on the node repo, whereas num of bytes to allocate is on the data stack - go for them
			if( typename DataStack::value_type size_2_alloc_in_bytes {}; GetDataStack().Pop( size_2_alloc_in_bytes ) )
			{

				if( const auto & nrepo { GetForth().GetNodeRepo() }; nrepo.size() > 0 )

					if( auto * cell_val_node = dynamic_cast< RawByteArray< Base > * >( nrepo[ nrepo.size() - 1 ].get() ) )
					{
						cell_val_node->GetContainer().resize( cell_val_node->GetContainer().size() + size_2_alloc_in_bytes );	// append the new size, i.e. keep the previous allocations
						return;																									// this makes possible interaction with prior ,
					}


				throw ForthError( "no preceding CREATE to ALLOT" );

			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );	
			}


		}


	};





	// CREATE
	// It will be compiled into the creat words with DOES>
	// It will execute during definition of the instance word
	//
	// CREATE DATA  100 ALLOT
	// DATA  100 ERASE
	// 
	// CREATE CELL-DATA  100 CELLS ALLOT
	//
	template < typename Base >
	class Create : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;
		using typename TWord< Base >::WordPtr;
		using WordUP = typename Base::WordUP;
		using TWord< Base >::GetForth;

	public:

		Create( Base & f ) : TWord< Base >( f ) {}

	public:


		void operator () ( void ) override
		{
			// When Create executes the RawByteArray node is created. It needs to be associated with the subsequent
			// word whose name is next in the input stream after the defining word (i.e. the one containing this CREATE)
			GetForth().Insert_2_NodeRepo( std::make_unique< RawByteArray< Base > >( GetForth() ) );
		}


	};



	// Just display the contained text
	template < typename Base >
	class Abort : public TWord< Base >
	{
	protected:

		using TWord< Base >::GetDataStack;
		using TWord< Base >::GetForth;

		Name		fText;

	public:

		Abort( Base & f, Name s = "" ) : TWord< Base >( f ), fText( s ) {}

	public:

		void operator () ( void ) override
		{
			// The base ABORT operates as follows:
			// (1) Clear the stacks
			// (2) Unconditionally terminate execution

			GetDataStack().clear();
			GetForth().GetRetStack().clear();

			throw fText;
		}

	};


	template < typename Base >
	class AbortQuote : public Abort< Base >
	{
	protected:

		using MyBase = Abort< Base >;

		using DS = typename Base::DataStack;
		using MyBase::GetDataStack;
		using MyBase::GetForth;


	public:

		AbortQuote( Base & f, Name s = "" ) : MyBase( f, s ) {}

	public:

		void operator () ( void ) override
		{
			if( typename DS::value_type word_addr {}; GetDataStack().Pop( word_addr ) )
			{
				if( word_addr )					// if flag is true (non-zero)
					MyBase::operator () ();		// call the base ABORT to clear the stacks and throw
			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );
			}
		}

	};



	// POSTPONE <name>
	// Usually in the context like this:
	// : XXXX DUP POSTPONE WORD_A DROP POSTPONE WORD_B ; IMMEDIATE
	// ...
	// : Scnd BLA BLA   XXXX   BLA ;
	// Because XXXX is immediate it will be immediately executed during compilation
	// except its two postponed words WORD_A and WORD_B. Hence, the above will evaluate to something like:
	// : Scnd BLA BLA   WORD_A  WORD_B  BLA ;
	//
	template < typename Base >
	class Postpone : public TWord< Base >
	{
		using typename TWord< Base >::WordPtr;
		using TWord< Base >::GetForth;

		const WordPtr	fWordPtr;	// this is the "postponed" word, i.e. it had been encountered during compilation
									// but was preceded by POSTPONE - because of this it hasn't been executed but 
									// compiled into the word being compiled

	public:

		Postpone( Base & f, const WordPtr w_ptr ) : TWord< Base >( f ), fWordPtr( w_ptr ) {}

	public:

		// Do an exception ** when compiling an immediate word ** with some of its sub-words
		// indicated as POSTPONE, though. In such a case, these sub-words will be compiled-in,
		// rathern than being executed with the rest of the immediated word. 
		// However, if not in the "compiling immdediate" mode, then such a word will be simply executed.
		void operator () ( void ) override;

	};




}	// The end of the BCForth namespace




