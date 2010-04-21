/*
  Copyright 2009,2010 Karl Robillard

  This file is part of the Urlan datatype system.

  Urlan is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Urlan is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Urlan.  If not, see <http://www.gnu.org/licenses/>.
*/


/** \struct UDatatype
  The UDatatype struct holds methods for a specific class of data.

  When implementing custom types, the unset.h methods can be used.

  \sa USeriesType
*/
/** \var UDatatype::name
  ASCII name of type, ending with '!'.
*/
/** \fn int (*UDatatype::make)(UThread*, const UCell* from, UCell* res)
  Make a new instance of the type.
  From and res are guaranteed to point to different cells.

  Make should perform a shallow copy on any complex types which can be
  nested.

  \param from   Value which describes the new instance.
  \param res    Cell for new instance.

  \return UR_OK/UR_THROW
*/
/** \fn void (*UDatatype::convert)(UThread*, const UCell* from, UCell* res)
  Convert data to a new instance of the type.
  From and res are guaranteed to point to different cells.

  Convert should perform a shallow copy on any complex types which can be
  nested.

  \param from   Value to convert.
  \param res    Cell for new instance.

  \return UR_OK/UR_THROW
*/
/** \fn void (*UDatatype::copy)(UThread*, const UCell* from, UCell* res)
  Make a clone of an existing value.
  From and res are guaranteed to point to different cells.

  \param from   Value to copy.
  \param res    Cell for new instance.
*/
/** \fn int (*UDatatype::compare)(UThread*, const UCell* a, const UCell* b, int test)
  Perform a comparison between cells.

  If the test is UR_COMPARE_SAME, then the type of cells a & b are
  guaranteed to be the same by the caller.  Otherwise, one of the cells
  might not be of the datatype for this compare method.

  When the test is UR_COMPARE_ORDER or UR_COMPARE_ORDER_CASE, then the method
  should return 1, 0, or -1, if cell a is greater than, equal to, or lesser
  than cell b.

  The method should only check for datatypes with a lesser or equal
  ur_type(), as the caller will use the compare method for the higher
  ordered type.

  \param a      First cell.
  \param b      Second cell.
  \param test   Type of comparison to do.

  \return Non-zero if the camparison is true.
*/
/** \fn int  (*UDatatype::select)( UThread*, const UCell* cell, UBlockIter* bi, UCell* res)
  Get value which path references.
  If the path is valid, this method must set the result, advance bi->it, and
  return UR_OK.
  If the path is invalid, call ur_error() and return UR_THROW.

  \param cell   Cell of this datatype.
  \param bi     Path block with bi->it set to the node following cell.
  \param res    Result of path selection.

  \return UR_OK/UR_THROW
*/
/** \fn void (*UDatatype::toString)( UThread*, const UCell* cell, UBuffer* str, int depth )
  Convert cell to its string data representation.

  \param cell   Cell of this datatype.
  \param str    String buffer to append to.
  \param depth  Indentation depth.
*/
/** \fn void (*UDatatype::toText)( UThread*, const UCell* cell, UBuffer* str, int depth )
  Convert cell to its string textual representation.

  \param cell   Cell of this datatype.
  \param str    String buffer to append to.
  \param depth  Indentation depth.
*/
/** \fn const char (*UDatatype::recycle)(UThread*, int phase)
  Performs thread global garbage collection duties for the datatype.
  This is called twice from ur_recycle(), first with UR_RECYCLE_MARK, then
  with UR_RECYCLE_SWEEP.
*/
/** \var UrlanRecyclePhase::UR_RECYCLE_MARK
  Phase passed to UDatatype::recycle.
*/
/** \var UrlanRecyclePhase::UR_RECYCLE_SWEEP
  Phase passed to UDatatype::recycle.
*/
/** \fn void (*UDatatype::mark)(UThread*, UCell* cell)
  Responsible for marking any buffers referenced by the cell as used.
  Use ur_markBuffer().
*/
/** \fn void (*UDatatype::destroy)(UBuffer* buf)
  Free any memory or other resources the buffer uses.
*/
/** \fn void (*UDatatype::markBuf)(UThread*, UBuffer* buf)
  Responsible for marking other buffers referenced by this buffer as used.
*/
/** \fn void (*UDatatype::toShared)( UCell* cell )
  Change any buffer ids in the cell so that they reference the shared
  environment (negate the id).
*/
/** \fn void (*UDatatype::bind)( UThread*, UCell* cell, const UBindTarget* bt )
  Bind cell to target.
*/

/** \struct USeriesType
  The USeriesType struct holds extra methods for series datatypes.
*/
/** \fn void (*USeriesType::pick)( const UBuffer* buf, UIndex n, UCell* res )
  Get a single value from the series.

  \param buf    Series buffer to pick from.
  \param n      Zero-based index of element to pick.
  \param res    Result of picked value.
*/
/** \fn void (*USeriesType::poke)( UBuffer* buf, UIndex n, const UCell* val )
  Replace a single value in the series.

  \param buf    Series buffer to modify.
  \param n      Zero-based index of element to replace.
  \param val    Value to replace.
*/
/** \fn int  (*USeriesType::append)( UThread*, UBuffer* buf, const UCell* val )
  Append a value to the series.

  \param buf    Series buffer to append to.
  \param val    Value to append.

  \return UR_OK/UR_THROW
*/
/** \fn void (*USeriesType::remove)( UThread*, UBuffer* buf,
                                     UIndex it, UIndex end )
  Remove part of the series.

  \param buf    Series buffer.
  \param it     Start of part to remove.
  \param end    End of part to remove.
*/
/** \fn int (*USeriesType::find)( UThread*, const UCell* ser, const UCell* val,
                                  int opt )
  Search for a value in the series.

  \param ser    Series cell.
  \param val    Value to find.
  \param opt    Options.

  \return Zero-based index of val in series or -1 if not found.
*/


#include "urlan_atoms.h"
#include "bignum.h"


//#define ANY3(c,t1,t2,t3)    ((1<<ur_type(c)) & ((1<<t1) | (1<<t2) | (1<<t3)))

#define DT(dt)          (ut->types[ dt ])
#define SERIES_DT(dt)   ((const USeriesType*) (ut->types[ dt ]))

#define context_markBuf block_markBuf

void block_markBuf( UThread* ut, UBuffer* buf );


//----------------------------------------------------------------------------
// UT_UNSET


int unset_make( UThread* ut, const UCell* from, UCell* res )
{
    (void) ut;
    (void) from;
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

void unset_copy( UThread* ut, const UCell* from, UCell* res )
{
    (void) ut;
    *res = *from;
}

int unset_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    (void) ut;
    (void) a;
    (void) b;
    (void) test;
    return 0;
}

int unset_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    (void) cell;
    (void) bi;
    (void) res;
    return ur_error( ut, UR_ERR_SCRIPT, "path select is unset for type %s",
                     ur_atomCStr(ut, ur_type(cell)) );
}

void unset_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendChar( str, '~' );
    ur_strAppendCStr( str, ur_atomCStr(ut, ur_type(cell)) );
    ur_strAppendChar( str, '~' );
}

void unset_mark( UThread* ut, UCell* cell )
{
    (void) ut;
    (void) cell;
}

void unset_destroy( UBuffer* buf )
{
    (void) buf;
}

void unset_toShared( UCell* cell )
{
    (void) cell;
}

void unset_bind( UThread* ut, UCell* cell, const UBindTarget* bt )
{
    (void) ut;
    (void) cell;
    (void) bt;
}


#define unset_recycle   0
#define unset_markBuf   0


UDatatype dt_unset =
{
    "unset!",
    unset_make,             unset_make,             unset_copy,
    unset_compare,          unset_select,
    unset_toString,         unset_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_DATATYPE


/**
  Test if cell is of a certain datatype.

  \param cell       Cell to test.
  \param datatype   Valid cell of type UT_DATATYPE.

  \return Non-zero if cell type matches datatype.
*/
int ur_isDatatype( const UCell* cell, const UCell* datatype )
{
    uint32_t dt = ur_type(cell);
    if( dt < 32 )
        return datatype->datatype.mask0 & (1 << dt);
    else
        return datatype->datatype.mask1 & (1 << (dt - 32));
}


/**
  Initialize cell to be a UT_DATATYPE value of the given type.
*/
void ur_makeDatatype( UCell* cell, int type )
{
    ur_setId(cell, UT_DATATYPE);
    ur_datatype(cell) = type;
    if( type < 32 )
    {
        cell->datatype.mask0 = 1 << type;
        cell->datatype.mask1 = cell->datatype.mask2 = 0;
        cell->datatype.bitCount = 1;
    }
    else if( type < 64 )
    {
        cell->datatype.mask1 = 1 << (type - 32);
        cell->datatype.mask0 = cell->datatype.mask2 = 0;
        cell->datatype.bitCount = 1;
    }
    else
    {
        cell->datatype.mask1 = 
        cell->datatype.mask0 = cell->datatype.mask2 = 0xffffffff;
        cell->datatype.bitCount = UT_MAX;
    }
}


/**
  Add type to multi-type UT_DATATYPE cell.
*/
void ur_datatypeAddType( UCell* cell, int type )
{
    uint32_t* mp;
    uint32_t mask;

    if( type < 32 )
    {
        mp = &cell->datatype.mask0;
        mask = 1 << type;
    }
    else if( type < 64 )
    {
        mp = &cell->datatype.mask1;
        mask = 1 << (type - 32);
    }
    else
    {
        mp = &cell->datatype.mask2;
        mask = 1 << (type - 64);
    }

    if( ! (mask & *mp) )
    {
        *mp |= mask;
        cell->datatype.n = UT_TYPEMASK;
        ++cell->datatype.bitCount;
    }
}


#if 0
/*
  If cell is any word and it has a datatype name then that type is returned.
  Otherwise the datatype of the cell is returned.
*/
static int _wordType( UThread* ut, const UCell* cell )
{
    int type = ur_type(cell);
    if( ur_isWordType(type) && (ur_atom(cell) < ur_datatypeCount(ut)) )
        type = ur_atom(cell);
    return type;
}
#endif


int datatype_make( UThread* ut, const UCell* from, UCell* res )
{
    (void) ut;
    ur_makeDatatype( res, ur_type(from) );
    return UR_OK;
}


int datatype_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    (void) ut;
    switch( test )
    {
        case UR_COMPARE_SAME:
            if( ur_datatype(a) == ur_datatype(b) )
            {
                if( ur_datatype(a) != UT_TYPEMASK )
                    return 1;
                return ((a->datatype.mask0 == b->datatype.mask0) &&
                        (a->datatype.mask1 == b->datatype.mask1));
            }
            break;

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            if( ur_type(a) == ur_type(b) )
            {
                return ((a->datatype.mask0 & b->datatype.mask0) ||
                        (a->datatype.mask1 & b->datatype.mask1));
            }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            if( ur_type(a) == ur_type(b) )
            {
                if( ur_datatype(a) > ur_datatype(b) )
                    return 1;
                if( ur_datatype(a) < ur_datatype(b) )
                    return -1;
                // Order of two multi-types is undefined.
            }
            break;
    }
    return 0;
}


void datatype_toString(UThread* ut, const UCell* cell, UBuffer* str, int depth)
{
    int dt = ur_datatype(cell);
    (void) depth;
    if( dt < UT_MAX )
    {
        ur_strAppendCStr( str, ur_atomCStr( ut, dt ) );
    }
    else
    {
        int i;
        int count = cell->datatype.bitCount;
        int mask = 1;
        int cellMask = cell->datatype.mask0;
        int maxt = ut->env->typeCount;
        for( i = 0; i < maxt; ++i )
        {
            if( mask & cellMask )
            {
                ur_strAppendCStr( str, ur_atomCStr( ut, i ) );
                if( --count )
                    ur_strAppendChar( str, '/' );
                else
                    break;
            }
            if( i == 31 )
            {
                mask = 1;
                cellMask = cell->datatype.mask1;
            }
            else
                mask <<= 1;
        }
    }
}


UDatatype dt_datatype =
{
    "datatype!",
    datatype_make,          datatype_make,          unset_copy,
    datatype_compare,       unset_select,
    datatype_toString,      datatype_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_NONE


int none_make( UThread* ut, const UCell* from, UCell* res )
{
    (void) ut;
    (void) from;
    ur_setId(res, UT_NONE);
    return UR_OK;
}


int none_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    (void) ut;
    switch( test )
    {
        case UR_COMPARE_SAME:
        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            return ur_type(a) == ur_type(b);
    }
    return 0;
}


void none_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    (void) cell;
    (void) depth;
    ur_strAppendCStr( str, "none" );
}


UDatatype dt_none =
{
    "none!",
    none_make,              none_make,              unset_copy,
    none_compare,           unset_select,
    none_toString,          none_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_LOGIC


int logic_make( UThread* ut, const UCell* from, UCell* res )
{
    (void) ut;
    ur_setId(res, UT_LOGIC);
    switch( ur_type(from) )
    {
        case UT_NONE:
            ur_int(res) = 0;
            break;
        case UT_LOGIC:
        case UT_CHAR:
        case UT_INT:
            ur_int(res) = ur_int(from) ? 1 : 0;
            break;
        case UT_DECIMAL:
            ur_int(res) = ur_decimal(from) ? 1 : 0;
            break;
        case UT_BIGNUM:
        {
            UCell tmp;
            bignum_zero( &tmp );
            ur_int(res) = bignum_equal(from, &tmp) ? 0 : 1;
        }
            break;
        default:
            ur_int(res) = 1;
            break;
    }
    return UR_OK;
}


void logic_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    (void) depth;
    ur_strAppendCStr( str, ur_int(cell) ? "true" : "false" );
}


UDatatype dt_logic =
{
    "logic!",
    logic_make,             logic_make,             unset_copy,
    unset_compare,          unset_select,
    logic_toString,         logic_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind,
};


//----------------------------------------------------------------------------
// UT_CHAR


int char_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_INT) || ur_is(from, UT_CHAR) )
    {
        ur_setId(res, UT_CHAR);
        ur_int(res) = ur_int(from);
        return UR_OK;
    }
    else if( ur_is(from, UT_STRING) )
    {
        USeriesIter si;
        ur_seriesSlice( ut, &si, from );
        SERIES_DT( UT_STRING )->pick( si.buf, si.it, res );
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE,
                     "make char! expected char!/int!/string!" );
}


void char_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    char tmp[5];
    char* cstr;
    int n = ur_int(cell);
    (void) ut;
    (void) depth;

    if( n > 127 )
    {
        ur_strAppendChar( str, '\'' );
        ur_strAppendChar( str, n );
        ur_strAppendChar( str, '\'' );
        return;
    }

    if( n < 16 )
    {
        if( n == '\n' )
            cstr = "'^/'";
        else if( n == '\t' )
            cstr = "'^-'";
        else
        {
            cstr = tmp;
            *cstr++ = '\'';
            *cstr++ = '^';
            n += ((n < 11) ? '0' : ('A' - 10));
            goto close_esc;
        }
    }
    else
    {
        cstr = tmp;
        *cstr++ = '\'';
        if( n == '^' || n == '\'' )
            *cstr++ = '^';
close_esc:
        *cstr++ = n;
        *cstr++ = '\'';
        *cstr = '\0';
        cstr = tmp;
    }
    ur_strAppendCStr( str, cstr );
}


void char_toText( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    (void) depth;
    ur_strAppendChar( str, ur_int(cell) );
}


int int_compare( UThread* ut, const UCell* a, const UCell* b, int test );

UDatatype dt_char =
{
    "char!",
    char_make,              char_make,              unset_copy,
    int_compare,            unset_select,
    char_toString,          char_toText,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_INT


extern int64_t str_toInt64( const char*, const char*, const char** pos );

int int_make( UThread* ut, const UCell* from, UCell* res )
{
    ur_setId(res, UT_INT);
    switch( ur_type(from) )
    {
        case UT_NONE:
            ur_int(res) = 0;
            break;
        case UT_LOGIC:
        case UT_CHAR:
        case UT_INT:
            ur_int(res) = ur_int(from);
            break;
        case UT_DECIMAL:
        case UT_TIME:
        case UT_DATE:
            ur_int(res) = ur_decimal(from);
            break;
        case UT_BIGNUM:
            ur_int(res) = (int32_t) bignum_l(from);
            break;
        case UT_STRING:
        {
            USeriesIter si;
            ur_seriesSlice( ut, &si, from );
            if( ur_strIsUcs2(si.buf) )
                ur_int(res) = 0;
            else
                ur_int(res) = (int32_t) str_toInt64(si.buf->ptr.c + si.it,
                                                    si.buf->ptr.c + si.end, 0);
        }
            break;
        default:
            return ur_error( ut, UR_ERR_TYPE,
                "make int! expected number or none!/logic!/char!/string!" );
    }
    return UR_OK;
}


#define MASK_CHAR_INT   ((1 << UT_CHAR) | (1 << UT_INT))
#define ur_isIntType(T) ((1 << T) & MASK_CHAR_INT)

int int_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    (void) ut;

    if( test == UR_COMPARE_SAME )
        return ur_int(a) == ur_int(b);

    if( ur_isIntType( ur_type(a) ) && ur_isIntType( ur_type(b) ) )
    {
        switch( test )
        {
            case UR_COMPARE_EQUAL:
            case UR_COMPARE_EQUAL_CASE:
                return ur_int(a) == ur_int(b);

            case UR_COMPARE_ORDER:
            case UR_COMPARE_ORDER_CASE:
                if( ur_int(a) > ur_int(b) )
                    return 1;
                if( ur_int(a) < ur_int(b) )
                    return -1;
                break;
        }
    }
    return 0;
}


void int_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    (void) depth;
    if( ur_flags(cell, UR_FLAG_INT_HEX) )
    {
        ur_strAppendCStr( str, "0x" );
        ur_strAppendHex( str, ur_int(cell), 0 );
    }
    else
        ur_strAppendInt( str, ur_int(cell) );
}


UDatatype dt_int =
{
    "int!",
    int_make,               int_make,               unset_copy,
    int_compare,            unset_select,
    int_toString,           int_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_DECIMAL


extern double str_toDouble( const char*, const char*, const char** pos );

int decimal_make( UThread* ut, const UCell* from, UCell* res )
{
    ur_setId(res, UT_DECIMAL);
    switch( ur_type(from) )
    {
        case UT_NONE:
            ur_decimal(res) = 0.0;
            break;
        case UT_LOGIC:
        case UT_CHAR:
        case UT_INT:
            ur_decimal(res) = (double) ur_int(from);
            break;
        case UT_DECIMAL:
        case UT_TIME:
        case UT_DATE:
            ur_decimal(res) = ur_decimal(from);
            break;
        case UT_BIGNUM:
            ur_decimal(res) = bignum_d(from);
            break;
        case UT_STRING:
        {
            USeriesIter si;
            ur_seriesSlice( ut, &si, from );
            if( ur_strIsUcs2(si.buf) )
                ur_decimal(res) = 0;
            else
                ur_decimal(res) = str_toDouble( si.buf->ptr.c + si.it,
                                                si.buf->ptr.c + si.end, 0 );
        }
            break;
        default:
            return ur_error( ut, UR_ERR_TYPE,
                "make decimal! expected number or none!/logic!/char!/string!" );
    }
    return UR_OK;
}


#define MASK_DECIMAL    ((1 << UT_DECIMAL) | (1 << UT_TIME) | (1 << UT_DATE))
#define ur_isDecimalType(T) ((1 << T) & MASK_DECIMAL)


int decimal_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    (void) ut;

    switch( test )
    {
        case UR_COMPARE_SAME:
            return ur_decimal(a) == ur_decimal(b);

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            if( ur_isDecimalType( ur_type(a) ) )
            {
                if( ur_isDecimalType( ur_type(b) ) )
                    return ur_decimal(a) == ur_decimal(b);
                else if( ur_isIntType( ur_type(b) ) )
                    return ur_decimal(a) == ur_int(b);
            }
            else
            {
                if( ur_isIntType( ur_type(a) ) )
                    return ((double) ur_int(a)) == ur_decimal(b);
            }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            if( ur_isDecimalType( ur_type(a) ) )
            {
                if( ur_isDecimalType( ur_type(b) ) )
                {
                    if( ur_decimal(a) > ur_decimal(b) )
                        return 1;
                    if( ur_decimal(a) < ur_decimal(b) )
                        return -1;
                }
                else if( ur_isIntType( ur_type(b) ) )
                {
                    if( ur_decimal(a) > ur_int(b) )
                        return 1;
                    if( ur_decimal(a) < ur_int(b) )
                        return -1;
                }
            }
            else
            {
                if( ur_isIntType( ur_type(a) ) )
                {
                    if( ((double) ur_int(a)) > ur_decimal(b) )
                        return 1;
                    if( ((double) ur_int(a)) < ur_decimal(b) )
                        return -1;
                }
            }
            break;
    }
    return 0;
}


void decimal_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    (void) depth;
    ur_strAppendDouble( str, ur_decimal(cell) );
}


UDatatype dt_decimal =
{
    "decimal!",
    decimal_make,           decimal_make,           unset_copy,
    decimal_compare,        unset_select,
    decimal_toString,       decimal_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_BIGNUM


#include "bignum.h"


int bignum_make( UThread* ut, const UCell* from, UCell* res )
{
    switch( ur_type(from) )
    {
        case UT_NONE:
            ur_setId(res, UT_BIGNUM);
            bignum_zero( res );
            break;
        case UT_LOGIC:
        case UT_CHAR:
        case UT_INT:
            ur_setId(res, UT_BIGNUM);
            bignum_seti( res, ur_int(from) );
            break;
        case UT_DECIMAL:
            ur_setId(res, UT_BIGNUM);
            bignum_setd( res, ur_decimal(from) );
            break;
        case UT_BIGNUM:
            *res = *from;
            break;
        case UT_STRING:
        {
            USeriesIter si;
            ur_seriesSlice( ut, &si, from );
            ur_setId(res, UT_BIGNUM);
            if( ur_strIsUcs2(si.buf) )
                bignum_zero( res );
            else
                bignum_setl(res, str_toInt64( si.buf->ptr.c + si.it,
                                              si.buf->ptr.c + si.end, 0) );
        }
            break;
        default:
            return ur_error( ut, UR_ERR_TYPE,
                "make bignum! expected number or none!/logic!/char!/string!" );
    }
    return UR_OK;
}


void bignum_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    int64_t n = bignum_l(cell);
    (void) ut;
    (void) depth;
    if( ur_flags(cell, UR_FLAG_INT_HEX) )
    {
        ur_strAppendCStr( str, "0x" );
        ur_strAppendHex( str, n & 0xffffffff, n >> 32 );
    }
    else
        ur_strAppendInt64( str, n );
}


UDatatype dt_bignum =
{
    "bignum!",
    bignum_make,            bignum_make,            unset_copy,
    unset_compare,          unset_select,
    bignum_toString,        bignum_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_TIME


int time_make( UThread* ut, const UCell* from, UCell* res )
{
    switch( ur_type(from) )
    {
        case UT_INT:
            ur_setId(res, UT_TIME);
            ur_decimal(res) = (double) ur_int(from);
            break;
        case UT_DECIMAL:
            ur_setId(res, UT_TIME);
            ur_decimal(res) = ur_decimal(from);
            break;
        default:
            return ur_error( ut, UR_ERR_TYPE,
                "make time! expected int!/decimal!" );
    }
    return UR_OK;
}


void time_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    int seg;
    double n = ur_decimal(cell);
    (void) ut;
    (void) depth;


    if( n < 0.0 )
    {
        n = -n;
        ur_strAppendChar( str, '-' );
    }

    // Hours
    seg = (int) (n / 3600.0);
    if( seg )
        n -= seg * 3600.0;
    ur_strAppendInt( str, seg );
    ur_strAppendChar( str, ':' );

    // Minutes
    seg = (int) (n / 60.0);
    if( seg )
        n -= seg * 60.0;
    if( seg < 10 )
        ur_strAppendChar( str, '0' );
    ur_strAppendInt( str, seg );
    ur_strAppendChar( str, ':' );

    // Seconds
    if( n < 10.0 )
        ur_strAppendChar( str, '0' );
    ur_strAppendDouble( str, n );
}


UDatatype dt_time =
{
    "time!",
    time_make,              time_make,              unset_copy,
    decimal_compare,        unset_select,
    time_toString,          time_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_DATE


extern void date_toString( UThread*, const UCell*, UBuffer*, int );


UDatatype dt_date =
{
    "date!",
    unset_make,             unset_make,             unset_copy,
    decimal_compare,        unset_select,
    date_toString,          date_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_VEC3


static void vec3_setf( UCell* res, float n )
{
    float* f = res->vec3.xyz;
    f[0] = f[1] = f[2] = n;
}


extern int vector_pickFloatV( const UBuffer*, UIndex n, float* fv, int count );

int vec3_make( UThread* ut, const UCell* from, UCell* res )
{
    ur_setId(res, UT_VEC3);
    switch( ur_type(from) )
    {
        case UT_NONE:
            vec3_setf( res, 0.0f );
            break;
        case UT_LOGIC:
        case UT_INT:
            vec3_setf( res, (float) ur_int(from) );
            break;
        case UT_DECIMAL:
            vec3_setf( res, (float) ur_decimal(from) );
            break;
        case UT_VEC3:
            memCpy( res->vec3.xyz, from->vec3.xyz, 3 * sizeof(float) );
            break;
        case UT_BLOCK:
        {
            UBlockIter bi;
            const UCell* cell;
            float num;
            int len = 0;

            ur_blkSlice( ut, &bi, from );
            ur_foreach( bi )
            {
                if( ur_is(bi.it, UT_WORD) )
                {
                    cell = ur_wordCell( ut, bi.it );
                    if( ! cell )
                        return UR_THROW;
                }
#if 0
                else if( ur_is(bi.it, UT_PATH) )
                {
                    if( ! ur_pathCell( ut, bi.it, res ) )
                        return UR_THROW;
                }
#endif
                else
                {
                    cell = bi.it;
                }

                if( ur_is(cell, UT_INT) )
                    num = (float) ur_int(cell);
                else if( ur_is(cell, UT_DECIMAL) )
                    num = (float) ur_decimal(cell);
                else
                    break;

                res->vec3.xyz[ len ] = num;
                if( ++len == 3 )
                    return UR_OK;
            }
            while( len < 3 )
                res->vec3.xyz[ len++ ] = 0.0f;
        }
            break;
        case UT_VECTOR:
        {
            int len;
            len = vector_pickFloatV( ur_bufferSer(from), from->series.it,
                                     res->vec3.xyz, 3 );
            while( len < 3 )
                res->vec3.xyz[ len++ ] = 0.0f;
        }
            break;
        default:
            return ur_error( ut, UR_ERR_TYPE,
                    "make vec3! expected none!/logic!/int!/decimal!/block!" );
    }
    return UR_OK;
}


void vec3_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) ut;
    for( depth = 0; depth < 3; ++depth )
    {
        if( depth )
            ur_strAppendChar( str, ',' );
        ur_strAppendDouble( str, cell->vec3.xyz[ depth ] );
    }
}


/* index is zero-based */
void vec3_pick( const UCell* cell, int index, UCell* res )
{
    if( (index < 0) || (index >= 3) )
    {
        ur_setId(res, UT_NONE);
    }
    else
    {
        ur_setId(res, UT_DECIMAL);
        ur_decimal(res) = cell->vec3.xyz[ index ];
    }
}


static
int vec3_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    if( ur_is(bi->it, UT_INT) )
    {
        vec3_pick( cell, ur_int(bi->it) - 1, res );
        ++bi->it;
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_SCRIPT, "vec3 select expected int!" );
}


UDatatype dt_vec3 =
{
    "vec3!",
    vec3_make,              vec3_make,              unset_copy,
    unset_compare,          vec3_select,
    vec3_toString,          vec3_toString,
    unset_recycle,          unset_mark,             unset_destroy,
    unset_markBuf,          unset_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_WORD


int word_makeType( UThread* ut, const UCell* from, UCell* res, int ntype )
{
    UAtom atom;
    int type = ur_type(from);

    if( ur_isWordType( type ) )
    {
        *res = *from;
        ur_type(res) = ntype;
        return UR_OK;
    }
    else if( type == UT_STRING )
    {
        USeriesIter si;

        ur_seriesSlice( ut, &si, from );
        if( si.buf->form == UR_ENC_LATIN1 )
        {
            atom = ur_internAtom( ut, si.buf->ptr.c + si.it,
                                      si.buf->ptr.c + si.end );
        }
        else
        {
            UBuffer tmp;
            ur_strInit( &tmp, UR_ENC_LATIN1, 0 );
            ur_strAppend( &tmp, si.buf, si.it, si.end );
            atom = ur_internAtom( ut, tmp.ptr.c, tmp.ptr.c + tmp.used );
            ur_strFree( &tmp );
        }
set_atom:
        ur_setId(res, ntype);
        ur_setWordUnbound(res, atom);
        return UR_OK;
    }
    else if( type == UT_DATATYPE )
    {
        atom = ur_datatype(from);
        if( atom < UT_MAX )
            goto set_atom;
    }
    return ur_error( ut, UR_ERR_TYPE, "make word! expected word!/string!" );
}


int word_make( UThread* ut, const UCell* from, UCell* res )
{
    return word_makeType( ut, from, res, UT_WORD );
}


/*
  Returns atom (if cell is any word), datatype atom (if cell is a simple
  datatype), or -1.
*/
static int word_atomOrType( const UCell* cell )
{
    int type = ur_type(cell);
    if( ur_isWordType(type) )
        return ur_atom(cell);
    if( type == UT_DATATYPE )
    {
        type = ur_datatype(cell);
        if( type < UT_MAX )
            return type;
    }
    return -1;
}


int word_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    switch( test )
    {
        case UR_COMPARE_SAME:
            return ((ur_atom(a) == ur_atom(b)) &&
                    (ur_binding(a) == ur_binding(b)) &&
                    (a->word.ctx == b->word.ctx));

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
        {
            int atomA = word_atomOrType( a );
            if( (atomA > -1) && (atomA == word_atomOrType(b)) )
                return 1;
        }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            if( ur_type(a) == ur_type(b) )
            {
#define ATOM_STR(cell)  (const uint8_t*) ur_atomCStr(ut, ur_atom(cell))
#define ATOM_LEN(str)   strLen((const char*) str)
                const uint8_t* strA = ATOM_STR(a);
                const uint8_t* strB = ATOM_STR(b);
                return compare_uint8_t( strA, strA + ATOM_LEN(strA),
                                        strB, strB + ATOM_LEN(strB) );
            }
            break;
    }
    return 0;
}


void word_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendCStr( str, ur_atomCStr( ut, ur_atom(cell) ) );
}


void word_mark( UThread* ut, UCell* cell )
{
    if( ur_binding(cell) == UR_BIND_THREAD )
    {
        UIndex n = cell->word.ctx;
        if( ur_markBuffer( ut, n ) )
            context_markBuf( ut, ur_buffer(n) );
    }
}


void word_toShared( UCell* cell )
{
    if( ur_binding(cell) == UR_BIND_THREAD )
    {
        ur_setBinding( cell, UR_BIND_ENV );
        cell->word.ctx = -cell->word.ctx;
    }
#if 1
    // FIXME: The core library should have no knowledge of other binding types.
    else if( ur_binding(cell) >= UR_BIND_USER ) // UR_BIND_FUNC, UR_BIND_OPTION
        cell->word.ctx = -cell->word.ctx;
#endif
}


UDatatype dt_word =
{
    "word!",
    word_make,              word_make,              unset_copy,
    word_compare,           unset_select,
    word_toString,          word_toString,
    unset_recycle,          word_mark,              unset_destroy,
    unset_markBuf,          word_toShared,          unset_bind
};


//----------------------------------------------------------------------------
// UT_LITWORD


int litword_make( UThread* ut, const UCell* from, UCell* res )
{
    return word_makeType( ut, from, res, UT_LITWORD );
}


void litword_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendChar( str, '\'' );
    ur_strAppendCStr( str, ur_atomCStr( ut, ur_atom(cell) ) );
}


UDatatype dt_litword =
{
    "lit-word!",
    litword_make,           litword_make,           unset_copy,
    word_compare,           unset_select,
    litword_toString,       word_toString,
    unset_recycle,          word_mark,              unset_destroy,
    unset_markBuf,          word_toShared,          unset_bind
};


//----------------------------------------------------------------------------
// UT_SETWORD


int setword_make( UThread* ut, const UCell* from, UCell* res )
{
    return word_makeType( ut, from, res, UT_SETWORD );
}


void setword_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendCStr( str, ur_atomCStr( ut, ur_atom(cell) ) );
    ur_strAppendChar( str, ':' );
}


UDatatype dt_setword =
{
    "set-word!",
    setword_make,           setword_make,           unset_copy,
    word_compare,           unset_select,
    setword_toString,       word_toString,
    unset_recycle,          word_mark,              unset_destroy,
    unset_markBuf,          word_toShared,          unset_bind
};


//----------------------------------------------------------------------------
// UT_GETWORD


int getword_make( UThread* ut, const UCell* from, UCell* res )
{
    return word_makeType( ut, from, res, UT_GETWORD );
}


void getword_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendChar( str, ':' );
    ur_strAppendCStr( str, ur_atomCStr( ut, ur_atom(cell) ) );
}


UDatatype dt_getword =
{
    "get-word!",
    getword_make,           getword_make,           unset_copy,
    word_compare,           unset_select,
    getword_toString,       word_toString,
    unset_recycle,          word_mark,              unset_destroy,
    unset_markBuf,          word_toShared,          unset_bind
};


//----------------------------------------------------------------------------
// UT_OPTION


void option_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendChar( str, '/' );
    ur_strAppendCStr( str, ur_atomCStr( ut, ur_atom(cell) ) );
}


UDatatype dt_option =
{
    "option!",
    unset_make,             unset_make,             unset_copy,
    word_compare,           unset_select,
    option_toString,        option_toString,
    unset_recycle,          word_mark,              unset_destroy,
    unset_markBuf,          word_toShared,          unset_bind
};


//----------------------------------------------------------------------------
// UT_BINARY


void binary_copy( UThread* ut, const UCell* from, UCell* res )
{
    UBinaryIter bi;
    UIndex n;
    int len;

    ur_binSlice( ut, &bi, from );
    len = bi.end - bi.it;
    n = ur_makeBinary( ut, len );       // Invalidates bi.buf.
    if( len )
        ur_binAppendData( ur_buffer(n), bi.it, len );

    ur_setId( res, ur_type(from) );     // Handle binary! & bitset!
    ur_setSeries( res, n, 0 );
}


int binary_make( UThread* ut, const UCell* from, UCell* res )
{
    int type = ur_type(from);
    if( type == UT_INT )
    {
        ur_makeBinaryCell( ut, ur_int(from), res );
        return UR_OK;
    }
    else if( type == UT_BINARY )
    {
        binary_copy( ut, from, res );
        return UR_OK;
    }
    else if( ur_isStringType(type) || (type == UT_VECTOR) )
    {
        USeriesIter si;
        UBuffer* bin;

        bin = ur_makeBinaryCell( ut, 0, res );
        ur_seriesSlice( ut, &si, from );
        ur_binAppendArray( bin, &si );
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE,
                     "make binary! expected int!/binary!/string!/file!" );
}

 
int binary_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    switch( test )
    {
        case UR_COMPARE_SAME:
            return ((a->series.buf == b->series.buf) &&
                    (a->series.it == b->series.it) &&
                    (a->series.end == b->series.end));

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            if( ! ur_is(a, UT_BINARY) || ! ur_is(b, UT_BINARY) )
                break;
            if( (a->series.buf == b->series.buf) &&
                (a->series.it == b->series.it) &&
                (a->series.end == b->series.end) )
                return 1;
            {
            USeriesIter ai;
            USeriesIter bi;

            ur_seriesSlice( ut, &ai, a );
            ur_seriesSlice( ut, &bi, b );

            if( (ai.end - ai.it) == (bi.end - bi.it) )
            {
                const uint8_t* pos;
                const uint8_t* end = bi.buf->ptr.b + bi.end;
                pos = match_pattern_uint8_t( ai.buf->ptr.b + ai.it,
                            ai.buf->ptr.b + ai.end,
                            bi.buf->ptr.b + bi.it, end );
                return pos == end;
            }
            }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            if( ur_is(a, UT_BINARY) && ur_is(b, UT_BINARY) )
            {
                USeriesIter ai;
                USeriesIter bi;

                ur_seriesSlice( ut, &ai, a );
                ur_seriesSlice( ut, &bi, b );

                return compare_uint8_t( ai.buf->ptr.b + ai.it,
                                        ai.buf->ptr.b + ai.end,
                                        bi.buf->ptr.b + bi.it,
                                        bi.buf->ptr.b + bi.end );
            }
            break;
    }
    return 0;
}


static inline int toNibble( int c )
{
    return (c < 10) ? c + '0' : c + 'A' - 10;
}


void binary_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    UBinaryIter bi;
    int c;
    (void) depth;

    ur_strAppendCStr( str, "#{" );
    ur_binSlice( ut, &bi, cell );
    ur_foreach( bi )
    {
        c = *bi.it;
        ur_strAppendChar( str, toNibble(c >> 4) );
        ur_strAppendChar( str, toNibble(c & 0x0f) );
    }
    ur_strAppendChar( str, '}' );
}


void binary_mark( UThread* ut, UCell* cell )
{
    UIndex n = cell->series.buf;
    if( n > UR_INVALID_BUF )    // Also acts as (! ur_isShared(n))
        ur_markBuffer( ut, n );
}


void binary_destroy( UBuffer* buf )
{
    ur_binFree( buf );
}


void binary_toShared( UCell* cell )
{
    UIndex n = cell->series.buf;
    if( n > UR_INVALID_BUF )
        cell->series.buf = -n;
}


void binary_pick( const UBuffer* buf, UIndex n, UCell* res )
{
    if( n > -1 && n < buf->used )
    {
        ur_setId(res, UT_INT);
        ur_int(res) = buf->ptr.b[ n ];
    }
    else
        ur_setId(res, UT_NONE);
}


void binary_poke( UBuffer* buf, UIndex n, const UCell* val )
{
    if( n > -1 && n < buf->used )
    {
        if( ur_is(val, UT_CHAR) || ur_is(val, UT_INT) )
            buf->ptr.b[ n ] = ur_int(val);
    }
}


int binary_append( UThread* ut, UBuffer* buf, const UCell* val )
{
    int vt = ur_type(val);

    if( (vt == UT_BINARY) || ur_isStringType(vt) )
    {
        USeriesIter si;
        int len;

        ur_seriesSlice( ut, &si, val );
        len = si.end - si.it;
        if( len )
        {
            if( (vt != UT_BINARY) && ur_strIsUcs2(si.buf) )
            {
                len *= 2;
                si.it *= 2;
            }
            ur_binAppendData( buf, si.buf->ptr.b + si.it, len );

        }
        return UR_OK;
    }
    else if( (vt == UT_CHAR) || (vt == UT_INT) )
    {
        ur_binReserve( buf, buf->used + 1 );
        buf->ptr.b[ buf->used++ ] = ur_int(val);
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE,
                     "append binary! expected char!/int!/binary!/string!" );
}


int binary_change( UThread* ut, USeriesIterM* si, const UCell* val,
                   UIndex part )
{
    int type = ur_type(val);
    if( type == UT_CHAR || type == UT_INT )
    {
        UBuffer* buf = si->buf;
        if( si->it == buf->used )
            ur_binReserve( buf, ++buf->used );
        buf->ptr.b[ si->it++ ] = ur_int(val);
        if( part > 1 )
            ur_binErase( buf, si->it, part - 1 );
        return UR_OK;
    }
    else if( type == UT_BINARY )
    {
        UBinaryIter ri;
        UBuffer* buf;
        UIndex newUsed;
        int slen;

        ur_binSlice( ut, &ri, val );
        slen = ri.end - ri.it;
        if( slen > 0 )
        {
            buf = si->buf;
            if( part > 0 )
            {
                if( part < slen )
                    ur_binExpand( buf, si->it, slen - part );
                else if( part > slen )
                    ur_binErase( buf, si->it, part - slen );
                newUsed = buf->used;
            }
            else
            {
                newUsed = si->it + slen;
                if( newUsed < buf->used )
                    newUsed = buf->used;
            }

            // TODO: Handle overwritting self when buf is val.

            buf->used = si->it;
            ur_binAppendData( buf, ri.it, slen );
            si->it = buf->used;
            buf->used = newUsed;
        }
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE,
                     "change binary! expected char!/int!/binary!" );
}


void binary_remove( UThread* ut, USeriesIterM* si, UIndex part )
{
    (void) ut;
    ur_binErase( si->buf, si->it, (part > 0) ? part : 1 );
}


int binary_find( UThread* ut, const USeriesIter* si, const UCell* val, int opt )
{
    const uint8_t* it;
    const UBuffer* buf = si->buf;
    int vt = ur_type(val);

    if( (vt == UT_CHAR) || (vt == UT_INT) )
    {
        it = buf->ptr.b;
        if( opt & UR_FIND_LAST )
            it = find_last_uint8_t( it + si->it, it + si->end, ur_int(val) );
        else
            it = find_uint8_t( it + si->it, it + si->end, ur_int(val) );
        if( it )
            return it - buf->ptr.b;
    }
    else if( ur_isStringType( vt ) || (vt == UT_BINARY) )
    {
        USeriesIter siV;
        const uint8_t* itV;

        ur_seriesSlice( ut, &siV, val );

        if( (vt != UT_BINARY) && ur_strIsUcs2(siV.buf) )
            return -1;      // TODO: Handle ucs2.

        it = buf->ptr.b;
        itV = siV.buf->ptr.b;
        it = find_pattern_uint8_t( it + si->it, it + si->end,
                                   itV + siV.it, itV + siV.end );
        if( it )
            return it - buf->ptr.b;
    }
    return -1;
}


int binary_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    if( ur_is(bi->it, UT_INT) )
    {
        const UBuffer* buf = ur_bufferSer(cell);
        binary_pick( buf, cell->series.it + ur_int(bi->it) - 1, res );
        ++bi->it;
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_SCRIPT, "binary select expected int!" );
}


USeriesType dt_binary =
{
    {
    "binary!",
    binary_make,            binary_make,            binary_copy,
    binary_compare,         binary_select,
    binary_toString,        binary_toString,
    unset_recycle,          binary_mark,            binary_destroy,
    unset_markBuf,          binary_toShared,        unset_bind
    },
    binary_pick,            binary_poke,            binary_append,
    binary_change,          binary_remove,          binary_find
};


//----------------------------------------------------------------------------
// UT_BITSET


#define setBit(array,n)      (array[(n)>>3] |= 1<<((n)&7))

int bitset_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_BINARY) )
    {
        return binary_make( ut, from, res );
    }
    else if( ur_is(from, UT_STRING) )
    {
        UBuffer* buf;
        UBinaryIter si;
        uint8_t* bits;
        UIndex n;
       
        //buf = ur_makeBinaryCell( ut, 32, res, UT_BITSET );
        n = ur_makeBinary( ut, 32 );
        ur_setId(res, UT_BITSET);
        ur_setSeries(res, n, 0 );

        buf = ur_buffer(n);
        buf->used = 32;
        bits = buf->ptr.b;
        memSet( bits, 0, buf->used );

        ur_binSlice( ut, &si, from );
        if( ur_strIsUcs2(si.buf) )
        {
            return ur_error( ut, UR_ERR_INTERNAL,
                    "FIXME: make bitset! does not handle UCS2 strings" );
        }
        else
        {
            ur_foreach( si )
            {
                n = *si.it;
                setBit( bits, n );
            }
        }

        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE, "make bitset! expected string!" );
}


void bitset_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    (void) depth;
    ur_strAppendCStr( str, "make bitset! " );   // Eval dep.
    binary_toString( ut, cell, str, 0 );
}


USeriesType dt_bitset =
{
    {
    "bitset!",
    bitset_make,            bitset_make,            binary_copy,
    unset_compare,          unset_select,
    bitset_toString,        bitset_toString,
    unset_recycle,          binary_mark,            binary_destroy,
    unset_markBuf,          binary_toShared,        unset_bind
    },
    binary_pick,            binary_poke,            binary_append,
    binary_change,          binary_remove,          binary_find
};


//----------------------------------------------------------------------------
// UT_STRING


void string_copy( UThread* ut, const UCell* from, UCell* res )
{
    USeriesIter si;
    UBuffer* buf;
    int len;

    ur_seriesSlice( ut, &si, from );
    len = si.end - si.it;
    // Make invalidates si.buf.
    buf = ur_makeStringCell( ut, si.buf->form, len, res );
    if( len )
        ur_strAppend( buf, ur_bufferSer(from), si.it, si.end );
}


int string_convert( UThread* ut, const UCell* from, UCell* res )
{
    int type = ur_type(from);
    if( ur_isStringType(type) )
    {
        string_copy( ut, from, res );
    }
    else if( type == UT_BINARY )
    {
        UBinaryIter bi;
        UIndex n;

        ur_binSlice( ut, &bi, from );
        n = ur_makeStringUtf8( ut, bi.it, bi.end );

        ur_setId( res, UT_STRING );
        ur_setSeries( res, n, 0 );
    }
    else
    {
        DT( type )->toString( ut, from,
                              ur_makeStringCell(ut, UR_ENC_LATIN1, 0, res), 0 );
    }
    return UR_OK;
}


int string_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_INT) )
    {
        ur_makeStringCell( ut, UR_ENC_LATIN1, ur_int(from), res );
        return UR_OK;
    }
    return string_convert( ut, from, res );
}

 
#define COMPARE_IC(T) \
int compare_ic_ ## T( const T* it, const T* end, \
        const T* itB, const T* endB ) { \
    int ca, cb; \
    int lenA = end - it; \
    int lenB = endB - itB; \
    while( it < end && itB < endB ) { \
        ca = ur_charLowercase( *it++ ); \
        cb = ur_charLowercase( *itB++ ); \
        if( ca > cb ) \
            return 1; \
        if( ca < cb ) \
            return -1; \
    } \
    if( lenA > lenB ) \
        return 1; \
    if( lenA < lenB ) \
        return -1; \
    return 0; \
}

COMPARE_IC(uint8_t)
COMPARE_IC(uint16_t)


int string_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    switch( test )
    {
        case UR_COMPARE_SAME:
            return ((a->series.buf == b->series.buf) &&
                    (a->series.it == b->series.it) &&
                    (a->series.end == b->series.end));

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            if( ! ur_isStringType(ur_type(a)) || ! ur_isStringType(ur_type(b)) )
                break;
            if( (a->series.buf == b->series.buf) &&
                (a->series.it == b->series.it) &&
                (a->series.end == b->series.end) )
                return 1;
            {
            USeriesIter ai;
            USeriesIter bi;
            int len;

            ur_seriesSlice( ut, &ai, a );
            ur_seriesSlice( ut, &bi, b );
            len = ai.end - ai.it;

            if( (bi.end - bi.it) == len )
            {
                if( (len == 0) ||
                    (ur_strMatch( &ai, &bi, (test == UR_COMPARE_EQUAL_CASE) )
                        == len ) )
                    return 1;
            }
            }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            if( ! ur_isStringType(ur_type(a)) || ! ur_isStringType(ur_type(b)) )
                break;
            {
            USeriesIter ai;
            USeriesIter bi;

            ur_seriesSlice( ut, &ai, a );
            ur_seriesSlice( ut, &bi, b );

            if( ai.buf->elemSize != bi.buf->elemSize )
                return 0;       // TODO: Handle all different encodings.

            if( ur_strIsUcs2(ai.buf) )
            {
                int (*func)(const uint16_t*, const uint16_t*,
                            const uint16_t*, const uint16_t* );
                func = (test == UR_COMPARE_ORDER) ? compare_ic_uint16_t
                                                  : compare_uint16_t;
                return func( ai.buf->ptr.u16 + ai.it,
                             ai.buf->ptr.u16 + ai.end,
                             bi.buf->ptr.u16 + bi.it,
                             bi.buf->ptr.u16 + bi.end );
            }
            else
            {
                int (*func)(const uint8_t*, const uint8_t*,
                             const uint8_t*, const uint8_t* );
                func = (test == UR_COMPARE_ORDER) ? compare_ic_uint8_t
                                                  : compare_uint8_t;
                return func( ai.buf->ptr.b + ai.it,
                             ai.buf->ptr.b + ai.end,
                             bi.buf->ptr.b + bi.it,
                             bi.buf->ptr.b + bi.end );
            }
            }
    }
    return 0;
}


// Newline (\n) or double quote (").
//                                        0x04
static uint8_t _strLongChars[5] = { 0x00, 0x00, 0x00, 0x00, 0x04 };

static uint8_t _strEscapeChars[16] = {
   0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x28
};


void string_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    const int longLen = 40;
    int len;
    int quote;
    UIndex escPos;
    USeriesIter si;

    ur_seriesSlice( ut, &si, cell );
    len = si.end - si.it;

    if( len < 1 )
    {
        ur_strAppendCStr( str, "\"\"" );
        return;
    }

    if( (len > longLen) ||
        (ur_strFindChars( si.buf, 0, (len < longLen) ? len : longLen,
                          _strLongChars, sizeof(_strLongChars) ) > -1) )
    {
        ur_strAppendChar( str, '{' );
        quote = '}';
    }
    else
    {
        ur_strAppendChar( str, '"' );
        quote = '"';
    }

    while( 1 )
    {
        escPos = ur_strFindChars( si.buf, si.it, si.end,
                                  _strEscapeChars, sizeof(_strEscapeChars) );
        if( escPos < 0 )
        {
            ur_strAppend( str, si.buf, si.it, si.end );
            break;
        }

        if( escPos != si.it )
            ur_strAppend( str, si.buf, si.it, escPos );
        si.it = escPos + 1;

        depth = ur_strIsUcs2(si.buf) ? si.buf->ptr.u16[ escPos ]
                                     : si.buf->ptr.b[ escPos ];
        switch( depth )
        {
           case '\t':
                ur_strAppendCStr( str, "^-" );
                break;

            case '\n':
                if( quote == '"' )
                    ur_strAppendCStr( str, "^/" );
                else
                    ur_strAppendChar( str, '\n' );
                break;

            case '^':
                ur_strAppendCStr( str, "^^" );
                break;

            case '{':
                if( quote == '"' )
                    ur_strAppendChar( str, '{' );
                else
                    ur_strAppendCStr( str, "^{" );
                break;

            case '}':
                if( quote == '"' )
                    ur_strAppendChar( str, '}' );
                else
                    ur_strAppendCStr( str, "^}" );
                break;

            default:
                ur_strAppendChar( str, '^' );
                ur_strAppendChar( str, toNibble( depth ) );
                break;
        }
    }

    ur_strAppendChar( str, quote );
}


void string_toText( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    const UBuffer* ss = ur_bufferSer(cell);
    (void) depth;
    ur_strAppend( str, ss, cell->series.it,
                  (cell->series.end > -1) ? cell->series.end : ss->used );
}


void array_destroy( UBuffer* buf )
{
    ur_arrFree( buf );
}


void string_pick( const UBuffer* buf, UIndex n, UCell* res )
{
    if( n > -1 && n < buf->used )
    {
        ur_setId(res, UT_CHAR);
        ur_int(res) = ur_strIsUcs2(buf) ? buf->ptr.u16[ n ]
                                        : buf->ptr.b[ n ];
    }
    else
        ur_setId(res, UT_NONE);
}


void string_poke( UBuffer* buf, UIndex n, const UCell* val )
{
    if( n > -1 && n < buf->used )
    {
        if( ur_is(val, UT_CHAR) || ur_is(val, UT_INT) )
        {
            if( ur_strIsUcs2(buf) )
                buf->ptr.u16[ n ] = ur_int(val);
            else
                buf->ptr.b[ n ] = ur_int(val);
        }
    }
}


int string_append( UThread* ut, UBuffer* buf, const UCell* val )
{
    int type = ur_type(val);

    if( ur_isStringType(type) )
    {
        USeriesIter si;
        ur_seriesSlice( ut, &si, val );
        ur_strAppend( buf, si.buf, si.it, si.end );
        return UR_OK;
    }
    else if( type == UT_CHAR )
    {
        ur_strAppendChar( buf, ur_int(val) );
        return UR_OK;
    }
    else if( type == UT_BLOCK )
    {
        UBlockIter bi;
        UDatatype** dt = ut->types;
        ur_blkSlice( ut, &bi, val );
        ur_foreach( bi )
        {
            dt[ ur_type(bi.it) ]->toText( ut, bi.it, buf, 0 );
            //ur_toText( ut, bi.it, str );
        }
        return UR_OK;
    }
    else
    {
        DT( type )->toText( ut, val, buf, 0 );
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE, "append string! expected char!/string!" );
}


/*
  \param si     String to change.
  \param ri     Replacement string.
  \param part   Replace this many characters, regardless of the length of ri.

  \return  si->it is placed at end of change and si->buf.used may be modified.
*/
static void ur_strChange( USeriesIterM* si, USeriesIter* ri, UIndex part )
{
    UBuffer* buf;
    UIndex newUsed;
    int slen = ri->end - ri->it;

    if( slen > 0 )
    {
        buf = si->buf;
        if( part > 0 )
        {
            if( part < slen )
                ur_arrExpand( buf, si->it, slen - part );
            else if( part > slen )
                ur_arrErase( buf, si->it, part - slen );
            newUsed = buf->used;
        }
        else
        {
            newUsed = si->it + slen;
            if( newUsed < buf->used )
                newUsed = buf->used;
        }

        // TODO: Handle overwritting self when buf is val.

        buf->used = si->it;
        ur_strAppend( buf, ri->buf, ri->it, ri->end );
        si->it = buf->used;
        buf->used = newUsed;
    }
}


int string_change( UThread* ut, USeriesIterM* si, const UCell* val,
                   UIndex part )
{
    USeriesIter siV;
    int type = ur_type(val);

    if( type == UT_CHAR )
    {
        UBuffer* buf = si->buf;
        if( si->it == buf->used )
            ur_arrReserve( buf, ++buf->used );

        if( ur_strIsUcs2(buf) )
            buf->ptr.u16[ si->it ] = ur_int(val);
        else
            buf->ptr.b[ si->it ] = ur_int(val);
        ++si->it;

        if( part > 1 )
            ur_arrErase( buf, si->it, part - 1 );
    }
    else if( ur_isStringType(type) )
    {
        ur_seriesSlice( ut, &siV, val );
        ur_strChange( si, &siV, part );
    }
    else
    {
        UBuffer tmp;

        ur_strInit( &tmp, UR_ENC_LATIN1, 0 );
        DT( type )->toString( ut, val, &tmp, 0 );

        siV.buf = &tmp;
        siV.it  = 0;
        siV.end = tmp.used;

        ur_strChange( si, &siV, part );
        ur_strFree( &tmp );
    }
    return UR_OK;
}


void string_remove( UThread* ut, USeriesIterM* si, UIndex part )
{
    (void) ut;
    ur_arrErase( si->buf, si->it, (part > 0) ? part : 1 );
}


/*
  Returns pointer to val or zero if val not found.
*/
#define FIND_LC(T) \
const T* find_lc_ ## T( const T* it, const T* end, T val ) { \
    while( it != end ) { \
        if( ur_charLowercase(*it) == val ) \
            return it; \
        ++it; \
    } \
    return 0; \
}

FIND_LC(uint8_t)
FIND_LC(uint16_t)


/*
  Returns pointer to val or zero if val not found.
*/
#define FIND_LC_LAST(T) \
const T* find_lc_last_ ## T( const T* it, const T* end, T val ) { \
    while( it != end ) { \
        --end; \
        if( ur_charLowercase(*end) == val ) \
            return end; \
    } \
    return 0; \
}

FIND_LC_LAST(uint8_t)
FIND_LC_LAST(uint16_t)


/*
  Returns first occurance of pattern or 0 if it is not found.
*/
#define FIND_LC_PATTERN(T) \
const T* find_lc_pattern_ ## T( const T* it, const T* end, \
        const T* pit, const T* pend ) { \
    T pfirst = *pit++; \
    while( it != end ) { \
        if( ur_charLowercase(*it) == pfirst ) { \
            const T* in = it + 1; \
            const T* p  = pit; \
            while( p != pend && in != end ) { \
                if( ur_charLowercase(*in) != *p ) \
                    break; \
                ++in; \
                ++p; \
            } \
            if( p == pend ) \
                return it; \
        } \
        ++it; \
    } \
    return 0; \
}

FIND_LC_PATTERN(uint8_t)
FIND_LC_PATTERN(uint16_t)


int string_find( UThread* ut, const USeriesIter* si, const UCell* val, int opt )
{
    const UBuffer* buf = si->buf;

    if( ur_is(val, UT_CHAR) )
    {
        int ch = ur_int(val);
        if( ur_strIsUcs2(buf) )
        {
            const uint16_t* it = buf->ptr.u16;
            if( opt & UR_FIND_CASE )
            {
                if( opt & UR_FIND_LAST )
                    it = find_last_uint16_t( it + si->it, it + si->end, ch );
                else
                    it = find_uint16_t( it + si->it, it + si->end, ch );
            }
            else
            {
                ch = ur_charLowercase( ch );
                if( opt & UR_FIND_LAST )
                    it = find_lc_last_uint16_t( it + si->it, it + si->end, ch );
                else
                    it = find_lc_uint16_t( it + si->it, it + si->end, ch );
            }
            if( it )
                return it - buf->ptr.u16;
        }
        else
        {
            const uint8_t* it = buf->ptr.b;
            if( opt & UR_FIND_CASE )
            {
                if( opt & UR_FIND_LAST )
                    it = find_last_uint8_t( it + si->it, it + si->end, ch );
                else
                    it = find_uint8_t( it + si->it, it + si->end, ch );
            }
            else
            {
                ch = ur_charLowercase( ch );
                if( opt & UR_FIND_LAST )
                    it = find_lc_last_uint8_t( it + si->it, it + si->end, ch );
                else
                    it = find_lc_uint8_t( it + si->it, it + si->end, ch );
            }
            if( it )
                return it - buf->ptr.b;
        }
    }
    else if( ur_isStringType( ur_type(val) ) )
    {
        UBuffer pat;
        USeriesIter siV;
        int pos = -1;

        ur_seriesSlice( ut, &siV, val );

        if( (buf->form != siV.buf->form) || ! (opt & UR_FIND_CASE) )
        {
            ur_strInit( &pat, buf->form, 0 );
            ur_strAppend( &pat, siV.buf, siV.it, siV.end );
            siV.buf = &pat;
        }
        else
            pat.ptr.b = 0;

        // TODO: Implement UR_FIND_LAST.
        if( ur_strIsUcs2(buf) )
        {
            const uint16_t* it = buf->ptr.u16;
            const uint16_t* itV = siV.buf->ptr.u16;
            if( opt & UR_FIND_CASE )
            {
                it = find_pattern_uint16_t( it + si->it, it + si->end,
                                            itV + siV.it, itV + siV.end );
            }
            else
            {
                ur_strLowercase( &pat, 0, pat.used );
                it = find_lc_pattern_uint16_t( it + si->it, it + si->end,
                                               itV, itV + pat.used );
            }
            if( it )
                pos = it - buf->ptr.u16;
        }
        else
        {
            const uint8_t* it = buf->ptr.b;
            const uint8_t* itV = siV.buf->ptr.b;
            if( opt & UR_FIND_CASE )
            {
                it = find_pattern_uint8_t( it + si->it, it + si->end,
                                           itV + siV.it, itV + siV.end );
            }
            else
            {
                ur_strLowercase( &pat, 0, pat.used );
                it = find_lc_pattern_uint8_t( it + si->it, it + si->end,
                                              itV, itV + pat.used );
            }
            if( it )
                pos = it - buf->ptr.b;
        }

        if( pat.ptr.b )
            ur_strFree( &pat );
        return pos;
    }
    else if( ur_is(val, UT_BINARY) )
    {
        UBinaryIter bi;
        ur_binSlice( ut, &bi, val );
        if( ! ur_strIsUcs2(buf) )
        {
            const uint8_t* it = buf->ptr.b;
            it = find_pattern_uint8_t( it + si->it, it + si->end,
                                       bi.it, bi.end );
            if( it )
                return it - buf->ptr.b;
        }
    }
    else if( ur_is(val, UT_BITSET) )
    {
        const UBuffer* bbuf = ur_bufferSer(val);
        if( ur_strIsUcs2(buf) )
        {
            const uint16_t* it = buf->ptr.u16;
            it = find_charset_uint16_t( it + si->it, it + si->end,
                                        bbuf->ptr.b, bbuf->used );
            if( it )
                return it - buf->ptr.u16;
        }
        else
        {
            const uint8_t* it = buf->ptr.b;
            it = find_charset_uint8_t( it + si->it, it + si->end,
                                       bbuf->ptr.b, bbuf->used );
            if( it )
                return it - buf->ptr.b;
        }
    }
    return -1;
}


int string_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    if( ur_is(bi->it, UT_INT) )
    {
        const UBuffer* buf = ur_bufferSer(cell);
        string_pick( buf, cell->series.it + ur_int(bi->it) - 1, res );
        ++bi->it;
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_SCRIPT, "string select expected int!" );
}


USeriesType dt_string =
{
    {
    "string!",
    string_make,            string_convert,         string_copy,
    string_compare,         string_select,
    string_toString,        string_toText,
    unset_recycle,          binary_mark,            array_destroy,
    unset_markBuf,          binary_toShared,        unset_bind
    },
    string_pick,            string_poke,            string_append,
    string_change,          string_remove,          string_find
};


//----------------------------------------------------------------------------
// UT_FILE


int file_make( UThread* ut, const UCell* from, UCell* res )
{
    int ok = string_make( ut, from, res );
    if( ok )
        ur_type(res) = UT_FILE;
    return ok;
}


int file_convert( UThread* ut, const UCell* from, UCell* res )
{
    int ok = string_convert( ut, from, res );
    if( ok )
        ur_type(res) = UT_FILE;
    return ok;
}


void file_copy( UThread* ut, const UCell* from, UCell* res )
{
    string_copy( ut, from, res );
    ur_type(res) = UT_FILE;
}


// "()[]; "
static uint8_t _fileQuoteChars[12] =
{
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x28
};

void file_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    USeriesIter si;
    (void) depth;

    ur_seriesSlice( ut, &si, cell );

    if( ur_strFindChars( si.buf, si.it, si.end, _fileQuoteChars,
                         sizeof(_fileQuoteChars) ) > -1 )
    {
        ur_strAppendCStr( str, "%\"" );
        ur_strAppend( str, si.buf, si.it, si.end );
        ur_strAppendChar( str, '"' );
    }
    else
    {
        ur_strAppendChar( str, '%' );
        ur_strAppend( str, si.buf, si.it, si.end );
    }
}


USeriesType dt_file =
{
    {
    "file!",
    file_make,              file_convert,           file_copy,
    string_compare,         string_select,
    file_toString,          string_toText,
    unset_recycle,          binary_mark,            array_destroy,
    unset_markBuf,          binary_toShared,        unset_bind
    },
    string_pick,            string_poke,            string_append,
    string_change,          string_remove,          string_find
};


//----------------------------------------------------------------------------
// UT_BLOCK


void block_copy( UThread* ut, const UCell* from, UCell* res )
{
    UBlockIter bi;
    UBuffer* buf;
    int len;

    ur_blkSlice( ut, &bi, from );
    len = bi.end - bi.it;
    // Make invalidates bi.buf.
    buf = ur_makeBlockCell( ut, ur_type(from), len, res );
    if( len )
        ur_blkAppendCells( buf, bi.it, len );
}


int block_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_INT) )
    {
        ur_makeBlockCell( ut, UT_BLOCK, ur_int(from), res );
        return UR_OK;
    }
    else if( ur_is(from, UT_STRING) )
    {
        USeriesIter si;
        ur_seriesSlice( ut, &si, from );
        if( si.it == si.end )
        {
            ur_makeBlockCell( ut, UT_BLOCK, 0, res );
            return UR_OK;
        }
        else if( (si.buf->elemSize == 1) )
        {
            if( ur_tokenize( ut, si.buf->ptr.c + si.it,
                                 si.buf->ptr.c + si.end, res ) )
                return UR_OK;
        }
        else
        {
            ur_error( ut, UR_ERR_TYPE,
                      "FIXME: make block! does not handle UCS2 string!" );
        }
        return UR_THROW;
    }
    else if( ur_isBlockType( ur_type(from) )  )
    {
        block_copy( ut, from, res );
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE,
                     "make block! expected int!/string!/block!" );
}


int block_convert( UThread* ut, const UCell* from, UCell* res )
{
    int type = ur_type(from);

    if( type == UT_STRING )
    {
        return block_make( ut, from, res );
    }
    else if( ur_isBlockType( type )  )
    {
        block_copy( ut, from, res );
    }
    else
    {
        UBuffer* blk = ur_makeBlockCell( ut, UT_BLOCK, 1, res );
        ur_blkPush( blk, from );
    }
    return UR_OK;
}


int block_compare( UThread* ut, const UCell* a, const UCell* b, int test )
{
    switch( test )
    {
        case UR_COMPARE_SAME:
            return ((a->series.buf == b->series.buf) &&
                    (a->series.it == b->series.it) &&
                    (a->series.end == b->series.end));

        case UR_COMPARE_EQUAL:
        case UR_COMPARE_EQUAL_CASE:
            if( ur_type(a) != ur_type(b) )
                break;
            if( (a->series.buf == b->series.buf) &&
                (a->series.it == b->series.it) &&
                (a->series.end == b->series.end) )
                return 1;
            {
            UBlockIter ai;
            UBlockIter bi;
            UDatatype** dt;
            int t;

            ur_blkSlice( ut, &ai, a );
            ur_blkSlice( ut, &bi, b );

            if( (ai.end - ai.it) == (bi.end - bi.it) )
            {
                dt = ut->types;
                ur_foreach( ai )
                {
                    t = ur_type(ai.it);
                    if( t < ur_type(bi.it) )
                        t = ur_type(bi.it);
                    if( ! dt[ t ]->compare( ut, ai.it, bi.it, test ) )
                        return 0;
                    ++bi.it;
                }
                return 1;
            }
            }
            break;

        case UR_COMPARE_ORDER:
        case UR_COMPARE_ORDER_CASE:
            break;
    }
    return 0;
}


/*
  If depth is -1 then the outermost pair of braces will be omitted.
*/
void block_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    UBlockIter bi;
    const UCell* start;
    int brace = 0;

    if( depth > -1 )
    {
        switch( ur_type(cell) )
        {
            case UT_BLOCK:
                ur_strAppendChar( str, '[' );
                brace = ']';
                break;
            case UT_PAREN:
                ur_strAppendChar( str, '(' );
                brace = ')';
                break;
#ifdef UR_CONFIG_MACROS
            case UT_MACRO:
                ur_strAppendCStr( str, "^(" );
                brace = ')';
                break;
#endif
        }
    }

    ur_blkSlice( ut, &bi, cell );
    start = bi.it;

    ++depth;
    ur_foreach( bi )
    {
        if( bi.it->id.flags & UR_FLAG_SOL )
        {
            ur_strAppendChar( str, '\n' );
            ur_strAppendIndent( str, depth );
        }
        else if( bi.it != start )
        {
            ur_strAppendChar( str, ' ' );
        }
        ur_toStr( ut, bi.it, str, depth );
    }
    --depth;

    if( (start != bi.end) && (start->id.flags & UR_FLAG_SOL) )
    {
        ur_strAppendChar( str, '\n' );
        if( brace )
            ur_strAppendIndent( str, depth );
    }

    if( brace )
        ur_strAppendChar( str, brace );
}


void block_toText( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    UBlockIter bi;
    const UCell* start;
    (void) depth;

    ur_blkSlice( ut, &bi, cell );
    start = bi.it;

    ur_foreach( bi )
    {
        if( bi.it != start )
            ur_strAppendChar( str, ' ' );
        ur_toText( ut, bi.it, str );
    }
}


void block_markBuf( UThread* ut, UBuffer* buf )
{
    int t;
    UCell* it  = buf->ptr.cell;
    UCell* end = it + buf->used;
    while( it != end )
    {
        t = ur_type(it);
        if( t >= UT_REFERENCE_BUF )
        {
            DT( t )->mark( ut, it );
        }
        ++it;
    }
}


void block_mark( UThread* ut, UCell* cell )
{
    UIndex n = cell->series.buf;
    if( n > UR_INVALID_BUF )    // Also acts as (! ur_isShared(n))
    {
        if( ur_markBuffer( ut, n ) )
            block_markBuf( ut, ur_buffer(n) );
    }
}


void block_toShared( UCell* cell )
{
    UIndex n = cell->series.buf;
    if( n > UR_INVALID_BUF )
        cell->series.buf = -n;
}


void block_pick( const UBuffer* buf, UIndex n, UCell* res )
{
    if( n > -1 && n < buf->used )
        *res = buf->ptr.cell[ n ];
    else
        ur_setId(res, UT_NONE);
}


void block_poke( UBuffer* buf, UIndex n, const UCell* val )
{
    if( n > -1 && n < buf->used )
        buf->ptr.cell[ n ] = *val;
}


int block_append( UThread* ut, UBuffer* buf, const UCell* val )
{
    if( ur_is(val, UT_BLOCK) )
    {
        UBlockIter bi;
        ur_blkSlice( ut, &bi, val );
        if( bi.buf == buf )
        {
            ur_arrReserve( buf, buf->used + (bi.end - bi.it) );
            ur_blkSlice( ut, &bi, val );
        }
        ur_blkAppendCells( buf, bi.it, bi.end - bi.it );
    }
    else
    {
        ur_blkPush( buf, val );
    }
    return UR_OK;
}


int block_change( UThread* ut, USeriesIterM* si, const UCell* val,
                  UIndex part )
{
    if( ur_isBlockType( ur_type(val) ) )
    {
        UBlockIter ri;
        UBuffer* buf;
        UIndex newUsed;
        int slen;

        ur_blkSlice( ut, &ri, val );
        slen = ri.end - ri.it;
        if( slen > 0 )
        {
            buf = si->buf;
            if( part > 0 )
            {
                if( part < slen )
                    ur_arrExpand( buf, si->it, slen - part );
                else if( part > slen )
                    ur_arrErase( buf, si->it, part - slen );
                newUsed = buf->used;
            }
            else
            {
                newUsed = si->it + slen;
                if( newUsed < buf->used )
                    newUsed = buf->used;
            }

            // TODO: Handle overwritting self when buf is val.

            buf->used = si->it;
            ur_blkAppendCells( buf, ri.it, slen );
            si->it = buf->used;
            buf->used = newUsed;
        }
    }
    else
    {
        UBuffer* buf = si->buf;
        if( si->it == buf->used )
            ur_arrReserve( buf, ++buf->used );
        buf->ptr.cell[ si->it++ ] = *val;
        if( part > 1 )
            ur_arrErase( buf, si->it, part - 1 );
    }
    return UR_OK;
}


void block_remove( UThread* ut, USeriesIterM* si, UIndex part )
{
    (void) ut;
    ur_arrErase( si->buf, si->it, (part > 0) ? part : 1 );
}


int block_find( UThread* ut, const USeriesIter* si, const UCell* val, int opt )
{
    UBlockIter bi;
    const UBuffer* buf = si->buf;

    bi.it  = buf->ptr.cell + si->it;
    bi.end = buf->ptr.cell + si->end;

    if( opt & UR_FIND_LAST )
    {
        while( bi.it != bi.end )
        {
            --bi.end;
            if( ur_equal( ut, val, bi.end ) )
                return bi.end - buf->ptr.cell;
        }
    }
    else
    {
        ur_foreach( bi )
        {
            if( ur_equal( ut, val, bi.it ) )
                return bi.it - buf->ptr.cell;
        }
    }
    return -1;
}


int block_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    const UBuffer* buf = ur_bufferSer(cell);

    if( ur_is(bi->it, UT_INT) )
    {
        block_pick( buf, cell->series.it + ur_int(bi->it) - 1, res );
        ++bi->it;
        return UR_OK;
    }
    else if( ur_is(bi->it, UT_WORD) )
    {
        UBlockIter wi;
        UAtom atom = ur_atom(bi->it);
        ur_blkSlice( ut, &wi, cell );
        ur_foreach( wi )
        {
            // Checking atom first would be faster (it will fail more often
            // and is a quicker check), but the atom field may not be
            // intialized memory so memory checkers will report an error.
            if( ur_isWordType( ur_type(wi.it) ) && (ur_atom(wi.it) == atom) )
            {
                if( ++wi.it != wi.end )
                    *res = *wi.it;
                ++bi->it;
                return UR_OK;
            }
        }
    }
    return ur_error( ut, UR_ERR_SCRIPT, "block select expected int!/word!" );
}


USeriesType dt_block =
{
    {
    "block!",
    block_make,             block_convert,          block_copy,
    block_compare,          block_select,
    block_toString,         block_toText,
    unset_recycle,          block_mark,             array_destroy,
    block_markBuf,          block_toShared,         unset_bind
    },
    block_pick,             block_poke,             block_append,
    block_change,           block_remove,           block_find
};


//----------------------------------------------------------------------------
// UT_PAREN


int paren_make( UThread* ut, const UCell* from, UCell* res )
{
    int ok = block_make( ut, from, res );
    if( ok )
        ur_type(res) = UT_PAREN;
    return ok;
}


int paren_convert( UThread* ut, const UCell* from, UCell* res )
{
    int ok = block_convert( ut, from, res );
    if( ok )
        ur_type(res) = UT_PAREN;
    return ok;
}


USeriesType dt_paren =
{
    {
    "paren!",
    paren_make,             paren_convert,          block_copy,
    block_compare,          block_select,
    block_toString,         block_toString,
    unset_recycle,          block_mark,             array_destroy,
    block_markBuf,          block_toShared,         unset_bind
    },
    block_pick,             block_poke,             block_append,
    block_change,           block_remove,           block_find
};


//----------------------------------------------------------------------------
// UT_PATH


int path_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_BLOCK) )
    {
        UBlockIter bi;
        UBuffer* blk;

        ur_blkSlice( ut, &bi, from );
        // Make invalidates bi.buf.
        blk = ur_makeBlockCell( ut, UT_PATH, bi.end - bi.it, res );

        ur_foreach( bi )
        {
            if( ur_is(bi.it, UT_WORD) || ur_is(bi.it, UT_INT) )
                ur_blkPush( blk, bi.it );
        }
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE, "make path! expected block!" );
}


void path_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    UBlockIter bi;
    const UCell* start;

    ur_blkSlice( ut, &bi, cell );
    start = bi.it;

    if( ur_is(cell, UT_LITPATH) )
        ur_strAppendChar( str, '\'' );

    ur_foreach( bi )
    {
        if( bi.it != start )
            ur_strAppendChar( str, '/' );
        ur_toStr( ut, bi.it, str, depth );
    }

    if( ur_is(cell, UT_SETPATH) )
        ur_strAppendChar( str, ':' );
}


USeriesType dt_path =
{
    {
    "path!",
    path_make,              path_make,              block_copy,
    block_compare,          block_select,
    path_toString,          path_toString,
    unset_recycle,          block_mark,             array_destroy,
    block_markBuf,          block_toShared,         unset_bind
    },
    block_pick,             block_poke,             block_append,
    block_change,           block_remove,           block_find
};


//----------------------------------------------------------------------------
// UT_LITPATH


USeriesType dt_litpath =
{
    {
    "lit-path!",
    path_make,              path_make,              block_copy,
    block_compare,          block_select,
    path_toString,          path_toString,
    unset_recycle,          block_mark,             array_destroy,
    block_markBuf,          block_toShared,         unset_bind
    },
    block_pick,             block_poke,             block_append,
    block_change,           block_remove,           block_find
};


//----------------------------------------------------------------------------
// UT_SETPATH


USeriesType dt_setpath =
{
    {
    "set-path!",
    path_make,              path_make,              block_copy,
    block_compare,          block_select,
    path_toString,          path_toString,
    unset_recycle,          block_mark,             array_destroy,
    block_markBuf,          block_toShared,         unset_bind
    },
    block_pick,             block_poke,             block_append,
    block_change,           block_remove,           block_find
};


//----------------------------------------------------------------------------
// UT_CONTEXT


int context_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_BLOCK) )
    {
        UBlockIterM bi;
        UBuffer* ctx;

        if( ! ur_blkSliceM( ut, &bi, from ) )
            return UR_THROW;

        ctx = ur_makeContextCell( ut, 0, res );
        ur_ctxSetWords( ctx, bi.it, bi.end );
        ur_ctxSort( ctx );
        ur_bind( ut, bi.buf, ctx, UR_BIND_THREAD );
        return UR_OK;
    }
    else if( ur_is(from, UT_CONTEXT) )
    {
        ur_ctxClone( ut, ur_bufferSer(from), res );
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE, "make context! expected block!/context!");
}


void context_copy( UThread* ut, const UCell* from, UCell* res )
{
    ur_ctxClone( ut, ur_bufferSer(from), res );
}


extern void ur_ctxWordAtoms( const UBuffer* ctx, UAtom* atoms );

int context_select( UThread* ut, const UCell* cell, UBlockIter* bi, UCell* res )
{
    const UBuffer* ctx;

    if( ! (ctx = ur_sortedContext( ut, cell )) )
        return UR_THROW;

    if( ur_is(bi->it, UT_WORD) )
    {
        int i = ur_ctxLookup( ctx, ur_atom(bi->it) );
        if( i < 0 )
            return ur_error( ut, UR_ERR_SCRIPT, "context has no word '%s",
                             ur_atomCStr(ut, ur_atom(bi->it)) );
        *res = *ur_ctxCell(ctx, i);
        ++bi->it;
        return UR_OK;
    }
    else if( ur_is(bi->it, UT_LITWORD) )
    {
        if( ur_atom(bi->it) == UR_ATOM_WORDS )
        {
            UBlockIterM di;
            UAtom* ait;
            UAtom* atoms;
            int bindType;
            UIndex ctxN = cell->series.buf;
            UIndex used = ctx->used;

            di.buf = ur_makeBlockCell( ut, UT_BLOCK, used, res );
            di.it  = di.buf->ptr.cell;
            di.end = di.it + used;

            ctx = ur_bufferSer(cell);   // Re-aquire.
            atoms = ait = ((UAtom*) di.end) - used;
            ur_ctxWordAtoms( ctx, atoms );

            bindType = ur_isShared(ctxN) ? UR_BIND_ENV : UR_BIND_THREAD;
            ur_foreach( di )
            {
                ur_setId(di.it, UT_WORD);
                ur_setBinding( di.it, bindType );
                di.it->word.ctx   = ctxN;
                di.it->word.index = ait - atoms;
                di.it->word.atom  = *ait++;
            }

            di.buf->used = used;
            ++bi->it;
            return UR_OK;
        }
    }
    return ur_error( ut, UR_ERR_SCRIPT,
                     "context select expected word!/lit-word!" );
}


void context_toText( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
#define ASTACK_SIZE 8
    union {
        UAtom* heap;
        UAtom  stack[ ASTACK_SIZE ];
    } atoms;
    UAtom* ait;
    int alloced;
    const UBuffer* buf = ur_bufferSer(cell);
    const UCell* it  = buf->ptr.cell;
    const UCell* end = it + buf->used;

    // Get word atoms in order.
    if( buf->used > ASTACK_SIZE )
    {
        alloced = 1;
        atoms.heap = ait = (UAtom*) memAlloc( sizeof(UAtom) * buf->used );
        ur_ctxWordAtoms( buf, atoms.heap );
    }
    else
    {
        alloced = 0;
        ur_ctxWordAtoms( buf, ait = atoms.stack );
    }

    while( it != end )
    {
        ur_strAppendIndent( str, depth );
        ur_strAppendCStr( str, ur_atomCStr( ut, *ait++ ) );
        ur_strAppendCStr( str, ": " );
        DT( ur_type(it) )->toString( ut, it, str, depth );
        ur_strAppendChar( str, '\n' );
        ++it;
    }

    if( alloced )
        memFree( atoms.heap );
}


void context_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    ur_strAppendCStr( str, "context [\n" );
    context_toText( ut, cell, str, depth + 1 );
    ur_strAppendIndent( str, depth );
    ur_strAppendCStr( str, "]" );
}


void context_destroy( UBuffer* buf )
{
    ur_ctxFree( buf );
}


UDatatype dt_context =
{
    "context!",
    context_make,           context_make,           context_copy,
    unset_compare,          context_select,
    context_toString,       context_toText,
    unset_recycle,          block_mark,             context_destroy,
    context_markBuf,        block_toShared,         unset_bind
};


//----------------------------------------------------------------------------
// UT_ERROR


int error_make( UThread* ut, const UCell* from, UCell* res )
{
    if( ur_is(from, UT_STRING) )
    {
        ur_setId(res, UT_ERROR);
        res->error.exType     = UR_ERR_SCRIPT;
        res->error.messageStr = from->series.buf;
        res->error.traceBlk   = UR_INVALID_BUF;
        return UR_OK;
    }
    return ur_error( ut, UR_ERR_TYPE, "make error! expected string! message" );
}


static const char* errorTypeStr[] =
{
    "Datatype",
    "Script",
    "Syntax",
    "Access",
    "Internal"
};


static void _lineToString( UThread* ut, const UCell* bc, UBuffer* str )
{
    UBlockIter bi;
    const UCell* start;
    UIndex fstart;


    // Specialized version of ur_blkSlice() to get pointers even if 
    // bi.it is at bi.end.  Changing ur_blkSlice to do this would slow it
    // down with extra checks to validate that series.it < buf->used.

    bi.buf = ur_bufferSer(bc);
    if( ! bi.buf->ptr.cell || ! bi.buf->used )
        return;
    {
        UIndex end = bi.buf->used;
        if( (bc->series.end > -1) && (bc->series.end < end) )
            end = bc->series.end;
        if( end < bc->series.it )
            end = bc->series.it;
        bi.it  = bi.buf->ptr.cell + bc->series.it;
        bi.end = bi.buf->ptr.cell + end;
    }
    start = bi.it;
    if( bi.it == bi.end )
        --start;

    // Set end to newline after bc->series.it.
    if( bi.it != bi.end )
    {
        ++bi.it;
        ur_foreach( bi )
        {
            if( ur_flags(bi.it, UR_FLAG_SOL) )
                break;
        }
        bi.end = bi.it;
    }

    // Set start to newline at or before bc->series.it.
    while( start != bi.buf->ptr.cell )
    {
        if( ur_flags(start, UR_FLAG_SOL) )
            break;
        --start;
    }
    bi.it = start;

    // Convert to string without any open/close braces.
    ur_foreach( bi )
    {
        if( bi.it != start )
            ur_strAppendChar( str, ' ' );
        fstart = str->used;
        ur_toStr( ut, bi.it, str, 0 );
        if( ur_is(bi.it, UT_BLOCK) || ur_is(bi.it, UT_PAREN) )
        {
            fstart = ur_strFindChar( str, fstart, str->used, '\n' );
            if( fstart > -1 )
                str->used = fstart;
        }
    }
}


void error_toString( UThread* ut, const UCell* cell, UBuffer* str, int depth )
{
    uint16_t et = cell->error.exType;
    const UBuffer* msg;
    (void) depth;

    if( et < UR_ERR_COUNT )
    {
        ur_strAppendCStr( str, errorTypeStr[ et ] );
        ur_strAppendCStr( str, " Error: " );
    }
    else
    {
        ur_strAppendCStr( str, "Error " );
        ur_strAppendInt( str, et );
        ur_strAppendCStr( str, ": " );
    }

    msg = ur_buffer( cell->error.messageStr );
    ur_strAppend( str, msg, 0, msg->used );

    if( cell->error.traceBlk > UR_INVALID_BUF )
    {
        UBlockIter bi;

        bi.buf = ur_buffer( cell->error.traceBlk );
        bi.it  = bi.buf->ptr.cell;
        bi.end = bi.it + bi.buf->used;

        if( bi.buf->used )
        {
            ur_strAppendCStr( str, "\nTrace:" );
            ur_foreach( bi )
            {
                ur_strAppendCStr( str, "\n -> " );
                _lineToString( ut, bi.it, str );
            }
        }
    }
}


void error_mark( UThread* ut, UCell* cell )
{
    UIndex n;
   
    ur_markBuffer( ut, cell->error.messageStr );

    n = cell->error.traceBlk;
    if( n > UR_INVALID_BUF )
    {
        if( ur_markBuffer( ut, n ) )
            block_markBuf( ut, ur_buffer(n) );
    }
}


void error_toShared( UCell* cell )
{
    UIndex n;
    n = cell->error.messageStr;
    if( n > UR_INVALID_BUF )
        cell->error.messageStr = -n;
    n = cell->error.traceBlk;
    if( n > UR_INVALID_BUF )
        cell->error.traceBlk = -n;
}


UDatatype dt_error =
{
    "error!",
    error_make,             error_make,             unset_copy,
    unset_compare,          unset_select,
    error_toString,         error_toString,
    unset_recycle,          error_mark,             unset_destroy,
    unset_markBuf,          error_toShared,         unset_bind
};


//EOF
