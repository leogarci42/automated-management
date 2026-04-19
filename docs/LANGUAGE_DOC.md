# Cucpp Language Features

- `compute` (functions) --> implemented
- function parameters --> implemented
- `return` --> implemented
- variables / assignments (`x = 12`) --> implemented
- variable shadowing (`x2 = x1 - 1`) --> implemented
- `if` / `ifelse` --> implemented
- math subtraction (`-`) and addition (`+`) --> implemented
- math multiplication (`*`) --> implemented
- comparisons (`>`, `<`, `==`) --> implemented
- `print(var)` built-in --> implemented
- `while` loops --> not implemented (Explanation: To loop, you must use tail-recursion. Create a `compute` block that takes the loop state as parameters, and returns a call to itself with updated parameters until a base `if` condition returns the final value).
- `for` loops --> not implemented (Explanation: Use tail-recursion just like you would for a `while` loop).
- `break` --> not implemented (Explanation: Since there are no standard loop constructs, use `return` to exit a recursive `compute` block early).
- `continue` --> not implemented (Explanation: Instead of `continue`, call the recursive `compute` block again with the next iteration's arguments).
- complex math (`/`, `%`, bitwise) --> not implemented
- logical conditions (`&&`, `||`, `!`) --> not implemented
- explicit data types (`bool`, `char`, `string`, floats) --> not implemented (All variables are 32-bit integers)
- arrays, structs, pointers --> not implemented (Omitted to preserve strict SSA semantics without memory allocations)
