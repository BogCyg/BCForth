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
#include <cstring>
#include <algorithm>




namespace BCForth
{


	// --------------------------------------
	class StringModule : public TForthModule
	{

		using StDatType = TForthCompiler::DataStack::value_type;

	public:

		// Call to upload new words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{
			MemoryOperations( forth_comp );
			StringOperations( forth_comp );
		}


	private:


		void MemoryOperations( TForthCompiler & forth_comp )
		{
		
			forth_comp.InsertWord_2_Dict( "FILL",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[] ( auto & ds )	{	StDatType addr, u, c;
										return ds.Pop( c ) && ds.Pop( u ) && ds.Pop( addr ) ? std::memset( (void*)addr, (int)c, u ), true : false;
									}	), " addr u char -- " );		
		



			DirectTextModule( { ": ERASE ( addr u -- ) 0 FILL ;" } )( forth_comp );
			DirectTextModule( { ": BLANK ( addr u -- ) BL FILL ;" } )( forth_comp );


			forth_comp.InsertWord_2_Dict( "MOVE",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[] ( auto & ds )	{	StDatType addr1, addr2, u;
										return ds.Pop( u ) && ds.Pop( addr2 ) && ds.Pop( addr1 ) ? std::memmove( (void*)addr2, (void*)addr1, u ), true : false;
									}	), " addr1 addr2 u (copy u from addr1 to addr2) -- " );	



			forth_comp.InsertWord_2_Dict( "DUMP",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[ & forth_comp ] ( auto & ds )	{	
													StDatType addr, u;
													if( ! ( ds.Pop( u ) && ds.Pop( addr ) ) ) return false;

													auto & stream { forth_comp.GetOutStream() };
													auto print_fun = forth_comp.ReadTheBase() == EIntCompBase::kHex ?	// in hex print on 2-wide-fields; for dec use 3-wide-fields
														std::function{ [ & stream ] ( const RawByte b ) { stream << std::setfill( '0' ) << std::setw( 2 ) << std::uppercase << std::hex << +b << kSpace; } }
																	:
														std::function{ [ & stream ] ( const RawByte b ) { stream << std::setfill( '0' ) << std::setw( 3 ) << std::dec << +b << kSpace; } }
																	;

													RawByte * ptr = reinterpret_cast< RawByte * >( addr );
													const StDatType kSplitRow { 16 };
													Name verbose;
													for( StDatType i {}; i < u; ++ i )
													{
														RawByte b { ptr[ i ] };
														verbose += std::isprint( b ) ? b : '.';
														print_fun( b );

														if( ( i + 1 ) % kSplitRow == 0 )
														{
															stream << kTab << kTab << verbose << kCR;
															verbose.clear();
														}
													}

													return true;

												}	), " addr u -- " );


		
		}



		void StringOperations( TForthCompiler & forth_comp )
		{

			// Compare strings (memory byte-by-byte) beginning at addr1 and addr2 and to the min(u1,u2)
			// Returns 0 if identical; else -1 if first non matching char in the first string has lesser value than in the second; +1 otherwise
			forth_comp.InsertWord_2_Dict( "COMPARE",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[] ( auto & ds )	{	StDatType addr1, addr2, u1, u2;			
										return ds.Pop( u2 ) && ds.Pop( addr2 ) && ds.Pop( u1 ) && ds.Pop( addr1 ) ? ds.Push( std::memcmp( reinterpret_cast< void * >( addr1 ), reinterpret_cast< void * >( addr2 ), std::min( u1, u2 ) ) ), true : false;
									}	
										), " addr1 u1 addr2 u2 (compare min(u1,u2) ) -- n " );	



			// Search in the string at (addr1,u1) for occurence of a string at (addr2,u2)
			// Returns, if found: addr3 of the first matching char, num of chars in the first string till the end, true
			// Returns, if not found: addr3 == addr1, u1, false
			forth_comp.InsertWord_2_Dict( "SEARCH",	std::make_unique< GenericStackOp< TForth > >( forth_comp, 
				[] ( auto & ds )	{	StDatType addr1, addr2, u1, u2;			
										if( ! ( ds.Pop( u2 ) && ds.Pop( addr2 ) && ds.Pop( u1 ) && ds.Pop( addr1 ) ) ) return false;
										if( auto it = std::search( (Char*)addr1, (Char*)addr1 + u1, (Char*)addr2, (Char*)addr2 + u2 ); it != (Char*)addr1 + u1 )
											return ds.Push( reinterpret_cast< StDatType >( &*it ) ) && ds.Push( u1 - ( reinterpret_cast< StDatType >( &*it ) - addr1 ) ) && ds.Push( kBoolTrue ), true;
										else
											return ds.Push( addr1 ) && ds.Push( u1 ) && ds.Push( kBoolFalse ), true;

									}	
										), " addr1 u1 addr2 u2 -- addr3 u3 flag " );	

		
		

			// Get at most maxLen chars and put them into the memory at addr. Continues until a CR is reached
			// or maxLen has been collected. Examples:
			// 
			//		: GET-STRING ( -- reaLen )   PAD 12 ACCEPT ;
			//		: SHOW-STRING ( -- )   GET-STRING  PAD SWAP TYPE ;
			//
			// If more than maxLen chars is entered, such as 15 in the above example, then the remaining as well as the ending CR are IGNORED.
			// Otherwise those left chars (e.g. 3 in the example) would be read and interpreted by the BCForth tokenizer, which is probably not desirable.
			// 
			// To read char-by-char, even with CR, use the KEY word
			//
			forth_comp.InsertWord_2_Dict( "ACCEPT",	std::make_unique< StackOp< TForth, CellType, Char *, CellType > >( forth_comp, 

				[ & forth_comp ] ( const auto addr, const auto maxLen ) 
				{ 
					std::cin.get( addr, maxLen + 1, kCR[ 0 ] );	// Reads at most count-1 == maxLen chars and stores them in the buffer at addr, until '\n' is found.
					auto reaLen { std::cin.gcount() };			// '\n' is not extracted from the input sequence
					std::cin.ignore( std::numeric_limits< std::streamsize >::max(), kCR[ 0 ] );		// get rid of ALL chars in this line AND '\n'
					return reaLen;
				} 
			
			), " addr maxLen -- reaLen " );



		}






	};



}



