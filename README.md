# PREviaLCM

## Setup  
link files in `lib/CodeGen/*.cpp` to `llvm/lib/CodeGen/` folder (you need to use absolute path).

```
ln -s lib/CodeGen/\*.cpp llvm/lib/CodeGen
```


## Procedures
1. Get all terms `T`.
2. Get all variables `V`.
1. Get Partial Redundant Expressions. Get `T'`
2. Calculate D-Safe for each expression (term)
3. Calculate Earliest for each expression (term)
4. Perform the Safe-Earliest Transformation