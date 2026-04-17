# AUTOMATED-MANAGEMENT

**Automated-Management** is a C-based project designed to parse and tokenize a custom programming language with the `.cucpp` file extension. The program reads a `.cucpp` source file and generates an Abstract Syntax Tree (AST), outputting a colored structural representation directly to the terminal.

## Getting Started

### Building
The project includes a `Makefile` for easy compilation. Simply run:
```bash
make
```

### Usage
Run the generated executable with a `.cucpp` file as an argument. If no argument is provided, the program will explain its usage.
```bash
./<executable_name> <filename.cucpp>
```
*Note: Run with `--help` for additional usage information if implemented.*

## The `.cucpp` Language Syntax

The custom language parsed by this project supports basic programmatic constructs like functions, conditionals, loops, and raw statements.

### 1. File Extension
All source files must use the exact `.cucpp` extension to be parsed correctly.

### 2. Functions
Functions are declared using the `func` keyword, followed by the function name, its arguments enclosed in parentheses `()`, and the body enclosed in curly braces `{}`.

```cucpp
func my_function(arg1, arg2) {
    // body
}
```

### 3. Conditionals
Conditional blocks can be declared with the `if` or `ifelse` keywords.

```cucpp
if (x == 0) {
    // statement
}

ifelse (y > 10) {
    // statement
}
```

### 4. Loops
Loop blocks can be declared using the `while` or `loop` keywords.

```cucpp
while (x > 0) {
    x++
}

loop (i < 10) {
    i++
}
```

### 5. Statements
Any other code inside a block that does not match the keywords above is treated as a generic statement. Statements span until the end of the line or until a block delimiter (`{` or `}`) is encountered.

## Example File (`test.cucpp`)

```cucpp
func test(test, test, test)
{
	while (x > 0)
		x++
	if (x == 0)
		xaaaaaaaa
}
```

When parsed, the tokenizer will evaluate `func`, its `(test, test, test)` context, and associate the nested `while` and `if` nodes (and their containing statements) as its body, ultimately generating a clear structural AST output.
