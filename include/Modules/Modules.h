// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once



#include "ForthCompiler.h"
#include "Tokenizer.h"
#include <filesystem>





namespace BCForth
{

	namespace fs = std::filesystem;


	// Forth modules to upload special sets of words
	// (kind of a decorator DP to the Forth's compiler)
	class TForthModule
	{

	public:

		virtual ~TForthModule() = default;


	public:

		// Call to upload new words to the forth_comp
		virtual void operator () ( TForthCompiler & forth_comp ) = 0;

	};



	// ----------------------------------------
	// This one for direct text words
	class DirectTextModule : public TForthModule
	{

		Names	fWords;

	public:

		DirectTextModule( void ) = default;
		DirectTextModule( const Names & defNames ) : fWords( defNames ) {}

	public:

		void AddWord( Name n ) { fWords.emplace_back( n ); }

		auto & GetWords( void ) { return fWords; }

	public:

		// Call to upload new words from a text file to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{
			for( const auto & word : fWords )
			{ 
				std::istringstream ss( word );
				forth_comp( TForthReader()( ss ) );	
			}
		}

	};


	class AuxTextModule : public DirectTextModule
	{


	public:

		AuxTextModule( void ) : DirectTextModule(	{

														": 3DUP 	( a b c -- a b c a b c )	DUP 2OVER ROT		;	\\ a b c a b c ", 
														": 3DROP	( a b c -- )				2DROP DROP ;", 

														R"(: FWITHIN ( f1 f2 f3 -- f_true if f2 <= f1 < f3 )	-ROT					\\ f3 f1 f2				\n
																												OVER					\\ f3 f1 f2 f1			\n
																												F<=						\\ f3 f1 c1				\n
																												-ROT					\\ c1 f3 f1				\n
																												F> AND					\\ c1 and c2			\n
															;)"

													}

												) {}
		



	};






	// ----------------------------------------
	// Read and upload Forth's words from a file
	class FileForthModule : public TForthModule
	{

		const fs::path	fFModulePath;

	public:

		FileForthModule( fs::path p ) : fFModulePath( p ) {}


	public:

		// Call to upload new words from a text file to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{
			if( std::ifstream fs( fFModulePath ); fs )
				for( TForthReader fileReader; fs; forth_comp( fileReader( fs ) ) ) 
					;	
		}

	};


	// --------------------------------------
	// Extra words for the data stack and
	// for the return stack.
	class AuxStackWords : public TForthModule
	{



	public:

		// Call to upload new words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{

			forth_comp.InsertWord_2_Dict( "2OVER",	std::make_unique< ExGenericStackOp< TForth,

				[] ( auto & ds )	{	const auto kReqElems { 4 };	// at least 4 data on the stack
										if( ds.size() < kReqElems ) return false;
										const auto d0 { ds.data()[ ds.size() - kReqElems ] };
										const auto d1 { ds.data()[ ds.size() - ( kReqElems - 1 ) ] };
										return ds.Push( d0 ) && ds.Push( d1 );
									}	
			
			
					> >( forth_comp ), " x y p q -- x y p q x y " );



			forth_comp.InsertWord_2_Dict( "2SWAP",	std::make_unique< ExGenericStackOp< TForth,

				[] ( auto & ds )	{	const auto kReqElems { 4 };	// at least 4 data on the stack
										if( ds.size() < kReqElems ) return false;
										std::swap( ds.data()[ ds.size() - kReqElems ],			ds.data()[ ds.size() - ( kReqElems - 2 ) ] );
										std::swap( ds.data()[ ds.size() - ( kReqElems - 1 ) ],	ds.data()[ ds.size() - ( kReqElems - 3 ) ] );
										return true;
									}


					> >( forth_comp ), " x y p q -- p q x y " );



			forth_comp.InsertWord_2_Dict( "2ROT",	std::make_unique< ExGenericStackOp< TForth,

				[] ( auto & ds )	{	const auto kReqElems { 6 };	// at least 6 data on the stack
										if( ds.size() < kReqElems ) return false;
										auto rev_offset = [ & ds ]( auto i ){ return ds.data() + ds.size() + i; };		// a lambda in a lambda is better than a preproc macro
										const auto x = * rev_offset( - kReqElems );										// save these two 
										const auto y = * rev_offset( - kReqElems + 1 );									// since they will be obliterated
										* rev_offset( - kReqElems     ) = * rev_offset( - kReqElems + 2 );
										* rev_offset( - kReqElems + 1 ) = * rev_offset( - kReqElems + 3 );
										* rev_offset( - kReqElems + 2 ) = * rev_offset( - kReqElems + 4 );
										* rev_offset( - kReqElems + 3 ) = * rev_offset( - kReqElems + 5 );
										* rev_offset( - kReqElems + 4 ) = x;
										* rev_offset( - kReqElems + 5 ) = y;
										return true;
									}


			> >( forth_comp ), " x y p q s t -- p q s t x y " );

			// Call this to remove all data from the data stack
			forth_comp.InsertWord_2_Dict( "SCLEAR",	std::make_unique< ExGenericStackOp< TForth, 
																						[] ( auto & ds )	{	ds.clear(); return true; } 
																	> >( forth_comp ), " a ... z --  " );


			// Ret stack operations
			auto & retStack = forth_comp.GetRetStack();	


			// top data stack ==> ret stack
			forth_comp.InsertWord_2_Dict( ">R",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & retStack ] ( auto & ds )	{	TForthCompiler::DataStack::value_type x;
													return ds.Pop( x ) ? retStack.Push( x ) : false;
												}	), " x -- | R: -- x " );


			// top ret stack ==> data stack
			forth_comp.InsertWord_2_Dict( "R>",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & retStack ] ( auto & ds )	{	TForthCompiler::DataStack::value_type x;
													return retStack.Pop( x ) ? ds.Push( x ) : false;
												}	), " -- x | R: x -- " );


			// a copy of the top ret stack ==> data stack
			forth_comp.InsertWord_2_Dict( "R@",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & retStack ] ( auto & ds )	{	TForthCompiler::DataStack::value_type x;
													return retStack.Peek( x ) ? ds.Push( x ) : false;
												}	), " -- x | R: x -- x " );





			// 2 top data stack ==> 2 ret stack
			forth_comp.InsertWord_2_Dict( "2>R",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & retStack ] ( auto & ds )	{	TForthCompiler::DataStack::value_type x, y;
													return ds.Pop( y ) && ds.Pop( x ) ? retStack.Push( x ) && retStack.Push( y ) : false;
												}	), " x y -- | R: -- x y " );


			// 2 top ret stack ==> 2 data stack
			forth_comp.InsertWord_2_Dict( "2R>",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & retStack ] ( auto & ds )	{	TForthCompiler::DataStack::value_type x, y;
													return retStack.Pop( y ) && retStack.Pop( x ) ? ds.Push( x ) && ds.Push( y ) : false;
												}	), " -- x y | R: x y -- " );


		}

	};



}


