# PREviaLCM

## Setup  
1. Clone repo into lib/Transforms/ in llvm source
2. Add following into lib/Transforms/CMakeLists.txt
```
add_subdirectory(PREviaLCM)
```


## Procedures
1. Get all terms `T`.
2. Get all variables `V`.
3. Get Partial Redundant Expressions. Get `T'`
4. Calculate D-Safe for each expression (term)
5. Calculate Earliest for each expression (term)
6. Perform the Safe-Earliest Transformation
