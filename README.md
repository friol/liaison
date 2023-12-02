# My Liaison with code

Each year, solving [Advent of Code](https://adventofcode.com/) is a sort of bet with myself. <br/><br/>
I started with plain Python in 2015, and went through the editions, learning new languages I should not have learnt like Rust (which unfortunately I didn't fully got familiar with) and struggling with the usual bugs and whistles that Advent of Code problems bring.<br/>
<br/>
This year, I kind of raised the bar, deciding, around the beginning of November, to write an interpreter for a new language, and then solving Advent of Code with it.<br/>
<br/>And so <b>Liaison</b> was born.<br/>
<br/>
I'm obviously not the first one to do that, and for sure Liaison is not the best language to solve Advent of Code, but as long as I went through the old AOC problems and solved them with my language, I felt more and more proud of myself.
<br/><br/>
Liaison is pretty similar to C or Python; it's mono-source file (at the moment you can't include external source files) and, ad said, quite verbose.<br/>
The simplest Liaison program looks like this:
<br/>

```<br/>
fn main(params)
{
  print("Hello world!");
}
```

(you already know what this code does).<br/>
<br/>
Liaison also has some "advanced" (let's call them this way) features, like booleans, arrays (but not arrays of arrays, at the moment), dictionaries, methods and properties for "objects" (like s.lenght or arr.split("x")) and circuit breaking (return statements immediately exit from a function).<br/>

Unfortunately, at the moment, Liaison is missing some "basic" functionalities like arithmetic and logic expressions. So you cannot write ```a=(a+1)*2;``` but you can always write ```a+=1; a*=2;``` (in more steps).
<br/><br/>

### Code examples
An [example program](https://github.com/friol/liaison/blob/master/examples/test.lia) (which by the way is the official Liaison test suite) is included, among other examples (see the examples folder).<br/>
<br/>

### Compiling
The Liaison interpreter is written in C++17. You should be able to compile it with Microsoft Visual Studio 2022. Just load the project and hit "compile solution".
<br/><br/>
### My thoughts on the experience of developing Liaison
As of this writing, we are at day 2 of Advent of Code, and I managed to solve both of the days with Liaison (making small additions/fixes, like implementing the "find" function for strings and not only for arrays, and so on).<br/><br/>
I think that this project absorbed my attention and thoughts quite a lot in the past weeks. At the end, writing a "serious" program like this is like being caught in a kind of trance, not just for minutes or hours, but for days or weeks.
<br/><br/>
Writing code in Liaison is not an easy task, since the language is quite verbose and it's unfortunately missing some features. It's also, obviously, not enough tested against massive number crunching, so, at the end, doing Advent of Code with a recently born language is like going through a vast dark cave with only a small candle in your hand. But we'll see what kind of monsters AOC brings on.

