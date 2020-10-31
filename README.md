<!-- markdownlint-disable MD033 MD013 -->

<p align="center"><img height="160px" src="./img/sood-logo.all-circles.svg" alt="sood-logo"/></p>

# Sood

Sood is a (very) minimal general purpose programming language based on some fairly verbose pseudo code I've used in the past to explain code to non-tech folks.

## Contents

- [The Concept](#the-concept)
- [The Compiler](#the-compiler)
  - [Installation](#installation)
  - [Usage](#usage)
  - [Compiler Outputs](#compiler-outputs)
- [Language Specification](#language-specification)
- [Language Support](#language-support)
- [Mini-Disclaimer](#mini-disclaimer)

## The Concept

The idea was to use full English sentences as code with as little syntactic grammar as possible (aside from grammar used in the average English sentence).

So no braces, few to no parentheses, no non-alpha characters used to define operations (such as `+`, `-`, `!`, etc.); just your standard everyday words meaning their standard everyday meanings.

Though this concept has been satisfactorily implemented in Sood, I cannot say it was one-hundred percent successful; Sood adheres to this "philosophy" better than any other language I can think of... but perhaps there's a good reason that other languages have not tried it.

There are of course other languages that use words and sentences to write code, but none I can find are just the words/sentences meaning exactly what they mean. For example, there is the [Rockstar programming language](https://github.com/RockstarLang/rockstar) where integer initialization may take the form:

```rockstar
(Rockstar)
Tommy was a big bad brother.
```

Meaning: declare and initialize the variable `Tommy` with the value `1337` where the value is determined by the length of the words following `was`, i.e. `#a == 1`, `#big == 3`, etc.

In Sood, this same statement would be:

```sood
# Sood
Tommy is an integer of value 1337.
```

Both are sentences, but the programmatic meaning is only obvious in Sood.

## Install

The project has a few dependencies

- [Bison](https://www.gnu.org/software/bison/) version: 3.7.2
- [Flex](https://github.com/westes/flex/) version: 2.6.4
- [LLVM](https://llvm.org/) version: 10.0.1
- [Spdlog](https://github.com/gabime/spdlog) version: 1.8.1

Should be simple enough, but I know it never goes down like...

```sh
cmake . && make
```

## Usage

CLI library in use is the wonderful, header-only, [CXXOpts](https://github.com/jarro2783/cxxopts).

```txt
Compiler for the Sood programming language
Usage:
  sood [OPTION...] positional parameters

  -h, --help                Show this help message
  -d, --debug               Enable debugging
  -a, --print-ast           Print generated AST to stdout
  -l, --print-llvm-ir       Print generated LLVM IR to stdout
  -V, --no-verify           Disable LLVM verification
  -R, --run-llvm-ir         Run module within the compiler
  -S, --stop-after-ast      Stop after generating the AST
  -C, --stop-after-llvm-ir  Stop after generating the LLVM IR
  -O, --stop-after-object   Stop after writing object file
  -o, --output arg          Output file name (default: a.sood.out)
```

In which, input file may either be specified as the value of the `-i` options, or, as the single positional parameter. If no input parameter is given, the compiler uses stdin.

And output (`-o`) is applied to whichever `stop-after-xxx` option is passed. Alternatively, this is the name of the resulting executable binary file.

## The Compiler

There have been a few iterations of the compiler. Initially, I was doing everything myself including lexing, parsing, and writing (very architecture dependent) binary. I finished the lexer, finished the parser, began to write the code generation... and then decided that it was too big a task for what is essentially, a toy language.

In looking at the various tools to help out with the AST -> executable phase, I found a couple of options:

- Transpilation
- Generating a byte-code of sort and developing a VM for it

It was only by looking into the second option that I recalled [LLVM](https://llvm.org/) from my uni days. A little more digging revealed that this would be the perfect system with which to write Sood - the code was already in C++, I had previous experience with LLVM and had written a few play-programs in LLVM's intermediate representation language, aptly named LLVM IR, and I remembered that I had wanted to do more with it.

This seemed the perfect fit the project and in finding this, I thought that perhaps the DIY approach to the other components was not the best either. This thought led me to two tools which I was sure I wouldn't use at the beginning of the project: [Flex](https://github.com/westes/flex/), a very popular lexer-generator; and [Bison](https://www.gnu.org/software/bison/) a very popular parser-generator.

I still thought that I would use my DIY lexer and parser, but after trying these latter two tools, I found that they were better and more capable versions of the modules I had written from scratch.

Perhaps I lose a few deep customizations in using existing tools to get the job done quickly and more efficiently, but I found that I was able to implement new ideas much much quicker than I could using the previous method. For example, a small change in the grammar (which hadn't been finalised early in the project) often took a lot of code re-routing probably due to poor design on my part. Compare that to just changing a line in the `.y` file and recompiling, it was a big time save.

### Compiler Outputs

The compiler has an option to output the resulting code/file at each stage of compilation from source to executable.

#### AST

The first task was to use the lexer and parser to generate an AST, this is a data structure representing each statmement and expression in the relevant order and within the relevant block.

As an example, the Sood code:

__Note__: Void functions do exist, but I made this an `integer` and return `0` just for slightly more to read in the outputs

```sood
# tests/helloworld-fn.sood

my_function is a function of type integer and of statements,
  write 'Hello world\n' to stdout.
  return 0...

my_function called with no arguments.
```

creates the AST:

```sood-ast
block: {
  func_decl {
    type: ident(integer),
    name: ident(my_function),

    block: {
      write { exp: str(Hello world\n), to: ident(stdout) }

      return { exp: int(0) }
    }
  }

func_call {
    id: ident(my_function)
  }
}
```

The command in use here is:

```sh
sood -S -o tests/helloworld-fn.sood-ast tests/helloworld-fn.sood
```

#### LLVM IR

As we are making use of the [LLVM C++ API](https://llvm.org/docs/ProgrammersManual.html), we are generating [LLVM IR](https://llvm.org/docs/LangRef.html), so that same file (`tests/helloworld-fn.vim`) would generate the LLVM IR:

```llvm-ir
; ModuleID = 'mod_main'
source_filename = "mod_main"

@numeric_fmt_spc = private unnamed_addr constant [3 x i8] c"%d\00", align
1
@string_fmt_spc = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@l_str = private unnamed_addr constant [13 x i8] c"Hello world\0A\00", ali
gn 1

declare i32 @printf(i8*, ...)

define void @main() {
entry:
  %_f_call = call i64 @my_function()
  ret void
}

define internal i64 @my_function() {
my_function__entry:
  %_printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds (
[3 x i8], [3 x i8]* @string_fmt_spc, i32 0, i32 0), i8* getelementptr inbo
unds ([13 x i8], [13 x i8]* @l_str, i32 0, i32 0))
  ret i64 0
}
```

Where the `printf` function is later linked from libc at compilation.

The command in use here is:

```sh
sood -C -o tests/helloworld-fn.ll tests/helloworld-fn.sood
```

__Note__: I know this isn't tremendous IR code, but it will hopefully be improved in time and it's functional right now.

#### Object

I'm obvious not going to dump some native object file to give as an example here, but the command to produce the file `tests/helloworld-fn.o` (the object file), would be:

```sh
sood -O -o tests/helloworld-fn.o tests/helloworld-fn.sood
```

### Executable

Finally is the native executable, this is the default function of the program and so the command to produce the executable for `helloworld-fn` is:

```sh
sood -o tests/helloworld-fn tests/helloworld-fn.sood
```

## Language Specification

- [Comments](#comments)
- [Variable Naming](#variable-naming)
- [Types](#types)
- [Statements](#statements)
- [Variable Declaration](#variable-declaration)
- [Assignment](#assignment)
- [Operations](#operations)
- [Function Declaration](#function-declaration)
- [Function Calls](#function-calls)
- [Control flow](#control-flow)
- [Input/Output](#inputoutput)

### Comments

Like BaSH, Python, Perl, etc. the hash character is used to denote a comment until the end of the line; though this does not quite fit the "English sentences meaning what the sentence means" motif, it was never the intention of the language for a source file to appear like an essay.

Although, I do kind of like that idea...

For example:

```sood
# This is a Sood comment
```

```sood
# This is a
#  block comment
#  in Sood
```

### Variable Naming

Variable naming is very C-like, underscores, and numerals are permitted, but a numeral is only permitted assuming the first charcater is an alpha or underscore character. E.g.

| Variable name | Acceptable?                                                     |
| :-            | :-                                                              |
| `my_variable` | Yes                                                             |
| `MyVariable`  | Yes                                                             |
| `_my_var`     | Yes                                                             |
| `_1`          | Yes                                                             |
| `123_var`     | No, starts with a numeral                                       |
| `my#var`      | No, hashes (and other grammatical characters) are not permitted |

#### Booleans

- `true` for truths

and you guessed it

- `false` for falsehoods

### Types

Currently, only three types are supported, these are:

- `integer`: A 64-bit signed integer
- `float`: A 64-bit floating-point number
- `string`: An array of characters, youre standard string

No aliases for these exist, they are what they are.

#### Integers

Integers (`integer`) are written simply as you might expect, so `1` would refer to the integer "one", and `23` to "twenty-three".

#### Floats

Much like integers, floats (`float`) are in the common notation "characteristic.mantissa", e.g. `12.898` for "twelve point eight-nine-eight".

#### Strings

Strings (`string`) can either be single quoted or double quoted, and are inherently multiline, so the string:

```sood
'
This is
a multiline string'
```

would be translated to `\nThis is\na multiline string`.

Very few escape characters are processed, mainly due to my laziness, these include `\n`, `\t`, and `\r`. Double-backslashes are also parsed, so `\\n` would come out as `\n` literally.

So, `this is a\nstring`, would evaluate to:

```txt
this is a
string
```

Where, `this is a\\nstring`, would evaluate to:

```txt
this is a\nstring
```

### Statements

Statements in Sood follow a sentence-like pattern, i.e. they are a series of phrases, occasionally separated by commas, ending with a full-stop (period).

Statement which take of a block of other statments, for example function declarations, begin with an incomplete sentence followed by a comma, are proceeded by the list of statements, and end with a double full-stop. So, the last statement's ending period and the two that form the end of the block, together form an ellipse.

Whitespace is ignored in the majority of the language (with the exception of within strings), so one may separate a statement, word by word, with spaces, tabs, and newline to whatever extent.

### Variable Declaration

All variables are zero-initialized, integers and floats take the value `0`, strings take the value `""`. Initilization is therefore not essential. Declaring a variable is as simple as giving a name and a type.

```sood
my_variable is a string.
```

#### Initialization

Should you wish to give a newly created variable an initialization value, the `of value` phrase can be used, e.g.

```sood
my_variable is a string of value "hello there".
```

This is equivalent to:

```sood
my_variable is a string.
my_variable is "hello there".
```

This also gives insight into assignments...

### Assignment

Once a variable is declared, assignment can be accomplished with the `is` keyword, denoting that a variable `is` this new value. E.g.

```sood
my_variable_that_has_been_declared is 98.
```

Assigning the value of `98` to that horrendously named variable.

### Operations

Operations are like the rest of the language, wordy. This is to avoid potentially confusing or misunderstood grammatic characters. So instead of the standard `<=`, there is `less than or equal to`.

These operations will perform some naive type-casting but only between integer and floating point.

__Note__: Sood supports various operations however at the time of writing very few operations will work on strings, so attempting to concatenate two string with the `plus` keyword will throw a syntax error; this is in the _todo_ pile.

#### Arithmetic

Examples evaluate to `3`.

| Operation      | Symbolic | Keyword/phrase  | Example             |
| :--            | :--      | :--             | :--                 |
| Addition       | `+`      | `plus`          | `1 plus 2`          |
| Subtraction    | `-`      | `minus`         | `4 minus 1`         |
| Division       | `/`      | `divided by`    | `6 divided by 2`    |
| Multiplication | `*`      | `multiplied by` | `1 multiplied by 3` |
| Modulus        | `%`      | `modulo`        | `7 modulo 4`        |

#### Boolean (Binary)

Examples evaluate to `true`.

| Operation        | Symbolic | Keyword/phrase             | Example                        |
| :--              | :--      | :--                        | :--                            |
| Equality         | `==`     | `is equal to`              | `2 is equal to 2`              |
| Inequality       | `!=`     | `is not equal to`          | `2 is not equal to 1`          |
| Less             | `<`      | `is less than`             | `1 is less than 2`             |
| Less or equal    | `<=`     | `is less than or equal to` | `1 is less than or equal to 2` |
| Greater          | `>`      | `is more than`             | `1 is more than 0`             |
| Greater or equal | `>=`     | `is more than or equal to` | `1 is more than or equal to 0` |
| And              | `&&`     | `also`                     | `true also true`               |
| Or               | `||`     | `alternatively`            | `false alternatively true`     |

I had used the word `and` too much already, hence the need for `also` in place of the convention. The `alternatively` in place of "or" is just because it made me laugh.

__Note__: I know "greater" is often preferred to "more" however that is not a preference I share.

#### Boolean (Unary)

Examples evaluate to `true`.

| Operation        | Symbolic | Keyword/phrase | Example               |
| :--              | :--      | :--            | :--                   |
| Not              | `!`      | `not`          | `not false`           |
| Additive inverse | `-`      | `negative`     | `negative negative 1` |

The double negation in the example is only to make it evaluate to true, i.e. `--1 == 1 == true`.

### Function Declaration

These take a very similar form to variable declaration with the exception of initialization which, for a function declaraction, is the code block of the function (obviously, this is not technically an initialization but it lexically takes the same role).

Argument lists begin with the phrase `with arguments of:` followed by arguments of the form `a <type> <identifer>`, e.g. `a string my_string`. This list ends with a semi-colon `;`.

The start of the block is denoted `and of statements,`. The block is then closed with two full-stops.

For example:

```sood
add_two is a function of type integer with arguments of: an integer n; and of statements,
  n is n plus 2.
  return n...
```

or, syntactically equivalent but perhaps more indentedly readable:

```sood
add_two is a function of type integer with arguments of:
    an integer n; and of statements,
  n is n plus 2.
  return n.
..
```

which some may prefer.

If a function shouldn't return a value, i.e. a `void` function, simply omit the `of type` phrase, for example:

```sood
add_two is a function with arguments of: an integer n; and of statements,
  write n plus 2 to stdout...
```

### Function Calls

Function calls were a tricky one to implement because by themselves they are expressions (as opposed to statements), the common statement form of these are in assignments, i.e. `b = func()` meaning assign the outcome of `func` to the variable `b`.

But, by using a statement wrapper for expressions, specifically the function call, they seem to work using the same syntax.

A function call begins with the function identifier followed by the keyword `called`. If no arguments are to be given, this is explicitly stated with the phrase `with no arguments`. If arguments are to be given, they are given as a comma separated list of expressions.

```sood
# Function call with no arguments
my_function called with no arguments.
```

```sood
# Function call with one argument (one expression)
my_function called with my_variable plus 2 as an argument.
```

```sood
# Function call with multiple arguments (multiple expressions)
my_function called with my_variable plus 2, my_other_variable, and 6 as arguments.

# or
my_function called with
    my_variable plus 2,
    my_other_variable,
    and 6 as arguments.
```

The `and` is not a requirement but does make more sense in my opinion.

### Control Flow

There are three core control-flow concepts in Sood, these are the `if` statement, the `while` statements, and the `while`'s inverse, the `until` statement.

Sood does not implement a `for` loop as this can be accomplished with `while`s and `if`s, perhaps this is a future addition if anyone requests it (if anyone sees this project (and uses it)).

These follow the already establish block notation, for example:

```sood
# Evaluate condition and perform block `if` true
if my_variable is less than 10,
  my_variable is my_variable plus one...
```

```sood
# Evaluate condition and perform block `while` condition is true
#  (`until` false)
while my_variable is less than 10,
  my_variable is my_variable plus one...
```

```sood
# Evaluate condition and perform block `until` condition is true
#  (`while` false)
until my_variable is equal to 9,
  my_variable is my_variable plus one...
```

With the latter two being syntactically equivalent. I've always like the idea of an `until` statement and find it quicker to grasp than the `while` equivalent, I assume this is just a personal preference.

__Note__: It only occurs to me at the time of writing this readme that the `break` and `continue` concepts for looping do not exist in the language, they are now in the _todo_ pile.

### Input/Output

IO is _very_ limited at this point in time, input straight up does not work due to lack of trying - in other words, I haven't got round to it yet (again, in the _todo_ pile).

Output is working but only to standard out and leverages [libc](https://www.wikiwand.com/en/GNU_C_Library)'s `printf` function.

The syntax of both input and output imply variable sources/sinks, however this is not the case for the language as it stands.

#### Input

__Note__: Not yet implemented

#### Output

Output takes the form:

```sood
write expression to stdout.
```

So, a hello world program in Sood would be:

```sood
write 'Hello world\n' to stdout.
```

__Note__: As previously mentioned, only standard out is available at the minute, the output sink _is_ required but _is not_ parsed. The above line is currently equivalent to:

```sood
write 'Hello world\n' to fish.
write 'Hello world\n' to a_basket_of_muffins.
write 'Hello world\n' to caterpillars.
```

## Text Editor Language Support

There is only support for the [Vim](https://www.vim.org/) editor as, realistically, it's the editor I use most often... well, it's the only editor I use.

A language plugin for Sood, for Vim, can be found [here](https://github.com/BodneyC/sood-vim).

It's adds syntax for `.sood` and `.sood-ast` files (see the [compiler outputs section](#compiler-outputs)). It also adds indent support for `.sood` files.

## Mini-Disclaimer

The idea of this language arose from the need in my current role to explain simple code to those who are new to programming and may not immediately understand what, for example, `a % b` means. And, Googling for "maths percent sign" yields far less contextually relevant results than, say, `a modulo b` and Googling "maths modulo meaning".

However, this language is only likely to be useful to English speakers. This wasn't out of any exclusionary desire, but it's my primary language (excluding VimL (don't @ me)) and it made sense for this iteration of Sood to be English focused.

I think it would be a wonderful idea for each natural language to have it's clear and literal formal counterpart, as Sood may be for English, but my polyglottony extends only as far as formal languages.
