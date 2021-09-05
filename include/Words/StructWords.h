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



namespace BCForth
{




	// Special intermediate node for all structural nodes, such as IF and DO
	template < typename Base >
	class StructuralWord : public TWord< Base >
	{

	public:

		StructuralWord( Base & f ) : TWord< Base >( f ) {}

	public:

		void operator () ( void ) override = 0;

	};




	// Just a composite DP, i.e. a word consisting of other words
	template < typename Base >
	class CompoWord : public StructuralWord< Base >
	{
	public:

		using WordPtr	= typename Base::WordPtr;
		using WordsVec	= std::vector< WordPtr >;

	private:

		WordsVec		fWordsVec;

	public:

		CompoWord( Base & f ) : StructuralWord< Base >( f ) {}

	public:

		void AddWord( WordPtr wp ) { assert( wp ); fWordsVec.push_back( wp ); }

		WordsVec &			GetWordsVec( void )			{ return fWordsVec; }
		const WordsVec &	GetWordsVec( void ) const	{ return fWordsVec; }

	public:


		// Execute all
		void operator () ( void ) override
		{
			for( const auto op : fWordsVec )
				( * op )();
		}

	};



	// A helper for the CompoWords
	constexpr bool IsEmpty( const auto & compo_word )
	{
		return compo_word.GetWordsVec().size() == 0;
	}




	// ------------------------
	// Structural patterns




	template < typename Base >
	class IF : public StructuralWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using StructuralWord< Base >::GetDataStack;

		using CW = CompoWord< Base >;

		CW	fTrueBranch;
		CW	fFalseBranch;

	public:

		CW &	GetTrueNode( void )		{ return fTrueBranch; }
		CW &	GetFalseNode( void )	{ return fFalseBranch; }

	public:

		IF( Base & f ) : StructuralWord< Base >( f ), fTrueBranch( f ), fFalseBranch( f ) {}

	public:

		void operator () ( void ) override
		{
			if( typename DataStack::value_type t {}; GetDataStack().Pop( t ) )
				t == kBoolFalse ? fFalseBranch() : fTrueBranch();
			else
				throw ForthError( "unexpectedly empty stack" );
		}

	};







	// LEAVE makes an immediate exit from the current loop
	// Achieved by throwing a special exception
	template < typename Base >
	class LEAVE : public StructuralWord< Base >
	{
		using BaseClass = StructuralWord< Base >;

	public:

		class LEAVE_Exception : std::exception	// we need only its type, no action
		{};

	public:

		LEAVE( Base & f ) : BaseClass( f ) {}

	public:

		void operator () ( void ) override
		{
			throw LEAVE_Exception();		// to immediatelly exit from a loop we throw - this will be caught by the loop functor
		}

	};



	// <limit> <initial> DO <words to repeat>          LOOP
	// <limit> <initial> DO <words to repeat> <value> +LOOP
	// Expects the initial loop index on top of the stack, with the limit value beneath it
	template < typename Base >
	class DO_LOOP : public StructuralWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;

		using CW = CompoWord< Base >;

		CW	fBodyNodes;


		SignedIntType		fIndex {};

	public:

		CW &	GetBodyNodes( void ) { return fBodyNodes; }

		SignedIntType	GetIndex( void ) const { return fIndex; }

	public:

		DO_LOOP( Base & f ) : StructuralWord< Base >( f ), fBodyNodes( f ) {}

	public:

		void operator () ( void ) override
		{
			// Get the current limits from teh stack
			auto & ds { GetDataStack() };
			if( typename DataStack::value_type limit {}, initial {}; ds.Pop( initial ) && ds.Pop( limit ) )
			{

				fIndex						= static_cast< SignedIntType >( initial );
				const SignedIntType kTo		= static_cast< SignedIntType >( limit );

				SignedIntType step_val {};

				try
				{
					do
					{
						fBodyNodes();		// the last one should leave the increment step on the data stack

						if( typename DataStack::value_type	s {}; ds.Pop( s ) )
							step_val = static_cast< SignedIntType >( s );
						else
							throw ForthError( "unexpectedly empty stack" );

						assert( step_val != 0 );		// otherwise the loop is infinite
						fIndex += step_val;

					} while( step_val < 0 ? fIndex >= kTo : fIndex < kTo );
				}
				catch( typename LEAVE< Base >::LEAVE_Exception & )
				{
					// this one is special for LEAVE - everything is under control, just return
					return;
				}
				// all other potential exceptions go to upper layers

			}
			else
			{
				throw ForthError( "unexpectedly empty stack" );
			}
		}

	};


	// Loop index node 
	template < typename Base >
	class I_LOOP : public TWord< Base >
	{
		using TWord< Base >::GetDataStack;

		const DO_LOOP< Base > &	fMyLoopNode;

	public:

		I_LOOP( Base & f, const DO_LOOP< Base > & my_loop ) : TWord< Base >( f ), fMyLoopNode( my_loop ) {}

	public:

		void operator () ( void ) override
		{
			GetDataStack().Push( static_cast< CellType >( fMyLoopNode.GetIndex() ) );
		}

	};







	// There are 3 structures starting with BEGIN
	// 
	// BEGIN ... S ... AGAIN		( indefinite )
	// BEGIN ... S ..v UNTIL		( iterate as long as v is FALSE )
	// BEGIN ... SA ...v WHILE ... SB ... REPEAT ( WHILE checks v, if FALSE, then exit the loop; otherwise REPEAT jumps to BEGIN )
	//
	// The thing is, however, that just when processing BEGIN we don't know exactly which version it is.
	// This is set only after reaching one of the keywords: AGAIN or UNTIL or WHILE-REPEAT
	//
	template < typename Base >
	class BEGIN_LOOP : public StructuralWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using TWord< Base >::GetDataStack;

		using CW = CompoWord< Base >;

		CW	fBegin_Nodes;		// all nodes in the BEGIN	 ... WHILE (UNTIL) branch
		CW	fWhile_Nodes;		// all nodes in the              WHILE ... REPEAT branch (empty for the AGAIN and UNTIL versions)


	private:

		// Set to exit the loop asap
		bool Exit( void )
		{
			return false;
		}

		// Returns true to continue the loop - this one always
		bool Again( void )
		{
			return true;
		}

		// Returns true to continue the loop - this one if condition on the stack is FALSE
		bool Until( void )
		{
			if( typename DataStack::value_type	cond {}; GetDataStack().Pop( cond ) )
				return cond ? false : true;
			else
				throw ForthError( "unexpectedly empty stack" );	
		}

		// Returns true to continue the loop - this one if condition on the stack is TRUE
		bool WhileRepeat( void )
		{
			if( typename DataStack::value_type	cond {}; GetDataStack().Pop( cond ) )
				return cond ? fWhile_Nodes(), true : false;
			else
				throw ForthError( "unexpectedly empty stack" );	
		}



		std::function< bool ( void ) >		fInternalFun;


	public:

		CW &	Get_Begin_Nodes( void ) { return fBegin_Nodes; }
		CW &	Get_While_Nodes( void ) { return fWhile_Nodes; }


		enum class EBeginLoopType { kAgain, kUntil, kWhileRepeat, kExit };

		void SetLoopType( EBeginLoopType ltp ) 
		{
			switch( ltp )
			{
			case EBeginLoopType::kAgain:
				fInternalFun = [ this ] () { return Again(); } ;
				break;

			case EBeginLoopType::kUntil:
				fInternalFun = [ this ] () { return Until(); } ;
				break;

			case EBeginLoopType::kWhileRepeat:
				fInternalFun = [ this ] () { return WhileRepeat(); } ;
				break;

			case EBeginLoopType::kExit:
				fInternalFun = [ this ] () { return Exit(); } ;
				break;
			}
		} 

	public:

		BEGIN_LOOP( Base & f ) : StructuralWord< Base >( f ), fBegin_Nodes( f ), fWhile_Nodes( f )
		{ 
			SetLoopType( EBeginLoopType::kAgain ); 
		}

	public:

		void operator () ( void ) override
		{
			try
			{
				// ---------------------
				do
				{
					fBegin_Nodes();		
				}
				while( fInternalFun() );
				// ---------------------

			}
			catch( typename LEAVE< Base >::LEAVE_Exception & )
			{
				// this one is special for LEAVE - everything is under control, just return
				return;
			}
			// all other potential exceptions go to upper layers


		}

	};



	// It can be associated with a BEGIN ... loop 
	// and when executed it makes the loop to break
	template < typename Base >
	class EXIT_BEGIN_LOOP : public TWord< Base >
	{
		BEGIN_LOOP< Base > &	fMyBeginNode;

	public:

		EXIT_BEGIN_LOOP( Base & f, BEGIN_LOOP< Base > & my_loop ) : TWord< Base >( f ), fMyBeginNode( my_loop ) {}

	public:

		void operator () ( void ) override
		{
			fMyBeginNode.SetLoopType( BEGIN_LOOP< Base >::EBeginLoopType::kExit );
		}

	};



	// Just a placeholder
	template < typename Base >
	class CASE : public CompoWord< Base >
	{
		using BaseClass = CompoWord< Base >;

	public:

		using BaseClass::AddWord;

	public:

		CASE( Base & f ) : BaseClass( f ) {}

	public:

		void operator () ( void ) override
		{
			BaseClass::operator() ();		// call the base composite
		}

	};






	// ------------------------
	// Creational pattern
	//
	//: ARRAY ( n -- ) CREATE DUP , CELLS ALLOT
	//	DOES> ( n -- a ) SWAP OVER @ OVER < OVER 1 < OR ABORT" Out of range" 
	//	CELLS + ;
	//
	//: MSG ( -- ) CREATE
	//	DOES> ( -- ) COUNT TYPE ;
	//
	// MSG (CR)		2 C, 0D C, 0A C,
	//

	template < typename Base >
	class DOES : public StructuralWord< Base >
	{
		using DataStack = typename Base::DataStack;
		using StructuralWord< Base >::GetDataStack;

		using CW = CompoWord< Base >;

		CW	fCreationBranch;
		CW	fBehaviorBranch;

	public:

		CW &	GetCreationNode( void )		{ return fCreationBranch; }
		CW &	GetBehaviorNode( void )		{ return fBehaviorBranch; }

	public:

		DOES( Base & f ) : StructuralWord< Base >( f ), fCreationBranch( f ), fBehaviorBranch( f ) {}

	public:

		// 12 ARRAY SHUTTER
		// ...
		// 3 SHUTTER
		//
		// This needs to add a defining word into the dictionary
		void operator () ( void ) override
		{
			assert( fCreationBranch.GetWordsVec().size() > 0 );
			fCreationBranch();		// call the creational branch
		}

	};





}	// The end of the BCForth namespace





