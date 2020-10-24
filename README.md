<!-- markdownlint-disable MD033 MD013 -->

<p align="center"><img style="height:160px" src="./img/sood-logo.all-circles.svg" alt="sood-logo"/></p>

# Sood

Sood is a (very) minimal general purpose programming language based on some fairly verbose pseudo code I've used in the past to explain code to non-tech folks.

## The Concept

The idea was to use full English sentences as code with as little syntactic grammar as possible (aside from grammar used in the average English sentence).

So no braces, few to no parentheses, no non-alpha characters used to define operations (such as `+`, `-`, `!`, etc.); just your standard everyday words meaning their standard everyday meanings.

Though this concept has been satisfactorily implemented in Sood, I cannot say it was one-hundred percent successful; Sood adheres to this "philosophy" better than any other language I can think of... but perhaps there's a good reason that other languages have not tried it.

There are of course other languages that use words and sentences to write code, but none I can find that are just the words/sentences meaning exactly what they mean. For example, there is the [Rockstar programming language](https://github.com/RockstarLang/rockstar) where integer initialization may take the form:

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

## Language Specification

## The Compiler

There have been a few iterations of the compiler

### Install

#### Dependencies

### Usage

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

### Outputs

#### AST

#### LLVM IR

#### Object

## Text Editor Language Support

It is only Vim... as I only use Vim..., see [here](https://github.com/BodneyC/sood-vim).

## Mini-Disclaimer

The idea of this language arose from the need in my current role to explain simple code to those who are new to programming and may not immediately understand what, for example, `a % b` means. And, Googling for "maths percent sign" yields far less contextually relevant results than, say, `a modulo b` and Googling "maths modulo meaning".

However, this language is only likely to be useful to English speakers. This wasn't out of any exclusionary desire, but it's my primary language (excluding VimL (don't @ me)) and it made sense for this iteration of Sood to be English focused.

I think it would be a wonderful idea for each natural language to have it's clear and literal formal counterpart, as Sood may be for English, but my polyglottony extends only as far as formal languages.
