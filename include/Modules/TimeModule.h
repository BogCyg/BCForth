// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once



#include <chrono>
#include "Modules.h"






namespace BCForth
{



	// --------------------------------------
	// Time & timer words.
	class TimeModule : public TForthModule
	{

	public:

		// Call to upload new time related words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{
			using timer = typename std::chrono::high_resolution_clock;

			// Example to measure performance of XXX
			// : TT TIMER_START XXX TIMER_END . ;
			// TT

			forth_comp.InsertWord_2_Dict( "TIMER_START",	std::make_unique< StackOp< TForth, CellType > >( forth_comp, 
				[] () { return std::chrono::duration_cast< std::chrono::milliseconds >( timer::now().time_since_epoch() ).count(); } ), " -- time_pt_ms " );


			forth_comp.InsertWord_2_Dict( "TIMER_END",	std::make_unique< StackOp< TForth, CellType, CellType > >( forth_comp, 
				[] ( const auto time_start ) { return std::chrono::duration_cast< std::chrono::milliseconds >( timer::now().time_since_epoch() ).count() - time_start; } ), " -- duration_ms " );





			// Get time as a string in the format: Www Mmm dd hh:mm:ss yyyy
			forth_comp.InsertWord_2_Dict( "GET_TIME",	std::make_unique< StackOp< TForth, CellType > >( forth_comp, 
				[ & forth_comp ] () 
				{  
					using timer = std::chrono::system_clock;

					std::time_t time_point = timer::to_time_t( timer::now() );
					std::string time_str( std::ctime( & time_point ) );

					// Call PAD to push its addr onto the data stack
					auto pad_stat = forth_comp.ExecWord( "PAD" );
					assert( pad_stat );
					CellType _pad_addr;
					auto stack_stat = forth_comp.GetDataStack().Pop( _pad_addr );
					assert( stack_stat );
					forth_comp.GetDataStack().Push( _pad_addr + 1 );

					CellType time_len = time_str.size();
					assert( time_len < ( 1 << 8 * sizeof( RawByte ) ) );
					RawByte * pad_addr = reinterpret_cast< RawByte * >( _pad_addr );
					assert( pad_addr != nullptr );
					* pad_addr ++ = static_cast< RawByte >( time_len );
					std::copy( time_str.begin(), time_str.end(), pad_addr );
			
					return time_len;	// this will be pushed onto the stack
				} ), " -- addr len " );


		}

	};






}


