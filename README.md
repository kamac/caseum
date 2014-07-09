CASEUM LANGUAGE
======

About
------
Caseum is a quite basic language. It's original reason for development was to simply learn how to code a lexer, parser & some simple code generator from scratch. Development started in december, 2013 by kamac. If it's updated, then only from time-to-time, as this is a very much of an educational side-project.

If you want, you can have a peek at how does the language syntax look like. Just browse the examples/ folder.

Compiling
------
The compiler at the moment compiles to native executables. The parameters when invoking it are as follows:
```
caseum source [additionalLibs] [options]
```
[additionalLibs] - for example: libmsvcrt.a libkernel32.a
[options] - currently only output file. Supporting -osomeName.exe and -osomeName.o

For example, this is how one would compile the helloworld example program:
```
caseum ../examples/helloworld/main
```
The executable is always placed next to the source file.

Platform support
------
Caseum supports only i386 architecture (x86). (At the moment)

OS support
------
Sorted by priority:
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
