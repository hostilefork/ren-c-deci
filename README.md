## "Deci" Extension

This implements an R3-Alpha concept of a "deci" number, which was written
by Ladislav Mecir.  According to the author:

   "The (deci) datatype is neither a bignum, nor a fixpoint arithmetic.
    It actually is unnormalized decimal floating point."


### History: R3-Alpha introduced a MONEY! datatype

The documentation claimed it used an IEEE floating point type:

  https://www.rebol.com/r3/docs/datatypes/money.html

But the description at that link appears outdated.  For instance, R3-Alpha's
MONEY! did not support the 15-bit ISO 4217 currency designator:

  https://en.wikipedia.org/wiki/ISO_4217

And for the underlying math, it used something customized and not part of
any standard, called "deci":

  https://github.com/rebol/rebol/blob/25033f897b2bd466068d7663563cd3ff64740b94/src/core/f-deci.c

According to the comments:

> Deci significands are 87-bit long, unsigned, unnormalized, stored in
> little endian order. (Maximal deci significand is 1e26 - 1, i.e. 26 nines)
>
> Sign is one-bit, 1 means nonpositive, 0 means nonnegative.
>
> Exponent is 8-bit, unbiased.

Some sample behavior:

    r3-alpha>> $1
    == $1

    r3-alpha>> $1.0
    == $1.0

    r3-alpha>> $1.00
    == $1.00

    r3-alpha>> $0.03 / 2
    == $0.015000000000000000000000000


### With Ladislav not contributing, deci lacked a maintainer

Few relevant codebases were performing MONEY!-based math.  Those that did
would probably be fine using plain old floating point calculations.  In fact,
the Red project just made their MONEY! type a floating point number that
only displays two of its digits after the decimal point:

    red>> $1
    == $1.00  ; always displays decimal points, regardless of input

    red>> half: $0.03 / 2
    == $0.01  ; remembers more digits, but doesn't display them

    red>> half = $0.01
    == false  ; ugh

    red>> half * 2
    == $0.03

Carrying around the deci implementation as part of Ren-C's core was just
adding size to the executable.  It is a significant amount of code that isn't
easy for the uninitiated (or the initiated?) to read.


### Ren-C Changed MONEY! to an ANY-UTF8? Type

The idea is that a MONEY! is an immutable string type.  It's a $ followed by
digits, with an optional decimal point plus exactly two additional digits
coming after it.

    >> $10
    == $10

    >> $10.20
    == $10.20

    >> $10.2
    ** Syntax Error: invalid "money" -- "$10.2"

    >> $0.304
    ** Syntax Error: invalid "money" -- "$0.304"

The dollar sign is not part of the string:

    >> to tag! $120,384,787,857498348754304930485094830948509438598435.50
    == <120384787857498348754304930485094830948509438598435.50>

It's possible to convert MONEY! values into DECIMAL! or INTEGER! via the
same code that operates on strings, do math on them, and then convert them
back into MONEY!:

    >> sum: (make decimal! $1.50) + (make integer! $5)
    == 6.5

    >> make money! sum
    == $6.50

    >> make money! round sum
    == $6

In practice, the truly useful form for dialects is integers: $1 $2 $3 $4 $5.
These are succinct and can indicate points of substitution or other things.
It's crucial that they not get random ".00" tacked onto them without an
operation explicitly requesting that.


### Extension Sacrifice: Deci Extension only works on 64-bit

The original deci design was such that it could be compacted and stored
inside of an R3-Alpha Cell's payload:

    typedef struct {
        unsigned m0:32;  /* significand, lowest part */
        unsigned m1:32;  /* significand, continuation */
        unsigned m2:23;  /* significand, highest part */
        unsigned s:1;    /* sign, 0 means nonnegative, 1 means nonpositive */
        int e:8;         /* exponent */
    } deci;

This worked when the Cell payload was the size of three platform pointers,
as it's a total of 96 bits...which can fit in that size on 32-bit platforms.

However, with the goal of removing the deci implementation from the core,
then the DECI! datatype becomes an "extension type".  Extension types in Ren-C
sacrifice one platform pointer to identify their datatype (since they are
beyond the byte used for the built-in types and typesets).  So on 32-bit
platforms, they only have 64 bits available for the payload of these types.

This leaves three options:

1. Cut down on the number of significant digits in the deci code to make it
   represent only smaller amounts, and fit in 64 bits.

2. Allocate a Stub-sized GC node to add a level of indirection, and put the
   deci information in that (which would technically allow for much
   bigger numbers if that were interesting).

3. Restrict the DECI! extension type to 64-bit builds, where the two platform
   pointers worth of size offer 128 bits of space, enough for the 96-bit
   historical representation of deci.

Option 1 would require editing the deci code, and it's an explicit goal (of
@hostilefork's) to not get involved with that.

Option 2 isn't difficult (assuming not trying to edit deci to use any extra
space beyond the 96 bits).  But it is a bit of extra work, and doesn't
demonstrate how to write code that just uses the space in the Cell, which
was the original spirit of the type.

So Option 3 is what is chosen.  If DECI! becomes important on 32-bit platforms
then it's fairly trivial to implement Option 2.
