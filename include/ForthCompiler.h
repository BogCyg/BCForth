// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================



#pragma once



#include "ForthInterpreter.h"







namespace BCForth
{




	// Responsible for processing of word definitions via : ;
	class TForthCompiler : public TForthInterpreter
	{

	private:

		Name	fWordCommentStr;		// this string collects all user's comments entered into the word's definition

		Name	fCompiledWordName;
		bool	fAllImmediate	{ false };	// used when processing [ ... ] in the compilation stage

		bool	fProcessingDefiningWord { false };		// when true, then a defining word is compiled, i.e. containing DOES>

	protected:

		using Base = TForthInterpreter;

		using Base::fWordDict;
		using Base::fDataStack;
		using Base::fRetStack;

		using Base::fOutStream;

		using Base::CollectTextUpToTokenContaining;



	protected:

		// The plug-in for the Forth interpreter
		void ProcessContextSequences( Names & ns ) override
		{
			const auto kNumNames { ns.size() };
			if( kNumNames == 0 )
				return;

			const auto & leadName { ns[ 0 ] };

			// IMMEDIATE
			if( leadName == "IMMEDIATE" )
			{
				// Let's find the lastly entered definition and mark it immediate
				assert( fCompiledWordName.length() > 0 );

				if( auto word = GetWordEntry( fCompiledWordName ) )
					( * word )->fWordIsImmediate = true;
				else
					assert( false );

				Erase_n_First_Words( ns, 1 );
				return;
			}

			// Call the base interpreter
			Base::ProcessContextSequences( ns );
		}


		// Destructor will be created automatically virtual due to declaration in the base class

	protected:


		using CompoWordPtr = CompoWord< TForth > *;


		using StructuralWordPtr = StructuralWord< TForth > * ;

		static const size_type kStructuralStackSize { 64 };			// the max number of nested IF ... DO ...
		using StructuralStack = TStackFor< StructuralWordPtr, kStructuralStackSize >;


		StructuralStack		fStructuralStack;	// the stack to process structural constructions such as IF ... THEN


	protected:



		// theWord - collects the defining words
		// ns - a list of words that will be consumed one after one
		void Compile_StructuralWords_Into( CompoWord< TForth > & theWord, Names & ns )
		{

			const auto kNumTokens { ns.size() };
			if( kNumTokens == 0 )
				return;

			const auto & token { ns[ 0 ] };



			// IF ... ELSE ... THEN
			// : TEST ( n -- )   DUP 0= IF DROP ELSE PROCESS THEN ;
			if( token == "IF" )
			{

				Erase_n_First_Words( ns, 1 );

				// Create the IF node and insert to the current definition

				auto if_node { std::make_unique< IF< TForth > >( * this ) };
				auto if_node_ptr { if_node.get() };
				theWord.AddWord( Insert_2_NodeRepo( std::move( if_node ) ) );

				fStructuralStack.Push( & theWord );			// push the "current" node
				fStructuralStack.Push( if_node_ptr );		// push this "IF" node

				// Put its "TRUE" branch as the current insertion node
				Compile_All_Into( if_node_ptr->GetTrueNode(), ns );

				return;
			}



			// IF ... ELSE ... THEN
			if( token == "ELSE" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just changed to the FALSE branch
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Peek( struct_node_ptr ) )
					if( auto * if_node_ptr = dynamic_cast< IF< TForth > * >( struct_node_ptr ) )
						Compile_All_Into( if_node_ptr->GetFalseNode(), ns );
					else
						throw ForthError( "incorrectly interspersed structured IF - DO - LOOP" );
				else
					throw ForthError( "unbalanced IF - THEN structure" );

				return;
			}


			// IF ... ELSE ... THEN
			if( token == "THEN" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just remove the last IF node from the structural stack
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( ! dynamic_cast< IF< TForth > * >( struct_node_ptr ) )
						throw ForthError( "incorrectly interspersed structured IF - DO - LOOP" );	

					// Pop the previous "context"
					if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
						if( CompoWordPtr compo_word_ptr = dynamic_cast< CompoWordPtr >( struct_node_ptr ) )
							Compile_All_Into( * compo_word_ptr, ns );
						else
							throw ForthError( "incorrectly interspersed structured IF - DO - LOOP" );	
					else
						throw ForthError( "unbalanced IF - THEN structure" );

				}
				else
				{
					throw ForthError( "unbalanced IF - THEN structure" );
				}


				return;
			}







			// DO ... LOOP
			if( token == "DO" )
			{
				Erase_n_First_Words( ns, 1 );


				auto do_node { std::make_unique< DO_LOOP< TForth > >( * this ) };
				auto do_node_ptr { do_node.get() };
				theWord.AddWord( Insert_2_NodeRepo( std::move( do_node ) ) );

				fStructuralStack.Push( & theWord );			// push the "current" node
				fStructuralStack.Push( do_node_ptr );		// push the "DO" node

				// Put its body branch as the current insertion node
				Compile_All_Into( do_node_ptr->GetBodyNodes(), ns );

				return;
			}



			// DO ... LOOP
			if( token == "LOOP" || token == "+LOOP" )
			{

				// Compile the extra "+1" literal node as the step value
				if( token[ 0 ] != kPlus )
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< IntValWord< TForth > >( * this, +1 ) ) );

				Erase_n_First_Words( ns, 1 );		// get rid of the token


				// Just remove the last DO node from the structural stack
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( ! dynamic_cast< DO_LOOP< TForth > * >( struct_node_ptr ) )
						throw ForthError( "incorrectly interspersed structured IF - DO - LOOP" );	

					// Pop the previous "context"
					if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
						if( CompoWordPtr compo_word_ptr = dynamic_cast< CompoWordPtr >( struct_node_ptr ) )
							Compile_All_Into( * compo_word_ptr, ns );
						else
							throw ForthError( "incorrectly interspersed structured IF - DO - LOOP" );	
					else
						throw ForthError( "unbalanced IF - THEN structure" );

				}
				else
				{
					throw ForthError( "unbalanced DO - LOOP structure" );
				}

				return;
			}



			// I 
			if( token == "I" || token == "J" )
			{
				auto skipCounter { token[ 0 ] - 'I' };		// "I" is 0, "J" is 1 (outeremost loop index)
				
				Erase_n_First_Words( ns, 1 );

				// Find a DO node and connect with the new I_LOOP node

				auto struct_stack { fStructuralStack.data() };

				for( auto i { fStructuralStack.size() }; i > 0; -- i )
				{
					if( const auto * do_node = dynamic_cast< DO_LOOP< TForth > * >( fStructuralStack.data()[ i - 1 ] ) )
					{
						// Found, ok
						if( skipCounter -- > 0 )
							continue;							// go and search deeper in the stack

						theWord.AddWord( Insert_2_NodeRepo( std::make_unique< I_LOOP< TForth > >( * this, * do_node ) ) );
						Compile_All_Into( theWord, ns );		// process the same compound word

						return;
					}
				}

				throw ForthError( " loop index I used in wrong context" );
			}



			// ==========================================
			// There are 3 structures starting with BEGIN
			// 
			// BEGIN ... S ... AGAIN		( indefinite )
			// BEGIN ... S ..v UNTIL		( iterate as long as v is FALSE )
			// BEGIN ... SA ...v WHILE ... SB ... REPEAT ( WHILE checks v, if FALSE, then exit the loop; otherwise REPEAT jumps to BEGIN )
			//
			if( token == "BEGIN" )
			{
				Erase_n_First_Words( ns, 1 );


				auto do_node { std::make_unique< BEGIN_LOOP< TForth > >( * this ) };
				auto do_node_ptr { do_node.get() };
				theWord.AddWord( Insert_2_NodeRepo( std::move( do_node ) ) );

				fStructuralStack.Push( & theWord );			// push the "current" node
				fStructuralStack.Push( do_node_ptr );		// push the "BEGIN" node

				// Put its body branch as the current insertion node
				Compile_All_Into( do_node_ptr->Get_Begin_Nodes(), ns );

				return;
			}



			if( token == "AGAIN" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just remove the last BEGIN node from the structural stack
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( auto begin_node = dynamic_cast< BEGIN_LOOP< TForth > * >( struct_node_ptr ) )
						begin_node->SetLoopType( BEGIN_LOOP< TForth >::EBeginLoopType::kAgain );
					else
						throw ForthError( "incorrectly interspersed structured BEGIN - AGAIN" );	


					// Pop the previous "context"
					if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
						if( CompoWordPtr compo_word_ptr = dynamic_cast< CompoWordPtr >( struct_node_ptr ) )
							Compile_All_Into( * compo_word_ptr, ns );
						else
							throw ForthError( "incorrectly interspersed structured BEGIN - AGAIN" );	
					else
						throw ForthError( "unbalanced BEGIN - AGAIN structure" );

				}
				else
				{
					throw ForthError( "unbalanced BEGIN - AGAIN structure" );
				}

				return;
			}


			if( token == "UNTIL" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just remove the last BEGIN node from the structural stack
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( auto begin_node = dynamic_cast< BEGIN_LOOP< TForth > * >( struct_node_ptr ) )
						begin_node->SetLoopType( BEGIN_LOOP< TForth >::EBeginLoopType::kUntil );
					else
						throw ForthError( "incorrectly interspersed structured BEGIN - UNTIL" );	


					// Pop the previous "context"
					if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
						if( CompoWordPtr compo_word_ptr = dynamic_cast< CompoWordPtr >( struct_node_ptr ) )
							Compile_All_Into( * compo_word_ptr, ns );
						else
							throw ForthError( "incorrectly interspersed structured BEGIN - UNTIL" );	
					else
						throw ForthError( "unbalanced BEGIN - UNTIL structure" );

				}
				else
				{
					throw ForthError( "unbalanced BEGIN - UNTIL structure" );
				}

				return;
			}


			if( token == "WHILE" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just remove the last BEGIN node from the structural stack
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( auto begin_node = dynamic_cast< BEGIN_LOOP< TForth > * >( struct_node_ptr ) )
					{
						begin_node->SetLoopType( BEGIN_LOOP< TForth >::EBeginLoopType::kWhileRepeat );
						Compile_All_Into( begin_node->Get_While_Nodes(), ns );		// work out the WHILE ... REPEAT branch
					}
					else
					{
						throw ForthError( "incorrectly interspersed structured BEGIN - WHILE" );	
					}
				}
				else
				{
					throw ForthError( "unbalanced BEGIN - WHILE structure" );
				}

				return;
			}


			if( token == "REPEAT" )
			{
				Erase_n_First_Words( ns, 1 );

				// Pop the previous "context"
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
					if( CompoWordPtr compo_word_ptr = dynamic_cast< CompoWordPtr >( struct_node_ptr ) )
						Compile_All_Into( * compo_word_ptr, ns );
					else
						throw ForthError( "incorrectly interspersed structured BEGIN - WHILE - REPEAT" );	
				else
					throw ForthError( "unbalanced BEGIN - WHILE - REPEAT structure" );

				return;
			}



			if( token == "EXIT" )
			{
				Erase_n_First_Words( ns, 1 );

				// Find the closest BEGIN node and connect with the EXIT_BEGIN_LOOP

				for( auto i { fStructuralStack.size() }; i > 0; -- i )
				{
					if( auto * do_node = dynamic_cast< BEGIN_LOOP< TForth > * >( fStructuralStack.data()[ i - 1 ] ) )
					{
						// Found, ok
						theWord.AddWord( Insert_2_NodeRepo( std::make_unique< EXIT_BEGIN_LOOP< TForth > >( * this, * do_node ) ) );
						Compile_All_Into( theWord, ns );		// process the same compound word

						return;
					}
				}

				throw ForthError( " EXIT word used without BEGIN" );
			}


			// ==========================================

			// CASE ... ENDCASE
			// an example
			// : TEST CASE ." chosen: "
			//			0 OF  ." no"  ENDOF
			//			1 OF  ." one" ENDOF
			//			2 OF  ." two" CR .F CR .F CR ENDOF
			//			DUP .			\ show it before it vanishes
			//		ENDCASE ;
			// CASE will be transformed into the nested IF ... ELSE ... THEN
			if( token == "CASE" )
			{
				// Actually do nothing - the real actions take place after encountering OF
				Erase_n_First_Words( ns, 1 );

				fStructuralStack.Push( & theWord );			// push the "current" context


				auto case_node { std::make_unique< CASE< TForth > >( * this ) };
				auto case_node_ptr { case_node.get() };
				theWord.AddWord( Insert_2_NodeRepo( std::move( case_node ) ) );

				fStructuralStack.Push( case_node_ptr );		// push this "CASE" node

				Compile_All_Into( * case_node_ptr, ns );	// from now on operate in the context of CASE

				return;
			}

			if( token == "OF" )
			{
				Erase_n_First_Words( ns, 1 );

				// Before the IF node insert OVER =
				// CHANGE - these words should be already in the dictionary, so find them and isert calls to them 
				assert( GetWordEntry( "OVER" ) );
				theWord.AddWord( ( * GetWordEntry( "OVER" ) )->fWordUP.get() );

				assert( GetWordEntry( "=" ) );
				theWord.AddWord( ( * GetWordEntry( "=" ) )->fWordUP.get() );



				// Create the IF node and insert to the current definition
				auto if_node { std::make_unique< IF< TForth > >( * this ) };
				auto if_node_ptr { if_node.get() };
				theWord.AddWord( Insert_2_NodeRepo( std::move( if_node ) ) );

				fStructuralStack.Push( if_node_ptr );		// push this "IF" node


				assert( GetWordEntry( "DROP" ) );
				if_node_ptr->GetTrueNode().AddWord( ( * GetWordEntry( "DROP" ) )->fWordUP.get() );

				// Put its "TRUE" branch as the current insertion node
				Compile_All_Into( if_node_ptr->GetTrueNode(), ns );

				return;

			}


			// Acts as ELSE
			if( token == "ENDOF" )
			{
				Erase_n_First_Words( ns, 1 );

				// Just changed to the FALSE branch
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Peek( struct_node_ptr ) )
					if( auto * if_node_ptr = dynamic_cast< IF< TForth > * >( struct_node_ptr ) )
						Compile_All_Into( if_node_ptr->GetFalseNode(), ns );
					else
						throw ForthError( "incorrectly interspersed structured OF - ENDOF" );
				else
					throw ForthError( "unbalanced IF - THEN structure" );


				return;
			}



			// ENDCASE needs to un-wind the fStructuralStack up to the nearest CASE node
			if( token == "ENDCASE" )
			{
				Erase_n_First_Words( ns, 1 );



				// bug fix - we don't need this DROP since it is already done by the last IF in the chain
				// Add DROP to the last (the newest on the stack) IF ... ELSE < we are here > THEN
				if( StructuralWordPtr struct_node_ptr {}; fStructuralStack.Pop( struct_node_ptr ) )
				{
					if( auto last_if_branch = dynamic_cast< IF< TForth > * >( struct_node_ptr ) )
					{
						assert( GetWordEntry( "DROP" ) );		// the last is DROP to get rid of the untaken value
						last_if_branch->GetFalseNode().AddWord( ( * GetWordEntry( "DROP" ) )->fWordUP.get() );


						// Dig out all the remaining IF ... ELSE ... THEN structures up to CASE
						for( auto node_ptr { StructuralStack::value_type() }; fStructuralStack.Pop( node_ptr ); )
						{
							if( dynamic_cast< CASE< TForth > * >( node_ptr ) && fStructuralStack.Pop( node_ptr ) )	// got the CASE, so immediately get next node (previous context)
							{
								if( auto comp_node_ptr = dynamic_cast< CompoWord< TForth > * >( node_ptr ) )
								{
									Compile_All_Into( * comp_node_ptr, ns );
									return;
								}
								else
								{
									assert( false );
								}

							}
						}

					}

				}



				throw ForthError( " unbalanced CASE ... ENDCASE" );
			}



			// Compile-in word's execution token (i.e. the address of the Word object to execute its operato())
			if( token == "[']" )
			{
				// The same action as for the LITERAL but with the word's pointer 
				if( const auto word_entry_ptr = GetWordEntry( ns[ 1 ] ) )
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< CellValWord< TForth > >( * this, reinterpret_cast< CellType >( ( * word_entry_ptr )->fWordUP.get() ) ) ) );
				else
					throw ForthError( " unknown word " + ns[ 1 ] + " following [']" );

				Erase_n_First_Words( ns, 2 );		// get rid of the two tokenn
				return;
			}




			// ==========================================

			// Process immediate words
			if( token == "[" )
			{
				fAllImmediate = true;
				Erase_n_First_Words( ns, 1 );		// get rid of the token
				return;
			}


			if( token == "]" )
			{
				fAllImmediate = false;
				Erase_n_First_Words( ns, 1 );		// get rid of the token
				Compile_All_Into( theWord, ns );	// return to the compile mode
				return;
			}


			if( token == "POSTPONE" )
			{
				if( ns.size() <= 1 )
					throw ForthError( "Syntax  POSTPONE should be followed by a word" );


				if( const auto word_entry_ptr = GetWordEntry( ns[ 1 ] ) )
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< Postpone< TForth > >( * this, ( * word_entry_ptr )->fWordUP.get() ) ) );
				else
					throw ForthError( " unknown word " + ns[ 1 ] + " following POSTPONE" );


				Erase_n_First_Words( ns, 2 );		// get rid of the token
				Compile_All_Into( theWord, ns );	// return to the compile mode
				return;
			}



			if( token == "LITERAL" )
			{
				if( typename DataStack::value_type t {}; GetDataStack().Pop( t ) )
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< CellValWord< TForth > >( * this, t ) ) );
				else
					throw ForthError( "unexpectedly empty stack" );

				Erase_n_First_Words( ns, 1 );		// get rid of the token
				return;		
			}


			if( token == "DOES>" )
			{

				// When DOES> is encountered then the following needs to be done:
				// - the DOES> node needs to be created
				// - its creational branch is copied from the currently collected words of "theWord"
				// - its behavioral branch becomes the current context
				// - the creational branch must contain the CREATE word (otherwise syntax error)


				Erase_n_First_Words( ns, 1 );		// get rid of the "DOES>" token

				assert( fProcessingDefiningWord == false );
				if( fProcessingDefiningWord == true )
					throw ForthError( " word definition can contain only one DOES>" );

				fProcessingDefiningWord = true;

				auto does_node { std::make_unique< DOES< TForth > >( * this ) };
				auto does_node_ptr { does_node.get() };
				assert( does_node_ptr );

				does_node_ptr->GetCreationNode().GetWordsVec() = theWord.GetWordsVec();		// copy whatever has been already collected in theWord
				theWord.GetWordsVec().clear();
				theWord.AddWord( Insert_2_NodeRepo( std::move( does_node ) ) );				// from now on, execution of theWord will launch exclusively action of the DOES node

				// Switch off the current context to the behavioral branch of the DOES node
				Compile_All_Into( does_node_ptr->GetBehaviorNode(), ns );					// return to the compile mode

				return;
			}


			// (bracket-care) take the following word/char and compile its ASCII value
			if( token == "[CHAR]" )
			{
				if( ns.size() <= 1 )
					throw ForthError( "Syntax  [CHAR] should be followed by a text" );

				theWord.AddWord( Insert_2_NodeRepo( std::make_unique< CharValWord< TForth > >( * this, BlindValueReInterpretation< Char >( ns[ 1 ][ 0 ] ) ) ) );

				Erase_n_First_Words( ns, 2 );		// get rid of the tokens
				Compile_All_Into( theWord, ns );	// return to the compile mode
				return;		
			}

		}


		// theWord - collects the defining words
		// ns - a list of words that will be consumed one after one
		void Compile_All_Into( CompoWord< TForth > & theWord, Names & ns )
		{

			Compile_StructuralWords_Into( theWord, ns );


			const auto kNumTokens { ns.size() };
			if( kNumTokens == 0 )
				return;


			const auto & token { ns[ 0 ] };


			if( IsInteger( token, ReadTheBase() ) )
			{
				if( fAllImmediate )
					GetDataStack().Push( BlindValueReInterpretation< CellType >( Word_2_Integer( token ) ) );
				else
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< IntValWord< TForth > >( * this, Word_2_Integer( token ) ) ) );

				Erase_n_First_Words( ns, 1 );
				Compile_All_Into( theWord, ns );
				return;
			}


			if( IsFloatingPt( token ) )
			{	
				if( fAllImmediate )
					GetDataStack().Push( BlindValueReInterpretation< CellType >( stod( token ) ) );
				else
					// Const from the words' definitions are compiled into the dictionary as well 
					theWord.AddWord( Insert_2_NodeRepo( std::make_unique< DblValWord< TForth > >( * this, stod( token) ) ) );

				Erase_n_First_Words( ns, 1 );
				Compile_All_Into( theWord, ns );
				return;
			}


			if( token == kDotQuote || token == kCQuote || token == kSQuote )
			{
				// Just entered the number of tokens up to the closing "
				const auto loc_token { token };		// make a local copy before it is erased
				Erase_n_First_Words( ns, 1 );

				if( auto [ flag, str ] = CollectTextUpToTokenContaining( ns, Letter(), kQuote ); flag )
				{
					// First, a small factory
					WordPtr		wp {};
					if( loc_token == kDotQuote )
						wp = Insert_2_NodeRepo( std::make_unique< QuoteSuite< TForth > >( * this, std::move( str ), 
																							[ this ] ( const auto & s, auto & ) { fOutStream << s; return true; } ) );
					else
						if( loc_token == kSQuote )		// ( -- addr u )
							wp = Insert_2_NodeRepo( std::make_unique< QuoteSuite< TForth > >( * this, std::move( str ), 
																							[ this ] ( const auto & s, auto & ds ) { ds.Push( reinterpret_cast< CellType >( s.data() ) ); ds.Push( static_cast< CellType >( s.length() ) ); return true; } ) );
						else							// ( -- addr )
							wp = Insert_2_NodeRepo( std::make_unique< QuoteSuite< TForth > >( * this, std::move( str ), 
																							[ this ] ( const auto & s, auto & ds ) { ds.Push( reinterpret_cast< CellType >( s.data() ) ); return true; } ) );


					// Then, either execute if in the immediate mode or add to the current definition
					if( fAllImmediate )
						( * wp )();
					else
						theWord.AddWord( wp );

				}
				else
				{
					throw ForthError( "no matching \" found" );
				}

				Compile_All_Into( theWord, ns );
				return;
			}



			if( token == kAbortQuote )
			{
				// Just entered the number of tokens up to the closing "

				Erase_n_First_Words( ns, 1 );

				if( auto [ flag, str ] = CollectTextUpToTokenContaining( ns, Letter(), kQuote ); flag )
					if( fAllImmediate )
						throw str;		// what to do with the immediate ABORT" ?
					else
						theWord.AddWord( Insert_2_NodeRepo( std::make_unique< AbortQuote< TForth > >( * this, std::move( str ) ) ) );
				else
					throw ForthError( "no closing \" found for the opening ABORT\"" );

				Compile_All_Into( theWord, ns );
				return;
			}


			// Extract word's comment
			if( token == Name( 1, kLeftParen ) )
			{
				// Just entered the number of tokens up to the closing "

				Erase_n_First_Words( ns, 1 );

				if( auto [ flag, str ] = CollectTextUpToTokenContaining( ns, kLeftParen, kRightParen ); flag )
					fWordCommentStr += str;
				else
					throw ForthError( "no closing \" found for the opening .\"" );

				Compile_All_Into( theWord, ns );
				return;
			}



			// Look for the words in the dictionary
			if( const auto word_entry_ptr = GetWordEntry( token ); word_entry_ptr && ( * word_entry_ptr )->fWordIsCompiled == false )
			{
				// If IMMEDIATE, or in the [ ... ] context, then execute righ now
				if( ( * word_entry_ptr )->fWordIsImmediate || fAllImmediate )
				{
					fWordDefinitionContext_4_Postpone = & theWord;	// enter the IMMEDIATE execution inside the : ; definition (used to properly handle POSTPONE)
					( * ( (*word_entry_ptr)->fWordUP ) )();
					fWordDefinitionContext_4_Postpone = nullptr;	// exit the IMMEDIATE mode
				}
				else
				{
					theWord.AddWord( ( * word_entry_ptr )->fWordUP.get() );
				}


				Erase_n_First_Words( ns, 1 );
				Compile_All_Into( theWord, ns );		
				return;
			}
			else
			{
				throw ForthError( "Unknown word " + token + " used in definition" );
			}


		}



		virtual bool EnterWordDefinition( Names && ns )
		{
			const auto kTokens { ns.size() };
			assert( kTokens > 2 );				// at least : [name] ;

			assert( ns[ 0 ][ 0 ] == kColon );
			assert( ns[ kTokens - 1 ][ 0 ] == kSemColon );

			// Check if the word with that name is already registered in the dictionary (ok to overwrite?)
			const auto & word_name { ns[ 1 ] };
			if( GetWordEntry( word_name ) )
				if( DecisionOnWordAlreadyExists( word_name ) == false )
					return false;	// don't overwrite


			fCompiledWordName = word_name;			// store it in the case this word will be later marked as IMMEDIATE

			fProcessingDefiningWord = false;		// we don't know yet

			// At firt create an entry for the (possibly) new word,
			// or replace the previous one

			WordUP new_word_node { std::make_unique< CompoWord< TForth > >( * this ) };
			CompoWord< TForth > * new_word_node_ptr { dynamic_cast< CompoWord< TForth > * >( new_word_node.get() ) };


			//                                                    is being compiled
			WordEntry new_word_entry( std::move( new_word_node ), true, false, false, "" );


			ns.erase( ns.begin() );						// remove :
			ns.erase( ns.begin() );						// remove the word name
			ns.erase( ns.begin() + ns.size() - 1 );		// remove ;



			Compile_All_Into( * new_word_node_ptr, ns );


			CheckForErrors();		// will throw on errors


			new_word_entry.fWordComment = fWordCommentStr;			// copy the collected comment
			fWordCommentStr = "";									// reset the comment string

			new_word_entry.fWordIsCompiled = false;					// indicate the end of compilation
			new_word_entry.fWordIsDefining = fProcessingDefiningWord;
			fWordDict[ fCompiledWordName ] = std::move( new_word_entry );	// the new word is entered to the dictionary (possibly obliterating the old definition with the same name)


			return true;
		}



		void CheckForErrors( void )
		{

			// Check if the structures have been properly matched
			if( fStructuralStack.size() != 0 )
			{
				fStructuralStack.clear();		// clear the stack
				throw ForthError( " Unmatched statement(s)" );
			}

		}




		virtual bool DecisionOnWordAlreadyExists( const Name & name )
		{
			// We can register a callback to be launched to ask the user
			GetOutStream() << "Warning: " << name << " overwrites the already existing word (can cause problems if that was used in other defs)\n";
			return true;		// ok to overwrite
		}

	public:



		// Parse & execute a stream of tokens
		void operator() ( Names && ns ) override
		{
			// Passing ns as && means "it is ok to dispose ns" and indeed we consume it

			const auto ns_size { ns.size() };

			if( ns_size == 0 )
				return;					// empty line is ok


			// Can be a call or a word definition
			const auto kMinTokens4Def { 3 };
			if( ns_size >= kMinTokens4Def && ns[ 0 ][ 0 ] == kColon && ns[ ns_size - 1 ][ 0 ] == kSemColon )
			{
				// A word definition
				EnterWordDefinition( std::move( ns ) );		// we have to use std::move since a named rvalue (ns in our case) is an lvalue!
			}												// since all named values are lvalues - the rule, "If it has a name, then it's an lvalue."
			else
			{
				Base::operator()( std::move( ns ) );		// call the base to execute the words (again, ns is an lvalue since it is named, so we need to call std::move)
			}


		}

	public:

		// Returns true if the postponed word is executed in a current compilation context.
		// Returns false if the postponed marked word is executed outside the : ; definition.
		virtual bool CompileInPostponedWord( const WordPtr w_ptr )
		{
			return fWordDefinitionContext_4_Postpone ? fWordDefinitionContext_4_Postpone->AddWord( w_ptr ) , true : false;
		}

	private:


		// This pointer is active (not null) only when executing immediate words during
		// compilation. Holds the current context where the postponed words should be
		// compiled into.
		CompoWordPtr	fWordDefinitionContext_4_Postpone { nullptr };	

	public:

		// It clears the data and return stacks - should be called in the catch 
		void CleanUpAfterRunTimeError( void ) override
		{
			Base::CleanUpAfterRunTimeError();					// the base will clear data and return stacks
			fStructuralStack.clear();							// clear the structural stack
		}


	};








	// ========================================================================
	// This operation requires prior knowledge of TForthCompiler - such a coupling 
	// is a trade off between virtual function in the TForth hierarchy
	// vs. dynamic cast in the rare cases. 


	// Do an exception ** when compiling an immediate word ** with some of its sub-words
	// indicated as POSTPONE, though. In such a case, these sub-words will be compiled-in,
	// rathern than being executed with the rest of the immediated word. 
	// However, if not in the "compiling immdediate" mode, then such a word will be simply executed.
	template < typename Base >
	void Postpone< Base >::operator () ( void )
	{
		assert( fWordPtr );
		assert( dynamic_cast< TForthCompiler * >( & GetForth() ) );			// no other contexts allowable (must be TForthCompiler)

		if( auto forth_compiler = dynamic_cast< TForthCompiler * >( & GetForth() ) )
			if( forth_compiler->CompileInPostponedWord( fWordPtr ) )		// just pass the stored postponed word
				return;

		// Default action is to execute the word - as with any other "normal" word
		( * fWordPtr )();
	}



	// ========================================================================





}	// The end of the BCForth namespace






