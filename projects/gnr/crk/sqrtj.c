/* Basic algorythm from Wikipedia
https://en.wikipedia.org/wiki/Integer_square_root
[But I have to fix it for variable number of fraction bits]

*/

function integerSqrt(n):
    if n < 0:
        error "integerSqrt works for only nonnegative inputs"
    
    # Find greatest shift.
    shift = 2
    nShifted = n >> shift
    # We check for nShifted being n, since some implementations of logical right shifting shift modulo the word size.
    while nShifted \u2260 0 and nShifted \u2260 n:
        shift = shift + 2
        nShifted = n >> shift
    shift = shift - 2
    
    # Find digits of result.
    result = 0
    while shift \u2265 0:
        result = result << 1
        candidateResult = result + 1
        if candidateResult*candidateResult \u2264 n >> shift:
            result = candidateResult
        shift = shift - 2
   
    return result
