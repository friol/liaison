# My Liaison with code

Each year, solving Advent of Code is a sort of bet with myself. <br/><br/>
I started with plain Python in 2015, and went through the editions, learning new languages I should not have learnt like Rust (which unfortunately I didn't fully got familiar with) and struggling with the usual bugs and whistles that Advent of Code problems bring.<br/>
<br/>
This year, I kind of raised the bar, deciding, around the beginning of November, to write an interpreter for a new language, and then solving Advent of Code with it.<br/>
<br/>And so <b>Liaison</b> was born.<br/>
<br/>
I'm obviously not the first one to do that, and for sure Liaison is not the best language to solve Advent of Code, but as long as I went through the old AOC problems and solved them with my language, I felt more and more proud of myself.
<br/><br/>
Liaison is pretty similar to C; it's mono-source file (at the moment you can't include external source files) and, ad said, quite verbose.<br/>
The simplest Liaison program looks like this:<br/>
<br/>

```<br/>
fn main(params)
{
  print("Hello world!");
}
```

(you already know what this code does).<br/>
<br/>
Liaison also has some "advanced" (let's call them this way) features, like booleans, arrays (but not arrays of arrays, at the moment), dictionaries, "object" methods and properties (like s.lenght or arr.split("x")) and circuit breaking (returns breaking a while cycle or similar).<br/>
An [example program](https://github.com/friol/liaison/blob/master/examples/test.lia) (which by the way is the official Liaison test suite) is included, among other examples.<br/><br/>

Unfortunately, at the moment, Liaison hasn't some "basic" functionalities like "complex" arithmetic and logic expressions.<br/>
So you cannot write ```a=(a+1)*2;``` but you can always write ```a+=1; a*=2;``` (in more steps).
<br/><br/>
### Compiling
You should be able to compile the interpreter with Microsoft Visual Studio 2022. Just load the project and hit "compile solution".
<br/>
### My personal comments on this experience
As of this writing, we are at day 2 of Advent of Code, and I managed to solve both the days with Liaison (making small additions/fixes, like implementing the "find" function for strings and not only for arrays).
I think that this language absorbed my attention and thoughts quite a lot in the past weeks. At the end, writing a "serious" program like this is like being in a kind of trance but not for minutes or hours, but for days or weeks.


