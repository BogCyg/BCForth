----------------------------------------------------------------------
BCForth is a header-only pure C++ implementation of the Forth language
MIT license
----------------------------------------------------------------------

It was written by Prof. Bogusław Cyganek (C) in 2021 as an exemplary 
project extending the book:
"Introduction to Programming with C++ for Engineers", Wiley.

An in-depth description of the BCForth platform has been published  
as a 4th lecture note to the above book. It can be accessed at:

http://home.agh.edu.pl/~cyganek/Forth_BC_v013.pdf
----------------------------------------------------------------------



Directory/file structure of the BCForht project:
----------------------------------------------------------------------
[+]"add_ons"
   |--"AddOns.txt"
[+]"build"
   |--"Info.txt"
[+]"doc"
   |--"ReadMe.txt"
[+]"examples"
   |--"AD_Measurement.txt"
   |--"ErathoSieve.txt"
   |--"ErathoSieveEx.txt"
   |--"Factorial.txt"
   |--"Fibo.txt"
   |--"ForthExamples.txt"
   |--"ForthExamples_2.txt"
   |--"Postpone.txt"
   |--"QuadEq.txt"
   |--"QuadEqVars.txt"
   |--"test.txt"
   |--"VectoredExecution.txt"
[+]"icon"
   |--"BCForth.bmp"
   |--"BCForth.tif"
   |--"BCForth2.tif"
[+]"include"
   [+]"Auxiliary"
      |--"TheStack.h"
   |--"BaseDefinitions.h"
   |--"Forth.h"
   |--"ForthCompiler.h"
   |--"ForthInterpreter.h"
   [+]"Interfaces"
      |--"Interfaces.h"
      |--"Tokenizer.h"
   [+]"Modules"
      |--"CoreModule.h"
      |--"FP_Module.h"
      |--"Modules.h"
      |--"RandModule.h"
      |--"StringModule.h"
      |--"TimeModule.h"
   [+]"Words"
      |--"StructWords.h"
      |--"SystemWords.h"
      |--"Words.h"
[+]"src"
   |--"main.cpp"
|--"CMakeLists.txt"

(dir tree obtained by calling the C++ RecursivelyTraverseDirectory 
function, code of the book: https://github.com/BogCyg/BookCpp)


"add_ons" contains the AddOns.txt with word definitions that Forth 
compiles when launched; This is an easy way to add some custom words 
to be inserted into the dictionary.

"examples" contains some files to illustrate the most common 
features of Forth.



----------------------------------------------------------------------
To build BCForth do as follows:

1. Change your current directory to 
build

3. To build the project type 
cmake ..

4. Go to the build directory and launch your project/compilation

5. BCForth has been succesively compiled in the following setups:
(a) Microsoft Visual C++ 2019 Version 16.9.6, Windows 10, Dell Precision 7730
(b) g++ 10.1.0, Linux Ubuntu 18.04 and 20, PC workstation

In CMakeLists.txt by default a debug version is set.
set( CMAKE_BUILD_TYPE Debug )

To build a release version uncomment the Release settings
#set( CMAKE_BUILD_TYPE Release )


----------------------------------------------------------------------






