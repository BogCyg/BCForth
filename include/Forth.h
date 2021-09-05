// ========================================================================
//
// The Forth interpreter-compiler by Prof. Boguslaw Cyganek (C) 2021
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ========================================================================


#pragma once


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <string>
#include <algorithm>
#include <concepts>
#include <optional>
#include <tuple>
#include <iterator>
#include <functional>
#include <fstream>


#include "Words.h"








namespace BCForth
{





	// Space for the basic Forth's data structures
	class TForth
	{
	public:

		virtual ~TForth() = default;

	public:

		static const size_t kStackMaxCells { 64 };	// change for low memory systems

		using DataStack = ForthStackFor< CellType, kStackMaxCells >;

		using RetStack = TStackFor< CellType, kStackMaxCells >;

	public:

		DataStack &	GetDataStack( void ) { return fDataStack; }			

		RetStack &	GetRetStack( void ) { return fRetStack; }	


	public:

		using WordPtr = TWord< TForth > *;

		using WordUP = std::unique_ptr< TWord< TForth > >;

	protected: 

		// The main record to store all information about each word in the system
		// These are held in the fWordDict
		struct WordEntry
		{
			WordUP	fWordUP;
			bool	fWordIsCompiled		: 1		{ false };		// set if a word is currently compiled 
			bool	fWordIsImmediate	: 1		{ false };		// set if a word is immediate (executed during compilation of other words)
			bool	fWordIsDefining		: 1		{ false };		// set if a word contains DOES> in its definition
			Name	fWordComment;								// commenting text of this word
			// reserved for further data
		};

		using WordOptional = std::optional< WordEntry * >;


		using WordDict = std::unordered_map< Name, WordEntry >;		// for low memory systems change to std::map



	protected:


		DataStack		fDataStack;			// the main Forth's data structure

		RetStack		fRetStack;			// the second stack, called a "return" stack in Forth frameworks
											// (not used, left only for user's convenience)

		WordDict		fWordDict;			// a dictionary with all Forth's words


	protected:


		using NodeRepo = std::vector< WordUP >;

		NodeRepo fNodeRepo;		// stores all word nodes that do not go to the dictionary, such as compiled in numerals, etc.


	public:

		WordPtr Insert_2_NodeRepo( WordUP wp )
		{
			auto wptr { wp.get() };
			fNodeRepo.push_back( std::move( wp ) );
			return wptr;
		}


		const NodeRepo & GetNodeRepo( void ) const { return fNodeRepo; }

	public:

		WordDict &	GetWordDict( void ) { return fWordDict; }


		// Returns false if the word already exists
		WordPtr InsertWord_2_Dict( Name name, WordUP wp, Name comment_str = "", bool compiled = false, bool immediate = false, bool defining = false )
		{
			WordPtr retPtr { wp.get() };
			fWordDict[ name ] = WordEntry( std::move( wp ), compiled, immediate, defining, comment_str );
			return retPtr;
		}

	public:

		// Get the word's entry but the word can be not present
		auto GetWordEntry( const Name & word_name )
		{

			if( const auto & word = fWordDict.find( word_name ); word != fWordDict.end() )
				return WordOptional( & word->second );
			else
				return WordOptional();
		}



		// Returns true if a word was found and executed
		virtual bool ExecWord( const Name & word_name )
		{
			auto word = GetWordEntry( word_name );
			return word ? ( * ( (*word)->fWordUP ) )(), true : false;
		}


		virtual EIntCompBase ReadTheBase( void )
		{
			return EIntCompBase::kDec;		// in the base only this, will be extended in the interpreter
		}

	};




}	// The end of the BCForth namespace






