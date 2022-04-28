# hcc_c
A C Compiler implemented in C language.
https://zhuanlan.zhihu.com/p/506669546

# references
 1. C99 standard final draft http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf
 2. Dave Prosser's C Preprocessing Algorithm http://www.spinellis.gr/blog/20060626/
 3. 8cc Preprocessing Algorithm https://github.com/rui314/8cc/wiki/cpp.algo.pdf
 4. ANSI C Grammar, Lex specification https://www.lysator.liu.se/c/ANSI-C-grammar-l.html
 5. ANSI C Grammar production http://www.lysator.liu.se/c/ANSI-C-grammar-y.html#translation-unit
 6. C89 spec http://www.lysator.liu.se/c/rat/title.html
 7. https://blog.csdn.net/sheisc/article/list/1?t=1
 
# source tree 
	hcc
	  +--dist (final distribute compiler tools)
           +--include
           +--lib
           +--bin
		   +--examples
      +--doc (document of development)
      +--src
			+--basic (commone used codes)
			+--libcpp (lib of c preprocessor)
			+--libcc (lib of c compiler)
			+--tools 
				+-- common (common codes for tools)
				+-- hcpp (c preprocessor app)
				+-- hcc (c compiler app)
	  +--build (solution)

# pipeline
 1. lexer
 2. cpp
 3. parser
 4. intermediate code
 5. backend
 
# features
  1. using c99 as reference
  2. not support universal char name
  
