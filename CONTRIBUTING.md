Hi,

for bug report, source packages dowload, binary packages and documentation
please take a little time to read the directives at http://dar.linux.free.fr/

If you wish to submit a patch or want to request a pull using GitHub, Welcome!
But please, follow the rules!
- respect the existing coding style (4 char margin, "{" and its corresponding
  "}" on the same column, no space between a function name and the parenthesis,
  spaces and new lines where it makes sens for readability, ...)
- most of dar code has been written long before C++11 was even a project, but
  new addition should follow C++14 as much as possible (A major code refresh
  has taken place for release 2.6.0 to leverage C++11/C++14 constructions
  though it did not worth doing absolutely all code without trace of C++98
  constructions... so pay attention to the existing code in dar/libdar)
- comment you code in particular when it does not obvious things
- if possible use /// doxygen inline documentation to document your prototypes,
  arguments and returned values. Doxygen \note are also welcome here.
- when an if/then condition does not need an "else" clause, add one anyway
  using a the SRC_BUG directive in this else clause, this will trigger and
  exception in case of bug (a so called "self reported bug").
- same thing whithin a switch{ } constructions always use a "default:" condition
  with invoking the SRC_BUG directive.
- portability is important, your code should work under Linux, FreeBSD,
  OpenSolaris, Cygwin/Windows, ... pay attention not to use system specific
  things unless escaped by compilation time directive (`#if HAVE_...` / #endif)
- be prepared to justify/defend/argue your proposal,
- be patient, code review of you code proposal depends of other developper's
  available time

In short: readability, robustness, portability, evolutivity, flexibility,
completness and documentation are the master keys dar/libdar has received since
2001 to design and implement dar/libdar. You code addition should follow these
criteria to be included into dar/libdar code.

Thanks for comprehension!
