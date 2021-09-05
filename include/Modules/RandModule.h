// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once



#include "Words.h"
#include "ForthCompiler.h"
#include <random>



namespace BCForth
{





	class RandomModule : public TForthModule
	{



		// ----------------------------------------------------------------------------
		// Template template parameter 
		template < typename Base, typename RetType, template < typename > typename RD >
		class RandValGen : public StackOp< Base, RetType >
		{
			public:

				using RandDistr = RD< RetType >;

			private:

				using BaseClass = StackOp< Base, RetType >;


				std::mt19937 fRandomEngine;		// so called Mersenne twister MT19937

				RandDistr	fRD;


			public:

				RandValGen( Base & f, RandDistr rd ) 
					:	BaseClass( f, [ this ] () { return fRD( fRandomEngine ); } ), 
						fRandomEngine( std::random_device()() ), fRD( rd ) {}

			public:

				using BaseClass::operator ();	// let's bring the base class implementation which simply calls the lambda provided in the constructor

		};
		// ----------------------------------------------------------------------------




	public:

		// Call to upload new words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{

			using RVG_Float = RandValGen< TForth, FloatType, std::uniform_real_distribution >;
			using RVG_Int = RandValGen< TForth, SignedIntType, std::uniform_int_distribution >;

			forth_comp.InsertWord_2_Dict( "FRAND",	std::make_unique< RVG_Float >( forth_comp, RVG_Float::RandDistr( 0.0, 1.0 ) ),	" -- fRand " );
			forth_comp.InsertWord_2_Dict( "RAND",	std::make_unique< RVG_Int >( forth_comp, RVG_Int::RandDistr( 0, 100 ) ),	" -- rand " );


			using RVG_Normal_Float = RandValGen< TForth, FloatType, std::normal_distribution >;                                 // mean, std
			forth_comp.InsertWord_2_Dict( "FNRAND",	std::make_unique< RVG_Normal_Float >( forth_comp, RVG_Normal_Float::RandDistr( 0.0, 1.0 ) ),	" -- fNormRand " );


		}


	};





}



