CASEUM LANGUAGE
======

About
------
Caseum is an educational project, made with intent on deepening knowledge about how real languages work. And for fun.

Examples
------

Here's how recursive fibonacci implementation looks like in caseum

```
import "../include/stdio"

#[int]fib([int]value) {
	if(value < 2) {
		return 1
	} else {
		return fib(value-1) + value
	}
}

#[int]main() {
	[int]value = 4
	printf("fib(%i) = %i", value, fib(value))
}
```

More examples can be found in "examples" folder.

Compiling
------
Caseum code compiles to FASM assembly, and from there to an executable. Here are the arguments when running the compiler:
```
caseum source [additionalLibs] [options]
```
[additionalLibs] - Libraries to link. For example: libmsvcrt.a libkernel32.a  
[options] - Currently only output file. Supporting -osomeName.exe and -osomeName.o  
  
For example, this is how one would compile the helloworld example:
```
caseum ../examples/helloworld/main libmsvcrt.a -ohelloworld.exe
```
The executable is always placed next to the source file.

If you want to run the compiler, you need to create a bin/ folder at the root of this project, and place following files in there:
ld.exe - a linker
FASM/FASM.exe - FASM compiler

Platform support
------
Caseum supports only i386 architecture (x86). (At the moment)

OS support
------
- [x] Windows
- [ ] Linux
- [ ] MacOS X
 

TODO list
------
- [ ] Add double type
- [ ] Add while & do...while loops
- [ ] Introduce some basic sort of garbage collecting
- [ ] Write some basic libraries (that'd wrap functions like printf, file IO, etc.)
- [ ] Add pointers (?)
- [ ] Add custom types
- [ ] Enhance error reporting (also, general parser enhancements)
- [ ] Introduce some sort of optimizer (?)
- [x] Allow to generate .o files & reuse them within other apps
- [ ] Add some more examples, exposing all the features
- [ ] Enhance library system (?)
