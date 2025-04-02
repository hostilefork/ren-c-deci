//
//  File: %mod-deci.c
//  Summary: "Extended Precision Datatype Excised From R3-Alpha"
//  Section: datatypes
//  Project: "Rebol 3 Interpreter and Run-time (Ren-C branch)"
//  Homepage: https://github.com/metaeducation/ren-c/
//
//=/////////////////////////////////////////////////////////////////////////=//
//
// Copyright 2012 REBOL Technologies
// Copyright 2012-2025 Ren-C Open Source Contributors
// REBOL is a trademark of REBOL Technologies
//
// See README.md and CREDITS.md for more information.
//
// Licensed under the Lesser GPL, Version 3.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://www.gnu.org/licenses/lgpl-3.0.html
//
//=/////////////////////////////////////////////////////////////////////////=//
//
// "Deci" was a not-quite-fixed point number type that was used in R3-Alpha.
//
// It was the implementation behind the MONEY! type, but Ren-C has taken a
// different approach--making MONEY! an immutable string type.
//
// This module is the attempt to keep the deci type around as an extension,
// for those who want to use it.  It is exposed as the DECI! datatype.
//
// See remarks in README.md for more information.
//

#include "tmp-mod-deci.h"

#include "deci.h"


INLINE Element* Init_Deci(Init(Element) out, deci amount) {
    Reset_Extended_Cell_Header_Noquote(
        out,
        EXTENDED_HEART(Is_Deci),
        (CELL_FLAG_DONT_MARK_NODE1)  // whole payload is just deci data
            | CELL_FLAG_DONT_MARK_NODE2  // none of it should be marked
    );

    STATIC_ASSERT(sizeof(out->payload.at_least_8) >= sizeof(deci));
    Mem_Copy(out->payload.at_least_8, &amount, sizeof(deci));

    return out;
}


INLINE deci Cell_Deci_Amount(const Cell* v) {
    assert(Is_Deci(v));

    STATIC_ASSERT(sizeof(v->payload.at_least_8) >= sizeof(deci));
    deci amount;
    Mem_Copy(&amount, v->payload.at_least_8, sizeof(deci));
    return amount;
}


IMPLEMENT_GENERIC(EQUAL_Q, Is_Deci)
{
    INCLUDE_PARAMS_OF_EQUAL_Q;

    deci a = Cell_Deci_Amount(ARG(VALUE1));
    deci b = Cell_Deci_Amount(ARG(VALUE2));
    UNUSED(ARG(STRICT));

    return LOGIC(deci_is_equal(a, b));
}


IMPLEMENT_GENERIC(LESSER_Q, Is_Deci)
{
    INCLUDE_PARAMS_OF_LESSER_Q;

    deci a = Cell_Deci_Amount(ARG(VALUE1));
    deci b = Cell_Deci_Amount(ARG(VALUE2));

    if (deci_is_equal(a, b))
        return LOGIC(false);

    return LOGIC(deci_is_lesser_or_equal(a, b));
}


IMPLEMENT_GENERIC(ZEROIFY, Is_Deci)
{
    INCLUDE_PARAMS_OF_ZEROIFY;
    UNUSED(ARG(EXAMPLE));  // always gives $0

    return Init_Deci(OUT, int_to_deci(0));
}


// !!! The deci API is being left mostly untouched, so it doesn't return
// Option(Error*), it just abruptly fail()s.  Try to contain the issue by
// using RESCUE_SCOPE().
//
static Option(Error*) Trap_Blob_To_Deci(Sink(Value) out, const Element* blob)
{
    assert(Is_Blob(blob));

    Size size;
    const Byte* at = Cell_Blob_Size_At(&size, blob);
    if (size > 12)
        size = 12;

    Byte buf[MAX_HEX_LEN + 4] = {0};  // binary to convert
    memcpy(buf, at, size);
    memcpy(buf + 12 - size, buf, size);  // shift to right side
    memset(buf, 0, 12 - size);

    RESCUE_SCOPE_IN_CASE_OF_ABRUPT_FAILURE {
        Init_Deci(out, binary_to_deci(buf));
        CLEANUP_BEFORE_EXITING_RESCUE_SCOPE;
        return nullptr;  // no failure
    }
    ON_ABRUPT_FAILURE (Error* e) {
        CLEANUP_BEFORE_EXITING_RESCUE_SCOPE;
        return e;
    }
}


//
//  export make-deci: native [
//
//  "Make a DECI! workaround (MAKE DECI! is still in progress"
//
//      return: [element?]  ; DECI! not working yet
//      spec [integer! decimal! percent! money! text! blob!]
//  ]
//
DECLARE_NATIVE(MAKE_DECI)
{
    INCLUDE_PARAMS_OF_MAKE_DECI;

    Element* arg = Element_ARG(SPEC);

    switch (Type_Of(arg)) {
      case TYPE_INTEGER:
        return Init_Deci(OUT, int_to_deci(VAL_INT64(arg)));

      case TYPE_DECIMAL:
      case TYPE_PERCENT:
        return Init_Deci(OUT, decimal_to_deci(VAL_DECIMAL(arg)));

      case TYPE_MONEY:
        return Copy_Cell(OUT, arg);

      case TYPE_TEXT: {
        Option(Error*) error = Trap_Transcode_One(OUT, TYPE_0, arg);
        if (error)
            return RAISE(unwrap error);
        if (Is_Deci(OUT))
            return OUT;
        if (Is_Decimal(OUT) or Is_Integer(OUT))
            return Init_Deci(OUT, decimal_to_deci(Dec64(stable_OUT)));
        break; }

      case TYPE_BLOB: {
        Option(Error*) e = Trap_Blob_To_Deci(OUT, arg);
        if (e)
            return FAIL(unwrap e);
        return OUT; }

      default:
        break;
    }

    return FAIL(PARAM(SPEC));
}


IMPLEMENT_GENERIC(MOLDIFY, Is_Deci)
{
    INCLUDE_PARAMS_OF_MOLDIFY;

    Element* v = Element_ARG(ELEMENT);
    Molder* mo = Cell_Handle_Pointer(Molder, ARG(MOLDER));
    bool form = Bool_ARG(FORM);

    UNUSED(form);

    Append_Ascii(mo->string, "#[deci!");  // deci_to_string adds space

    Byte buf[60];
    REBINT len = deci_to_string(buf, Cell_Deci_Amount(v), ' ', '.');
    Append_Ascii_Len(mo->string, s_cast(buf), len);

    Append_Ascii(mo->string, "]");

    return NOTHING;
}


static Value* Math_Arg_For_Money(
    Sink(Value) store,
    Value* arg,
    const Symbol* verb
){
    if (Is_Deci(arg))
        return arg;

    if (Is_Integer(arg)) {
        Init_Deci(store, int_to_deci(VAL_INT64(arg)));
        return store;
    }

    if (Is_Decimal(arg) or Is_Percent(arg)) {
        Init_Deci(store, decimal_to_deci(VAL_DECIMAL(arg)));
        return store;
    }

    fail (Error_Math_Args(TYPE_MONEY, verb));
}


IMPLEMENT_GENERIC(OLDGENERIC, Is_Deci)
{
    const Symbol* verb = Level_Verb(LEVEL);
    Option(SymId) id = Symbol_Id(verb);

    Element* v = cast(Element*, ARG_N(1));

    switch (id) {
      case SYM_ADD: {
        Value* arg = Math_Arg_For_Money(SPARE, ARG_N(2), verb);
        return Init_Deci(
            OUT,
            deci_add(Cell_Deci_Amount(v), Cell_Deci_Amount(arg))
        ); }

      case SYM_SUBTRACT: {
        Value* arg = Math_Arg_For_Money(SPARE, ARG_N(2), verb);
        return Init_Deci(
            OUT,
            deci_subtract(Cell_Deci_Amount(v), Cell_Deci_Amount(arg))
        ); }

      case SYM_DIVIDE: {
        Value* arg = Math_Arg_For_Money(SPARE, ARG_N(2), verb);
        return Init_Deci(
            OUT,
            deci_divide(Cell_Deci_Amount(v), Cell_Deci_Amount(arg))
        ); }

      case SYM_REMAINDER: {
        Value* arg = Math_Arg_For_Money(SPARE, ARG_N(2), verb);
        return Init_Deci(
            OUT,
            deci_mod(Cell_Deci_Amount(v), Cell_Deci_Amount(arg))
        ); }

      case SYM_NEGATE: // sign bit is the 32nd bit, highest one used
        v->payload.split.two.u ^= (cast(uintptr_t, 1) << 31);
        return COPY(v);

      case SYM_ABSOLUTE:
        v->payload.split.two.u &= ~(cast(uintptr_t, 1) << 31);
        return COPY(v);

      case SYM_EVEN_Q:
      case SYM_ODD_Q: {
        REBINT result = 1 & cast(REBINT, deci_to_int(Cell_Deci_Amount(v)));
        if (Symbol_Id(verb) == SYM_EVEN_Q)
            result = not result;
        return Init_Logic(OUT, result != 0); }

      default:
        break;
    }

    return UNHANDLED;
}


IMPLEMENT_GENERIC(TO, Is_Deci)
{
    INCLUDE_PARAMS_OF_TO;

    Element* v = Element_ARG(ELEMENT);
    Heart to = Cell_Datatype_Builtin_Heart(ARG(TYPE));

    deci d = Cell_Deci_Amount(v);

    if (to == TYPE_DECIMAL or to == TYPE_PERCENT)
        return Init_Decimal_Or_Percent(OUT, to, deci_to_decimal(d));

    if (to == TYPE_INTEGER) {  // !!! how to check for digits after dot?
        REBI64 i = deci_to_int(d);
        deci reverse = int_to_deci(i);
        if (not deci_is_equal(d, reverse))
            return RAISE(
                "Can't TO INTEGER! a MONEY! w/digits after decimal point"
            );
        return Init_Integer(OUT, i);
    }

    if (Any_Utf8_Type(to)) {
        if (d.e != 0 or d.m1 != 0 or d.m2 != 0)
            Init_Decimal(v, deci_to_decimal(d));
        else
            Init_Integer(v, deci_to_int(d));

        DECLARE_MOLDER (mo);
        SET_MOLD_FLAG(mo, MOLD_FLAG_SPREAD);
        Push_Mold(mo);
        Mold_Element(mo, v);
        const String* s = Pop_Molded_String(mo);
        if (not Any_String_Type(to))
            Freeze_Flex(s);
        return Init_Any_String(OUT, to, s);;
    }

    if (to == TYPE_MONEY)
        return COPY(v);

    return UNHANDLED;
}


IMPLEMENT_GENERIC(MULTIPLY, Is_Deci)
{
    INCLUDE_PARAMS_OF_MULTIPLY;

    deci d1 = Cell_Deci_Amount(ARG(VALUE1));  // first generic arg is money

    Value* money2 = Math_Arg_For_Money(SPARE, ARG(VALUE2), CANON(MULTIPLY));
    deci d2 = Cell_Deci_Amount(money2);

    return Init_Deci(OUT, deci_multiply(d1, d2));
}


IMPLEMENT_GENERIC(ROUND, Is_Deci)
{
    INCLUDE_PARAMS_OF_ROUND;

    Element* v = Element_ARG(VALUE);

    Value* to = ARG(TO);
    if (Is_Nulled(to))
        Init_Deci(to, decimal_to_deci(1.0L));

    deci scale = decimal_to_deci(Bool_ARG(TO) ? Dec64(ARG(TO)) : 1.0);

    if (deci_is_zero(scale))
        return RAISE(Error_Zero_Divide_Raw());

    scale = deci_abs(scale);

    deci d = Cell_Deci_Amount(v);;
    if (Bool_ARG(EVEN))
        d = deci_half_even(d, scale);
    else if (Bool_ARG(DOWN))
        d = deci_truncate(d, scale);
    else if (Bool_ARG(HALF_DOWN))
        d = deci_half_truncate(d, scale);
    else if (Bool_ARG(FLOOR))
        d = deci_floor(d, scale);
    else if (Bool_ARG(CEILING))
        d = deci_ceil(d, scale);
    else if (Bool_ARG(HALF_CEILING))
        d = deci_half_ceil(d, scale);
    else
        d = deci_half_away(d, scale);

    if (Is_Decimal(to) or Is_Percent(to)) {
        REBDEC dec = deci_to_decimal(d);
        Heart to_heart = Heart_Of_Builtin_Fundamental(to);
        Reset_Cell_Header_Noquote(
            TRACK(OUT),
            FLAG_HEART_BYTE(to_heart) | CELL_MASK_NO_NODES
        );
        VAL_DECIMAL(OUT) = dec;
        return OUT;
    }

    if (Is_Integer(to)) {
        REBI64 i64 = deci_to_int(d);
        return Init_Integer(OUT, i64);
    }

    return Init_Deci(OUT, d);
}


//
//  startup*: native [
//
//  "Register behaviors for DECI!"
//
//      return: [~]
//  ]
//
DECLARE_NATIVE(STARTUP_P)
{
    INCLUDE_PARAMS_OF_STARTUP_P;

    EXTENDED_HEART(Is_Deci) = Register_Datatype("deci!");

    Register_Generics(EXTENDED_GENERICS());

    return NOTHING;
}


//
//  shutdown*: native [
//
//  "Unregister behaviors for DECI!"
//
//      return: [~]
//  ]
//
DECLARE_NATIVE(SHUTDOWN_P)
{
    INCLUDE_PARAMS_OF_SHUTDOWN_P;

    Unregister_Generics(EXTENDED_GENERICS());

    Unregister_Datatype(EXTENDED_HEART(Is_Deci));

    return NOTHING;
}
