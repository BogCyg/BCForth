// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================



#pragma once



#include "Forth.h"
#include "SystemWords.h"
#include "StructWords.h"



namespace BCForth
{




	// Forth interpreter (parser)
	class TForthInterpreter : public TForth
	{

	protected:

		using Base = TForth;

		using Base::fWordDict;



	protected:

		// The output stream used by all output words, such as EMIT or TYPE
		std::ostream & fOutStream { std::cout };

	public:

		auto & GetOutStream( void ) { return fOutStream; }


	protected:


		const std::regex kIntVal	{ R"(([+-]?\d+))" };
		const std::regex kHexIntVal { R"((0[x|X][\da-fA-F]+))" };		// use the C++ syntax

		const std::regex kFloatPtVal { R"([+-]?(\d+[.]\d*([eE][+-]?\d+)?|[.]\d+([eE][+-]?\d+)?))" };		// always require a dot inside


		// A language cathegory in a word definition can be as follows:
		// - a word's name
		// - a reference to another word, i.e. already present in the dictionary
		// - a numerical value, either a character, an integer or the floating-point

		// There are built in words which need to be pre-defined by the Forth initializing code.
		// These are:
		// - basic arithmetic operations, +, -, *, /, */, 1+, 1-, etc.
		// - basic logical operators &, |, ^, not
		// - DUP, DROP, OVER, ROT and their "2" versions
		// - moves between the stacks: >R, <R, etc.
		// - IO operations: . (dot), .", "
		// - CONSTANT and VARIABLE

	public:

		EIntCompBase ReadTheBase( void ) override
		{
			if( auto base_word_entry = GetWordEntry( "BASE" ) )														// if exists, BASE is a Forth's variable
				if( auto * we = dynamic_cast< CompoWord< TForth > * >( (*base_word_entry)->fWordUP.get() ) )		// each Forth's word contains a CompoWord
					if( auto & compo_vec = we->GetWordsVec(); compo_vec.size() > 0 )								// The CompoWord should contain sub-words
						if( auto * byte_arr = dynamic_cast< RawByteArray< Base > * >( compo_vec[ 0 ] ) )			// For the variable this has to be RawByteArray
								return byte_arr->GetContainer()[ 0 ] == 16 ? EIntCompBase::kHex : EIntCompBase::kDec;


			return EIntCompBase::kDec;			// this is the default value
		}

	protected:

		auto Word_2_Integer( const Name & word )
		{
			try
			{
				return std::stoll( word, nullptr, ReadTheBase() );
			}
			catch( ... )
			{
				throw ForthError( "wrong format of the integer literal" );
			}
		}


		bool IsInteger( const Name & n, EIntCompBase expected_base )
		{
			switch( expected_base )
			{
				case kDec: return std::regex_match( n, kIntVal );
				case kHex: return std::regex_match( n, kHexIntVal );
				default: assert( ! "not supported formats" ); break;
			}

			return false;
		}


		bool IsFloatingPt( const Name & n )
		{
			return std::regex_match( n, kFloatPtVal );
		}



		void Erase_n_First_Words( Names & ns, const Names::size_type to_remove )
		{
			ns.erase( ns.begin(), ns.begin() + to_remove );
		}

	protected:


		auto ExtractTextFromTokenUpToSubstr( const Name & n, const Name::size_type pos, const Letter letter )
		{
			assert( n.length() > pos && n[ pos ] == letter );
			return n.substr( 0, pos );
		}



		// Allows for recursive (nested) enclosings
		auto CollectTextUpToTokenContaining( Names & ns, const Letter enter_letter, const Letter close_letter )
		{
			Name str;

			bool internal_mode { false };			// if false, then the closing letter causes exit
													// if true then disregard the closing letter and continue collection (for nested parenthesis, etc.)

			while( ns.size() > 0 )
			{
				const auto token { ns[ 0 ] };		// at each iteration take the first word
				Erase_n_First_Words( ns, 1 );		// immediately, remove from the stream

				if( const auto pos = ContainsSubstrAt( token, enter_letter ) ; pos != Name::npos )
				{
					// If here, then we encountered the nested definition (i.e. parenthesis inside parenthesis), so we need to match them
					if( const auto pos = ContainsSubstrAt( token, close_letter ); pos == Name::npos )
					{	
						// no closing symbol yet
						internal_mode = true;		// change internal mode only on unbalanced symbols
						str += token + kSpace;		// spaces were lost by the lexer, so we need to add it here
					}
					else
					{
						// a closing symbol found - split, left attach, right treat as a new token
						const auto & [ s0, s1 ] = SplitAt( token, pos + 1 );		
						str += s0;
						ns.insert( ns.begin(), s1 );	// insert new token to the stream
					}

				}
				else
				{

					if( const auto pos = ContainsSubstrAt( token, close_letter ) ; pos != Name::npos )
					{

						if( internal_mode )
						{
							internal_mode = false;
						}
						else
						{
							// " can be glued to the end of a text, so get rid of it and exit
							str += ExtractTextFromTokenUpToSubstr( token, pos, close_letter );
							return std::make_tuple< bool, Name >( true, std::move( str ) );
						}
					}


					str += token + kSpace;	// spaces were lost by the lexer, so we need to add it here

				}

			}

			return std::make_tuple< bool, Name >( false, "" );		// something went wrong, no ending quotation
		}




	protected:


		// Process and consume the processed words (i.e. these will be erased from the input stream)
		// ns will be modified
		virtual void ProcessContextSequences( Names & ns )
		{
			const auto kNumNames { ns.size() };
			if( kNumNames == 0 )
				return;

			auto & leadName { ns[ 0 ] };


			// FIND DROP
			if( leadName == "FIND" )
			{
				// There should be a following name for that word
				if( kNumNames <= 1 )
					throw ForthError( "Syntax missing word name" );

				if( auto word_entry = GetWordEntry( ns[ 1 ] ); word_entry )
					cout << "Word " << ns[ 1 ] << " found ==> ( " << ( * word_entry )->fWordComment << " )" << ( ( * word_entry )->fWordIsImmediate ? "\t\timmediate" : "" ) << endl;
				else
					cout << "Unknown word " << ns[ 1 ] << endl;

				Erase_n_First_Words( ns, 2 );
				return;
			}


			// ' DUP
			if( leadName == "'" )
			{
				// There should be a following name for that variable
				if( kNumNames <= 1 )
					throw ForthError( "Syntax missing variable name" );

				Tick< TForth >( * this, ns[ 1 ] )();

				Erase_n_First_Words( ns, 2 );
				return;
			}



			// This one goes like this:
			// 234 TO CUR_FUEL
			if( leadName == "TO" )
			{
				// There should be a following name for that variable
				if( kNumNames <= 1 )
					throw ForthError( "Syntax missing variable name" );

				To< TForth >( * this, ns[ 1 ] )();

				Erase_n_First_Words( ns, 2 );
				return;
			}


			// Parse the following word and put ASCII code of its first char onto the stack
			if( leadName == "CHAR" )
			{
				// There should be a following name for that variable
				if( kNumNames <= 1 )
					throw ForthError( "Syntax CHAR should be followed by a text" );

				GetDataStack().Push( BlindValueReInterpretation< Char >( ns[ 1 ][ 0 ] ) );

				Erase_n_First_Words( ns, 2 );
				return;
			}


			// Constructions to enter the counted string (i.e. len-chars), e.g.:
			// CREATE	AGH		,"	University of Science and Technology"
			if( leadName == kCommaQuote )
			{
				// Just entered the number of tokens up to the closing "

				Erase_n_First_Words( ns, 1 );

				if( auto [ flag, str ] = CollectTextUpToTokenContaining( ns, Letter(), kQuote ); flag )
					CommaQuote< TForth >( * this, str )();		// create & call
				else
					throw ForthError( "no closing \" found for the opening ,\"" );

				return;
			}


			// CREATE DATA  100 CHARS ALLOT
			// CREATE CELL-DATA  100 CELLS ALLOT
			// CREATE TWOS 2 , 4 , 8 , 16 ,
			if( leadName == "CREATE" )
			{
				leadName = ns[ 0 ] = "[CREATE]";	// just exchange CREATE into [CREATE] and business as usual
			}



		}


		// The main entry to the Forth's INTERPRETER
		virtual void ExecuteWords( Names && ns )
		{

			ProcessContextSequences( ns );

			const auto kNumNames { ns.size() };
			if( kNumNames == 0 )
				return;

			const auto & word { ns[ 0 ] };


			if( IsInteger( word, ReadTheBase() ) )
			{
				GetDataStack().Push( Word_2_Integer( word ) );
				Erase_n_First_Words( ns, 1 );	// get rid of the already consumed word
				ExecuteWords( std::move( ns ) );
				return;
			}


			if( IsFloatingPt( word ) )
			{
				GetDataStack().Push( BlindValueReInterpretation< CellType >( stod( word ) ) );		// for now, later we need to devise something better for floats
				Erase_n_First_Words( ns, 1 );	// get rid of the already consumed word
				ExecuteWords( std::move( ns ) );
				return;
			}


			if( ProcessDefiningWord( word, ns ) )
			{
				Erase_n_First_Words( ns, 2 );	// get rid of the already consumed words
				ExecuteWords( std::move( ns ) );
				return;
			}


			// Check if a registered word and execute
			if( ExecWord( word ) )
			{
				Erase_n_First_Words( ns, 1 );	// get rid of the already consumed word
				ExecuteWords( std::move( ns ) );
				return;
			}
			else
			{
				throw ForthError( "unknown word - " + word );
			}

		}


	protected:


		// The one created with DOES>
		virtual bool ProcessDefiningWord( const Name & word_name, const Names & ns )
		{
			// First, check if this is a defining word
			if( auto word_entry = GetWordEntry( word_name ); word_entry && (*word_entry)->fWordIsDefining )
			{
				if( auto * we = dynamic_cast< CompoWord< TForth > * >( (*word_entry)->fWordUP.get() ) )
				{

					if( auto & compo_vec = we->GetWordsVec(); compo_vec.size() == 1 )
					{
						if( auto * does_wrd = dynamic_cast< DOES< TForth > * >( compo_vec[ 0 ] ) )
						{
							// Ok, this is the DOES> word

							// There should be a following name for that variable
							if( ns.size() <= 1 )
								throw ForthError( "Syntax missing variable name for the defining word" );


							// Call the creation branch - this should leave 
							// (i) some values on the stack
							// (ii) new RawByteArray in the local repository due to CREATE
							( * does_wrd )();


							// The RawByteArray should be already in the fNodeRepo, so let's access it and verify its identity
							if( fNodeRepo.size() == 0 )
								throw ForthError( "missing CREATE action in the defining word" );	

							auto & array_wp = fNodeRepo[ fNodeRepo.size() - 1 ];		// let's access - this will be WordUP

							auto * arr_wrd = dynamic_cast< RawByteArray< TForth > * >( array_wp.get() );
							if( arr_wrd == nullptr )
								throw ForthError( "missing CREATE action in the defining word" );	


							// Create a new entry with connected behavioral branch
							auto definedWord { std::make_unique< CompoWord< TForth > >( * this ) };
							auto definedWordPtr { definedWord.get() };

							definedWordPtr->AddWord( arr_wrd );								// (1) Connect the RawByteArray word - whenever called it will leave the address of its data 
							if( ! IsEmpty( does_wrd->GetBehaviorNode() ) )
								definedWordPtr->AddWord( & does_wrd->GetBehaviorNode() );	// (2) Connect the behavioral branch, as already pre-defined in the defining word

							InsertWord_2_Dict( ns[ 1 ], std::move( definedWord ) );		// Now we have fully created new word in the dictionary		

							return true;
						}


					}


				}
				
				

				assert( false );		// should not happen, all words are at least of CompoWord type or derived


			}

			return false;		// no, this is not a defining word 
		}


		// Destructor will be created automatically virtual due to declaration in the base class



	public:



		// Process a stream of tokens
		virtual void operator() ( Names && ns )
		{
			ExecuteWords( std::move( ns ) );
		}

	public:

		// It clears the data and return stacks - should be called in the catch 
		virtual void CleanUpAfterRunTimeError( void )
		{
			GetDataStack().clear();
			GetRetStack().clear();	
		}


	};




}	// The end of the BCForth namespace






