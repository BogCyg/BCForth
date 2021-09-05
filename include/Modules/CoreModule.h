// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once




#include "Modules.h"





namespace BCForth
{



	// --------------------------------------
	// Extra words for the data stack and
	// for the return stack.
	class CoreEncodedWords : public TForthModule
	{

		using DS	= TForth::DataStack;
		using DS_VT = DS::value_type;


	public:

		// Call to upload new words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{


			forth_comp.InsertWord_2_Dict( ".",		std::make_unique< Dot< TForth, SignedIntType > >( forth_comp, forth_comp.GetOutStream() ), " x -- " );
			forth_comp.InsertWord_2_Dict( ".S",		std::make_unique< Dot_S< TForth, SignedIntType > >( forth_comp, forth_comp.GetOutStream() ), " x -- x " );


			forth_comp.InsertWord_2_Dict( ".SD",	std::make_unique< Stack_Dump< TForth, SignedIntType > >( forth_comp, forth_comp.GetOutStream(), Letter_2_Name( kSpace ) ), " x -- x ==> int stack dump " );
			forth_comp.InsertWord_2_Dict( ".SDU",	std::make_unique< Stack_Dump< TForth, CellType > >( forth_comp, forth_comp.GetOutStream(), Letter_2_Name( kSpace ) ), " x -- x ==> uint stack dump " );




			forth_comp.InsertWord_2_Dict( "DROP",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Drop();  }	 > >( forth_comp ), " x -- " );
			forth_comp.InsertWord_2_Dict( "DUP",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Dup();  }	 > >( forth_comp ), " x -- x x " );
			forth_comp.InsertWord_2_Dict( "SWAP",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Swap();  }	 > >( forth_comp ), " x y -- y x " );
			forth_comp.InsertWord_2_Dict( "OVER",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Over();  }	 > >( forth_comp ), " x y -- x y x " );
			forth_comp.InsertWord_2_Dict( "ROT",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Rot();  }	 > >( forth_comp ), " x y z -- y z x " );


			forth_comp.InsertWord_2_Dict( "+",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Plus< SignedIntType >();  }	 > >( forth_comp ), " x y -- x+y " );
			forth_comp.InsertWord_2_Dict( "-",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Minus< SignedIntType >();  }	 > >( forth_comp ), " x y -- x-y " );
			forth_comp.InsertWord_2_Dict( "*",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Mult< SignedIntType >();  }	 > >( forth_comp ), " x y -- x*y " );
			forth_comp.InsertWord_2_Dict( "/",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Div< SignedIntType >();  }	 > >( forth_comp ), " x y -- x/y " );
			forth_comp.InsertWord_2_Dict( "MOD",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Mod< SignedIntType >();  }	 > >( forth_comp ), " x y -- x/y " );



			using UnarySignOp = StackOp< TForth, SignedIntType, SignedIntType >;
			using BinSignOp = StackOp< TForth, SignedIntType, SignedIntType, SignedIntType >;

			forth_comp.InsertWord_2_Dict( "NEG",	std::make_unique< UnarySignOp >( forth_comp, [] ( const auto x ) { return -x; } ), " x -- -x " );


			forth_comp.InsertWord_2_Dict( "AND",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.And();  }	 > >( forth_comp ), " x y -- x_AND_y " );
			forth_comp.InsertWord_2_Dict( "OR",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Or();  }	 > >( forth_comp ), " x y -- x_OR_y " );
			forth_comp.InsertWord_2_Dict( "XOR",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Xor();  }	 > >( forth_comp ), " x y -- x_XOR_y " );
			forth_comp.InsertWord_2_Dict( "~",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Neg();  }	 > >( forth_comp ), " x -- BIT_INV(x) " );



			forth_comp.InsertWord_2_Dict( "CR",		std::make_unique< DotQuote< TForth > >( forth_comp, forth_comp.GetOutStream(), kCR ) );
			forth_comp.InsertWord_2_Dict( "TAB",	std::make_unique< DotQuote< TForth > >( forth_comp, forth_comp.GetOutStream(), Letter_2_Name( kTab ) ) );
			forth_comp.InsertWord_2_Dict( "SPACE",	std::make_unique< DotQuote< TForth > >( forth_comp, forth_comp.GetOutStream(), Letter_2_Name( kSpace ) ) );


			forth_comp.InsertWord_2_Dict( "CREATE",	std::make_unique< Create< TForth > >( forth_comp ), " -- " );
			forth_comp.InsertWord_2_Dict( "ALLOT",	std::make_unique< Allot< TForth > >( forth_comp ), " n_bytes -- " );
			forth_comp.InsertWord_2_Dict( ",",		std::make_unique< Comma< TForth, CellType > >( forth_comp ), " x -- " );
			forth_comp.InsertWord_2_Dict( "C,",		std::make_unique< Comma< TForth, RawByte > >( forth_comp ), " c -- " );

			forth_comp.InsertWord_2_Dict( "EXECUTE",std::make_unique< Execute< TForth > >( forth_comp ), " ex_token -- ? " );

			forth_comp.InsertWord_2_Dict( "PAD",	std::make_unique< RawByteArray< TForth > >( forth_comp, k_PAD_Size ), " -- PAD_addr " );



			// Emit and key
			forth_comp.InsertWord_2_Dict( "KEY",	std::make_unique< StackOp< TForth, Char > >( forth_comp, [] () { Char c {}; std::cin.get( c ); return c; } ), " -- c " );
			forth_comp.InsertWord_2_Dict( "EMIT",	std::make_unique< StackOp< TForth, void, Char > >( forth_comp, [ & forth_comp ] ( const auto c ) { forth_comp.GetOutStream() << c; } ), " c -- " );
			forth_comp.InsertWord_2_Dict( "TYPE",	std::make_unique< StackOp< TForth, void, Char *, CellType > >( forth_comp, [ & forth_comp ] ( const auto addr, const auto len ) { for( auto i{0}; i < len; ++ i ) forth_comp.GetOutStream() << addr[ i ]; } ), " addr len -- " );



			// Comparisons
			forth_comp.InsertWord_2_Dict( "=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template EQ< SignedIntType >();  } > >( forth_comp ), " x y -- x<y " );
			forth_comp.InsertWord_2_Dict( "<>",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template NE< SignedIntType >();  } > >( forth_comp ), " x y -- x<=y " );
			forth_comp.InsertWord_2_Dict( "<",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LT< SignedIntType >();  } > >( forth_comp ), " x y -- x>y " );
			forth_comp.InsertWord_2_Dict( "<=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LE< SignedIntType >();  } > >( forth_comp ), " x y -- x>=y " );
			forth_comp.InsertWord_2_Dict( ">",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GT< SignedIntType >();  } > >( forth_comp ), " x y -- x=y " );
			forth_comp.InsertWord_2_Dict( ">=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GE< SignedIntType >();  } > >( forth_comp ), " x y -- x<>y " );





			forth_comp.InsertWord_2_Dict( "CELLS",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.Cells();  }						 > >( forth_comp ), " n -- 8*n " );
			forth_comp.InsertWord_2_Dict( "CELL+",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.CellPlus();  }						 > >( forth_comp ), " addr -- addr+8" );
			
			forth_comp.InsertWord_2_Dict( "@",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template ReadAt< CellType >();  }	 > >( forth_comp ), " addr -- [addr] " );
			forth_comp.InsertWord_2_Dict( "!",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template WriteAt< CellType >();  }	 > >( forth_comp ), " x addr -- " );

			forth_comp.InsertWord_2_Dict( "C@",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template ReadAt< Char >();  }	 > >( forth_comp ), " c_addr -- [c_addr] " );
			forth_comp.InsertWord_2_Dict( "C!",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template WriteAt< Char >();  }	 > >( forth_comp ), " c c_addr -- " );
			forth_comp.InsertWord_2_Dict( "C+!",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template UpdateAt< Char >();  } > >( forth_comp ), " c c_addr -- " );



			forth_comp.InsertWord_2_Dict( "1+",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template OnePlus< SignedIntType >();  } > >( forth_comp ), " x -- x+1 " );
			forth_comp.InsertWord_2_Dict( "1-",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template OneMinus< SignedIntType >();  } > >( forth_comp ), " x -- x-1 " );
			
			forth_comp.InsertWord_2_Dict( "2+",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template TwoPlus< SignedIntType >();  } > >( forth_comp ), " x -- x+2 " );
			forth_comp.InsertWord_2_Dict( "2-",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template TwoMinus< SignedIntType >();  } > >( forth_comp ), " x -- x-2 " );
			forth_comp.InsertWord_2_Dict( "2*",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template TwoTimes< SignedIntType >();  } > >( forth_comp ), " x -- x*2 " );
			
			forth_comp.InsertWord_2_Dict( "0=",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template EQ_0< SignedIntType >();  } > >( forth_comp ), " x -- x=0 " );
			forth_comp.InsertWord_2_Dict( "0<>",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template NE_0< SignedIntType >();  } > >( forth_comp ), " x -- x<>0 " );
			forth_comp.InsertWord_2_Dict( "0<",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LT_0< SignedIntType >();  } > >( forth_comp ), " x -- x<0 " );
			forth_comp.InsertWord_2_Dict( "0<=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LE_0< SignedIntType >();  } > >( forth_comp ), " x -- x<=0 " );
			forth_comp.InsertWord_2_Dict( "0>",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GT_0< SignedIntType >();  } > >( forth_comp ), " x -- x>0 " );
			forth_comp.InsertWord_2_Dict( "0>=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GE_0< SignedIntType >();  } > >( forth_comp ), " x -- x>=0 " );





			// Spec words
			// List all words already in the dictionary
			forth_comp.InsertWord_2_Dict( "WORDS",	std::make_unique< StackOp< TForth, void > >( forth_comp, 
				[ & forth_comp ] () 
			{ 
				std::vector< std::tuple< Name, Name, Name > >		name_comm_vec;
				for( const auto & [ n, w ] : forth_comp.GetWordDict() )
					name_comm_vec.push_back( std::make_tuple( n, w.fWordComment, w.fWordIsImmediate ? " [immediate]" : "" ) );
				std::sort( name_comm_vec.begin(), name_comm_vec.end() );
				std::for_each( name_comm_vec.begin(), name_comm_vec.end(), [ & forth_comp ] ( const auto & t ) { forth_comp.GetOutStream() << std::get<0>( t ) << "\t\t\t" << std::get<1>( t ) << "\t\t\t\t\t" << std::get<2>( t ) << std::endl; } );
			} ), " -- " );


			forth_comp.InsertWord_2_Dict( "ABORT",	std::make_unique< Abort< TForth > >( forth_comp, "ABORT called" ), " -- " );


			forth_comp.InsertWord_2_Dict( "LEAVE",	std::make_unique< LEAVE< TForth > >( forth_comp ), " -- " );

		}

	};




	class CoreDefinedWords : public DirectTextModule
	{
		  
		const Name & kBL			{ ": BL ( -- space_char ) " + std::to_string( kSpace ) + " ;" };	  
			  
		const Name & kFALSE_Const	{ std::to_string( kBoolFalse ) + " CONSTANT FALSE" };
		const Name & kTRUE_Const	{ std::to_string( kBoolTrue ) + " CONSTANT TRUE" };


	public:

		CoreDefinedWords( void )
		{
		
			GetWords() =
			{
					": 2DUP ( x y -- x y x y ) OVER OVER ;", 
					": ?DUP ( x -- 0 | x x ) DUP IF DUP THEN ;", 
					": 2DROP ( x y -- ) DROP DROP ;",
					": -ROT ( x y z -- z x y ) ROT ROT ;",


					": */ ( x y z -- (x*y)/z ) ROT ROT * SWAP / ;",


					": ? ( addr -- ) @ . ;",	// a word to query a variable

					": CHARS ( -- ) ;",			// just no-op
					": CHAR+ ( x -- x+1 ) 1+ ;",


					kBL,


					": MIN ( n1 n2 -- min{n1,n2} ) 2DUP < IF DROP ELSE SWAP DROP THEN ;",
					": MAX ( n1 n2 -- max{n1,n2} ) 2DUP > IF DROP ELSE SWAP DROP THEN ;",


					R"(: WITHIN ( n1 n2 n3 -- t if n2 <= n1 < n3 )	-ROT					\\ n3 n1 n2				\n
																	OVER					\\ n3 n1 n2 n1			\n
																	<=						\\ n3 n1 c1				\n
																	-ROT					\\ c1 n3 n1				\n
																	> AND					\\ c1 and c2			\n
					;)",


					": WITHIN? ( n1 n2 n3 -- t if n2 <= n1 <= n3 ) 1+ WITHIN ;",

					// String words
					": COUNT ( c-addr_1 -- c-addr_2 c-len ) DUP 1+ SWAP C@ ;",



					// Here go some creational words


					// e.g.
					// 31 CONSTANT DAYS_IN_MAY
					// DAYS_IN_MAY @ .
					R"(	: CONSTANT			( n -- | ) 
									CREATE					\\ at compile time, create new data \n
										,					\\ write in the value n \n
									DOES>	( -- n )		\\ what the child does, get data address \n
									@						\\ get address of data & push its value onto the stack \n
						; 
					)",




					// e.g.
					// 1024 VALUE NUM_BYTES
					// 2048 TO NUM_BYTES
					// NUM_BYTES @ .
					R"(	: VALUE				( n -- | ) 
									CREATE					\\ at compile time, create new data \n
										,					\\ write in the value n \n
									DOES>	( -- n )		\\ what the child does, get data address \n
									@						\\ get address of data & push its value onto the stack \n
						; 
					)",



					// e.g.
					// VARIABLE DATA
					// DATA C@
					R"(	: VARIABLE			( -- | ) 
									CREATE 1 CELLS ALLOT	\\ at compile create one cell data	\n
									DOES>	( -- addr )		\\ at run-time return a variable address \n
						; 
					)",




					// e.g.
					// 100 BUFFER:	DATA	/ create a buffer DATA of 100 bytes
					// 31 DATA !			/ write 31 to the first cell (8 bytes) of DATA
					// the same as
					// CREATE DATA 100 ALLOT
					R"(	: BUFFER:				( len_bytes -- | )		
									CREATE ALLOT						\\ at compile create a buffer of len_bytes	\n
									DOES>		( -- addr )				\\ at run-time return a buffer address	\n
						; 
					)",



					// Use as follows:
					// 100	ARRAY	VOLT
					// 230 2 VOLT !			\\ enter 230 to the 3rd cell of the array VOLT
					// 2 VOLT ?				\\ read and print 3rd cell of VOLT
					// Here we assume 0-based indexing but the first cell contains the max num of elements
					R"(	: ARRAY ( n -- | )		CREATE DUP , CELLS ALLOT					\\ allocate the number of cells AND the array buffer \n
								DOES> ( n -- a ) 
												SWAP OVER 
												@ OVER <= ABORT" Index out of range" 						

												1+ CELLS +			;						\\ 1+ to pass the first cell with the number of elements
					)",



					// This is a special word to allow CREATE in both, the interpreter
					// and the compiler modes, e.g. as in the above words ARRAY etc. or
					// during interpretation
					// 
					// CREATE TENS 1 , 10 , 100 , 1000 ,
					// CREATE TEXT ," The quics brown fox "
					//
					// In the above call to CREATE will be interceptedd and redirected
					// to the [CREATE] instead
					R"(	: [CREATE]				(  -- )		
								CREATE								\\ at compile create an (empty yet) buffer	\n
								DOES>			( -- addr )			\\ at run-time return a buffer address	\n
					; 
					)",



					// These must go AFTER the creational words CONSTANT and VARIABLE

					"VARIABLE BASE",		// define


					": HEX 16 BASE ! ;",			
					": DEC 10 BASE ! ;",		

					"DEC",					// turn decimal on entry


					kFALSE_Const,
					kTRUE_Const,


			};		
		
		
		}




	};



}


