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
#include <cmath>
#include <random>



namespace BCForth
{


	class FP_Module : public TForthModule
	{



	public:

		// Call to upload new words to the forth_comp
		void operator () ( TForthCompiler & forth_comp ) override
		{


			forth_comp.InsertWord_2_Dict( ".F",		std::make_unique< Dot< TForth, FloatType > >( forth_comp, forth_comp.GetOutStream() ), " xf -- " );
			forth_comp.InsertWord_2_Dict( ".FS",	std::make_unique< Dot_S< TForth, FloatType > >( forth_comp, forth_comp.GetOutStream() ), " xf -- xf " );
			forth_comp.InsertWord_2_Dict( ".SDF",	std::make_unique< Stack_Dump< TForth, FloatType > >( forth_comp, forth_comp.GetOutStream(), Letter_2_Name( kSpace ) ), " x -- x ==> float stack dump " );


			forth_comp.InsertWord_2_Dict( "F+",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Plus< FloatType >(); }	> >( forth_comp ), " xf yf -- xf+yf " );
			forth_comp.InsertWord_2_Dict( "F-",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Minus< FloatType >(); }	> >( forth_comp ), " xf yf -- xf-yf " );
			forth_comp.InsertWord_2_Dict( "F*",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Mult< FloatType >(); }	> >( forth_comp ), " xf yf -- xf*yf " );
			forth_comp.InsertWord_2_Dict( "F/",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template Div< FloatType >(); }		> >( forth_comp ), " xf yf -- xf/yf " );


			forth_comp.InsertWord_2_Dict( "F=",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template EQ< FloatType >();  } > >( forth_comp ), " xf yf -- xf<yf " );
			forth_comp.InsertWord_2_Dict( "F<>",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template NE< FloatType >();  } > >( forth_comp ), " xf yf -- xf<=yf " );
			forth_comp.InsertWord_2_Dict( "F<",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LT< FloatType >();  } > >( forth_comp ), " xf yf -- xf>yf " );
			forth_comp.InsertWord_2_Dict( "F<=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template LE< FloatType >();  } > >( forth_comp ), " xf yf -- xf>=yf " );
			forth_comp.InsertWord_2_Dict( "F>",		std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GT< FloatType >();  } > >( forth_comp ), " xf yf -- xf=yf " );
			forth_comp.InsertWord_2_Dict( "F>=",	std::make_unique< ExGenericStackOp< TForth, [] ( auto & ds ) { return ds.template GE< FloatType >();  } > >( forth_comp ), " xf yf -- xf<>yf " );


			using UnaryFloatOp = StackOp< TForth, FloatType, FloatType >;
			using BinFloatOp = StackOp< TForth, FloatType, FloatType, FloatType >;

			forth_comp.InsertWord_2_Dict( "FNEG",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return -x; } ), " x -- -x " );


			forth_comp.InsertWord_2_Dict( "SQRT",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return std::sqrt( x ); } ), " xf -- sqrt(xf) " );
			forth_comp.InsertWord_2_Dict( "POW",	std::make_unique< BinFloatOp >( forth_comp, [] ( const auto x, const auto y ) { return std::pow( x, y ); } ), " xf yf -- pow(xf,yf) " );


			forth_comp.InsertWord_2_Dict( "SIN",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return std::sin( x ); } ), " xf -- sin(xf) " );
			forth_comp.InsertWord_2_Dict( "COS",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return std::cos( x ); } ), " xf -- cos(xf) " );
			forth_comp.InsertWord_2_Dict( "TAN",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return std::tan( x ); } ), " xf -- tan(xf) " );
			forth_comp.InsertWord_2_Dict( "ATAN",	std::make_unique< UnaryFloatOp >( forth_comp, [] ( const auto x ) { return std::tan( x ); } ), " xf -- atan(xf) " );
			forth_comp.InsertWord_2_Dict( "ATAN2",	std::make_unique< BinFloatOp >( forth_comp, [] ( const auto x, const auto y ) { return std::atan2( x, y ); } ), " xf yf -- atan2(xf,yf) " );


			// Convert the top data int->float, float->int
			forth_comp.InsertWord_2_Dict( "2INT",	std::make_unique< StackOp< TForth, SignedIntType, FloatType > >( forth_comp, [] ( const auto x ) { return static_cast< SignedIntType >( x ); } ), " f -- i " );
			forth_comp.InsertWord_2_Dict( "2FP",	std::make_unique< StackOp< TForth, FloatType, SignedIntType > >( forth_comp, [] ( const auto x ) { return static_cast< FloatType >( x ); } ), " i -- f " );

		}

	};



}



