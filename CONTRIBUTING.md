Hi,

for bug report, source packages dowload, binary packages and documentation please take a little time to read the directives at http://dar.linux.free.fr/

If you wish to submit a patch or want to request a pull using GitHub, Welcome! but please, follow the rules!
- respect the existing coding style
- most of dar code has been written long before C++11 was even a project, but new addition should follow C++14 as much as possible (A major code upgrade/review is planed to move toward C++14, though dar/libdar already compiles as C++11 code).
- comment you code when what it does is not obvious
- if possible use /// doxygen inline documentation to document your prototypes, arguments and returned values 
- when a if/then condition does not need an "else" clause, add one anyway with the SRC_BUG; directive in it, this will trigger and exception in case of bug (a so called "self reported bug").
- same thing whithin a switch{ } directive use the default: condition with a SRC_BUG; directive here too, and enumerate all the other possible values (if that makes sens of course).
- portability is important, your code should work under Linux, FreeBSD, OpenSolaris, Cygwin/Windows, ...
- be prepared to justify/defend/argue your proposal, I the case I could not see the value it brings :-)
- be patient, I will review your code proposal

In short: readability, robustness, portability, evolutivity, flexibility, completness and documentation are the master keys I've been following since 2001 to design and implement dar/libdar, I just hope you can understand that.

Thanks!
