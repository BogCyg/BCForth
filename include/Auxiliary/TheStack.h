// ==========================================================================
//
// Software written by Boguslaw Cyganek (C) to be used with the book:
// INTRODUCTION TO PROGRAMMING WITH C++ FOR ENGINEERS
// Published by Wiley, 2020
//
// The software is supplied as is and for educational purposes
// without any guarantees nor responsibility of its use in any application. 
//
// ==========================================================================



#pragma once



#include <cassert>




namespace BCForth
{


	// ==============================================================================================

	// Definition of the base stack data structure
	template < typename T, auto MaxElems >
	class TStackFor
	{
	public:

		using value_type = T;
		enum { kMaxSize = MaxElems };

		// Here we need an additional typename
		using size_type = BCForth::size_type;

	protected:

		size_type							fStackPtr {};		// indicates the first free cell

		std::unique_ptr< value_type [] >	fData;



	public:

		constexpr size_type size() const { return fStackPtr; }


		constexpr T * data() const { return fData.get(); }


		constexpr void clear() { fStackPtr = 0; }

	public:

		constexpr TStackFor()
			: fStackPtr( 0 )
		{
			fData = std::make_unique< value_type [] >( kMaxSize );
		}


		// We don't need to explicitly disable copying (i.e. the assignment and the copy constructor), 
		// since std::unique_ptr will force that this class can be moved but not copied

	public:



		///////////////////////////////////////////////////////////
		// This function puts an element onto the stack
		///////////////////////////////////////////////////////////
		//		
		// INPUT:
		//			new_elem - a reference to the element to
		//				be put. Actually its copy is put onto
		//				the stack.
		//		
		// OUTPUT:
		//			true - if operation successful,
		//			false - failure, due to insufficient
		//				space on the stack (e.g. too many 
		//				elements)
		//		
		// REMARKS:
		//		
		//		
		constexpr bool Push( const value_type & new_elem )
		{
			return fStackPtr < kMaxSize ? fData[ fStackPtr ++ ] = new_elem, true : false;
		}

		// Move semantics version
		constexpr bool Push( value_type && new_elem )
		{
			return fStackPtr < kMaxSize ? fData[ fStackPtr ++ ] = new_elem, true : false;
		}


		///////////////////////////////////////////////////////////
		// This functions takes and removes an element from the stack
		///////////////////////////////////////////////////////////
		//		
		// INPUT:
		//			ret_elem - a reference to the object which
		//				will be copied with a topmost element
		//				from the stack. Then the topmost element
		//				is removed from the stack.
		//		
		// OUTPUT:
		//			true - if operation successful,
		//			false - failure, due to empty stack
		//		
		// REMARKS:
		//		
		constexpr bool Pop( value_type & ret_elem )
		{
			return fStackPtr > 0 ? ret_elem = std::move( fData[ -- fStackPtr ] ), true : false;		// if possible, "steal" from the top object, rather than copy (a fallback still possible)
		}



		///////////////////////////////////////////////////////////
		// This functions copies the topmost element not removing
		// it from the stack.
		///////////////////////////////////////////////////////////
		//		
		// INPUT:
		//			ret_elem - a reference to the object which
		//				will be copied with a topmost element
		//				from the stack. 
		//		
		// OUTPUT:
		//			true - if operation successful,
		//			false - failure, due to empty stack
		//		
		// REMARKS:
		//	
		constexpr bool Peek( value_type & ret_elem ) const
		{
			return fStackPtr > 0 ? ret_elem = fData[ fStackPtr - 1 ], true : false;		// here we need a strong copy
		}

	};









	// ---------------------------------------------------------------------------------------------
	// These are time critical operations therefore they are defined as close the stack as possible.
	// These are implemented in the series of the following mixin classes.
	// All others can be implemented with the general interface using exlusively Push and Pop.





	template < typename Base >
	class MSystemWordsStackFor : public Base
	{
	public:

		using BaseClass = Base;

		using BaseClass::Push;
		using BaseClass::Pop;
		using BaseClass::Peek;

	public:

		using T = BaseClass::value_type;
		using BaseClass::size_type;
		using BaseClass::kMaxSize;

	protected:

		using BaseClass::fData;							// kind of a concept, i.e. the base class must have this 
		using BaseClass::fStackPtr;

	public:


		// Forth specific words - defined here for performance
		constexpr bool Drop()
		{
			return fStackPtr > 0 ? -- fStackPtr, true : false;		
		}

		constexpr bool Dup()
		{
			return fStackPtr > 0 ? Push( fData[ fStackPtr - 1 ] ) : false;
		}

		constexpr bool Over()
		{
			return fStackPtr > 1 ? Push( fData[ fStackPtr - 2 ] ) : false;
		}

		constexpr bool Swap() const
		{
			if( fStackPtr >= 2 )
			{
				auto top { fData[ fStackPtr - 1 ] };
				fData[ fStackPtr - 1 ] = fData[ fStackPtr - 2 ];
				fData[ fStackPtr - 2 ] = top;
				return true;
			}
			return false;
		}

		constexpr bool Rot() const
		{
			if( fStackPtr >= 3 )
			{
				auto top { fData[ fStackPtr - 3 ] };
				fData[ fStackPtr - 3 ] = fData[ fStackPtr - 2 ];
				fData[ fStackPtr - 2 ] = fData[ fStackPtr - 1 ];
				fData[ fStackPtr - 1 ] = top;
				return true;
			}
			return false;
		}

		constexpr bool Cells()		{ return fStackPtr > 0 ? fData[ fStackPtr - 1 ] *= sizeof( T ), true : false; }

		constexpr bool CellPlus()	{ return fStackPtr > 0 ? fData[ fStackPtr - 1 ] += sizeof( T ), true : false; }



		// @
		template < typename Type2Read >
		constexpr bool ReadAt()
		{
			return fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( * reinterpret_cast< Type2Read * >( fData[ fStackPtr - 1 ] ) ), true : false;
		}

		// !
		template < typename Type2Write >
		constexpr bool WriteAt()
		{
			return fStackPtr > 1 ? * reinterpret_cast< Type2Write * >( fData[ fStackPtr - 1 ] ) = BlindValueReInterpretation< Type2Write >( fData[ fStackPtr - 2 ] ), fStackPtr -= 2, true : false;
		}

		// C+!
		template < typename Type2Write >
		constexpr bool UpdateAt()
		{
			return fStackPtr > 1 ? * reinterpret_cast< Type2Write * >( fData[ fStackPtr - 1 ] ) += BlindValueReInterpretation< Type2Write >( fData[ fStackPtr - 2 ] ), fStackPtr -= 2, true : false;
		}


	};








	template < typename Base >
	class MLogicalOpsStackFor : public Base
	{
	public:

		using BaseClass = Base;

		using BaseClass::Push;
		using BaseClass::Pop;
		using BaseClass::Peek;

	protected:

		using BaseClass::fData;							// kind of a concept, i.e. the base class must have this 
		using BaseClass::fStackPtr;


	public:

		constexpr bool And()	{ return fStackPtr > 1 ? fData[ fStackPtr - 2 ] &= fData[ fStackPtr - 1 ], -- fStackPtr, true : false; }
		constexpr bool Or()		{ return fStackPtr > 1 ? fData[ fStackPtr - 2 ] |= fData[ fStackPtr - 1 ], -- fStackPtr, true : false; }
		constexpr bool Xor()	{ return fStackPtr > 1 ? fData[ fStackPtr - 2 ] ^= fData[ fStackPtr - 1 ], -- fStackPtr, true : false; }
		constexpr bool Neg()	{ return fStackPtr > 0 ? fData[ fStackPtr - 1 ] = ~ fData[ fStackPtr - 1 ], true : false; }

	};











	template < typename Base >
	class MArithmeticOpsStackFor : public Base
	{

	public:

		using BaseClass = Base;

		using BaseClass::Push;
		using BaseClass::Pop;
		using BaseClass::Peek;

	public:

		using T = BaseClass::value_type;
		using BaseClass::size_type;
		using BaseClass::kMaxSize;

	protected:

		using BaseClass::fData;							// kind of a concept, i.e. the base class must have this 
		using BaseClass::fStackPtr;


	public:


		template < typename A >
		constexpr bool Plus()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) + top );
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool Minus()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) - top );
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool Mult()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) * top );
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool Div()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				if( top == A( 0 ) )
					throw ForthError( "div by 0" );
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) / top );
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool Mod()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				if( top == A( 0 ) )
					throw ForthError( "div by 0" );
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) % top );
				return true;
			}

			return false;
		}

	public:


		// Be careful - the returned value indicates a success of an operation, which is true if there is enough data on the stack
		// However, the comparison result goes to the topmost cell of the stack !! (i.e. this is NOT what is returned!)

		template < typename A >
		constexpr bool EQ()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) == top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool NE() 
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) != top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool LT() 
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) < top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool LE()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) <= top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool GT()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) > top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool GE()
		{
			if( fStackPtr > 1 )
			{
				auto top { BlindValueReInterpretation< A >( fData[ -- fStackPtr ] ) };
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) >= top ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}




		// Spec for 0

		template < typename A >
		constexpr bool EQ_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) == static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool NE_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) != static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool GT_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) > static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool GE_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) >= static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool LT_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) < static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}

		template < typename A >
		constexpr bool LE_0()
		{
			if( fStackPtr > 0 )
			{
				fData[ fStackPtr - 1 ] = BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) <= static_cast< A >( 0 ) ? kBoolTrue : kBoolFalse;
				return true;
			}

			return false;
		}



	public:

		// Forth extras - here for speed-up

		template < typename A >
		constexpr bool OnePlus()
		{
			return  fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) + static_cast< A >( 1 ) ), true : false;
		}

		template < typename A >
		constexpr bool OneMinus()
		{
			return  fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) - static_cast< A >( 1 ) ), true : false;
		}

		template < typename A >
		constexpr bool TwoPlus()
		{
			return  fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) + static_cast< A >( 2 ) ), true : false;
		}

		template < typename A >
		constexpr bool TwoMinus()
		{
			return  fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) - static_cast< A >( 2 ) ), true : false;
		}

		template < typename A >
		constexpr bool TwoTimes()
		{
			return  fStackPtr > 0 ? fData[ fStackPtr - 1 ] = BlindValueReInterpretation< T >( BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) + BlindValueReInterpretation< A >( fData[ fStackPtr - 1 ] ) ), true : false;
		}


	};








	// Assembly them all together 
	template < typename T, auto MaxElems >
	using ForthStackFor = MSystemWordsStackFor< MLogicalOpsStackFor< MArithmeticOpsStackFor< TStackFor< T, MaxElems > > > >;








}		// End of the BCForth namespace






// ----------------------------------------------------


