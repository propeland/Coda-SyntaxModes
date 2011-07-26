Read me
=======
 
This is a Coda/SubEthaEdit syntax coloring mode for the CoffeeScript language.
It's plenty rough around the edges, but it's usable.
 
I mostly based it on the built-in JavaScript Mode.
Special thanks to brajeshwar's Sass.Mode and jashkenas' TextMate bundle;
I used them for reference while developing this mode.
 
 
Installation
============
 
- Download and unpack either the ZIP or TAR version of the CoffeeScript mode.
- Coda:
  Manually copy the CoffeeScript.mode directory into ~/Library/Application\ Support/Coda/Modes/.
- SubEthaEdit:
  Open the CoffeeScript.mode file from within SubEthaEdit using the File > Open command 
  or by dragging the CoffeeScript.mode file onto the SubEthaEdit application icon in the dock.
 
Extras
------------
Chendrix made an alternate [light-on-dark color scheme](https://github.com/chendrix/Specials-Board) for those who prefer the dark side.
 
Known issues
------------
- Comment bug:
  When you press Command + / on a line consisting of "###",
  Coda will throw a grumpy error, and suggest you restart it.
 
 
 
To-do
-----
- Choose colors
  I have been using colors haphazardly,
  mainly the same colors as the built-in Javascript mode.
  Need to choose colors in an intentional, designerly fashion.
 
 
 
- Colorize function arguments:
  Should we colorize args inside the function 
  the same as the function? Differently? treat as "default?"
 
- Colorize instance variables
  Make @vars stand out.
   
- Colorize boolean operators?
  Could distinguish if, else, unless, and, or, not, &&, ||, ==, <, >, ?, ?= from other types of operators
   
- Colorize =, :, or=
  These have the same appearance as the vars they come after.
  This is fine for now, but I may want to change the color slightly.
 
 
 
- Can we have single line auto-comment?
  Now, Command + '/' keyboard shortcut wraps a single line in "### ###".
  I would prefer it prepended single line comments with "#",
  and multi-line comments with "### ###"