# Lock-Free_Internal_BST
Implementation of the Fast Lock-Free Internal Binary Search Tree for COP 4520 final.

## Compiling

To compile the code use g++:

```
g++ testLockFreeBST.cpp LockFreeBST.cpp header.cpp subFunctions.cpp bitManipulations.cpp util.cpp
```

## Testing

To run the test, pass in the command line arguments as such:

```
a numOfThreads readPercentage insertPercentage deletePercentage durationInSeconds maximumKeySize initialSeed
```

for example:

For Windows:
```
a.exe 64 90 9 1 5 100000 0
```

For Mac:
```
./a.out 64 90 9 1 5 100000 0
```
