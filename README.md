# my Liaison with code

Each year, solving Advent of Code is a sort of bet with myself. <br/>
I started with plain Python in 2015, and went through the editions, learning new languages I should not have learnt like Rust (which unfortunately I didn't fully got familiar with) and struggling with the usual bugs and whistles that Advent of Code problems bring.<br/>
<br/>
This year, I kind of raised the bar, deciding, around the beginning of November, to write an interpreter for a new language, and then solving Advent of Code with it. And so Liaison was born.<br/>
I'm obviously not the first one to do that, and for sure Liaison is not the best language to solve Advent of Code, but as long as I reviewed the old AOC problems and solved them with Liaison, I felt more and more proud of myself.
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
Liaison also has some "advanced" (let's call them this way) features, like booleans, arrays (but not arrays of arrays, at the moment), dictionaries, "object" methods and properties (like s.lenght or s.split("x")).

