/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header: /upc/share/CVS/netcdf-3/libncdap3/dapcvt.c,v 1.4 2009/03/25 01:48:27 dmh Exp $
 *********************************************************************/
#include "config.h"
#include "ncdap3.h"
#include "dapodom.h"
#include "getvar3.h"

NCerror
ncdap3convert(nc_type srctype, nc_type dsttype, char* memory, char* value)
{
    NCerror ncstat = NC_NOERR;

#define CASE(nc1,nc2) (nc1*256+nc2)
	switch (CASE(srctype,dsttype)) {

case CASE(NC_CHAR,NC_CHAR):
    *((char*)memory) = (char) *(char*)value;
    break;
case CASE(NC_CHAR,NC_BYTE):
    *((char*)memory) = (char) *(char*)value;
    break;
case CASE(NC_CHAR,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(char*)value;
    break;
case CASE(NC_CHAR,NC_SHORT):
    *((short*)memory) = (short) *(char*)value;
    break;
case CASE(NC_CHAR,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(char*)value;
    break;
case CASE(NC_CHAR,NC_INT):
    *((int*)memory) = (int) *(char*)value;
    break;
case CASE(NC_CHAR,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(char*)value;
    break;
case CASE(NC_CHAR,NC_INT64):
    *((long long*)memory) = (long long) *(char*)value;
    break;
case CASE(NC_CHAR,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(char*)value;
    break;
case CASE(NC_CHAR,NC_FLOAT):
    *((float*)memory) = (float) *(char*)value;
    break;
case CASE(NC_CHAR,NC_DOUBLE):
    *((double*)memory) = (double) *(char*)value;
    break;

case CASE(NC_BYTE,NC_CHAR):
    *((char*)memory) = (char) *(char*)value;
    break;
case CASE(NC_BYTE,NC_BYTE):
    *((char*)memory) = (char) *(char*)value;
    break;
case CASE(NC_BYTE,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(char*)value;
    break;
case CASE(NC_BYTE,NC_SHORT):
    *((short*)memory) = (short) *(char*)value;
    break;
case CASE(NC_BYTE,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(char*)value;
    break;
case CASE(NC_BYTE,NC_INT):
    *((int*)memory) = (int) *(char*)value;
    break;
case CASE(NC_BYTE,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(char*)value;
    break;
case CASE(NC_BYTE,NC_INT64):
    *((long long*)memory) = (long long) *(char*)value;
    break;
case CASE(NC_BYTE,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(char*)value;
    break;
case CASE(NC_BYTE,NC_FLOAT):
    *((float*)memory) = (float) *(char*)value;
    break;
case CASE(NC_BYTE,NC_DOUBLE):
    *((double*)memory) = (double) *(char*)value;
    break;

case CASE(NC_SHORT,NC_CHAR):
    *((char*)memory) = (char) *(short*)value;
    break;
case CASE(NC_SHORT,NC_BYTE):
    *((char*)memory) = (char) *(short*)value;
    break;
case CASE(NC_SHORT,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(short*)value;
    break;
case CASE(NC_SHORT,NC_SHORT):
    *((short*)memory) = (short) *(short*)value;
    break;
case CASE(NC_SHORT,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(short*)value;
    break;
case CASE(NC_SHORT,NC_INT):
    *((int*)memory) = (int) *(short*)value;
    break;
case CASE(NC_SHORT,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(short*)value;
    break;
case CASE(NC_SHORT,NC_INT64):
    *((long long*)memory) = (long long) *(short*)value;
    break;
case CASE(NC_SHORT,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(short*)value;
    break;
case CASE(NC_SHORT,NC_FLOAT):
    *((float*)memory) = (float) *(short*)value;
    break;
case CASE(NC_SHORT,NC_DOUBLE):
    *((double*)memory) = (double) *(short*)value;
    break;

case CASE(NC_USHORT,NC_CHAR):
    *((char*)memory) = (char) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_BYTE):
    *((char*)memory) = (char) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_SHORT):
    *((short*)memory) = (short) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_INT):
    *((int*)memory) = (int) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_INT64):
    *((long long*)memory) = (long long) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_FLOAT):
    *((float*)memory) = (float) *(unsigned short*)value;
    break;
case CASE(NC_USHORT,NC_DOUBLE):
    *((double*)memory) = (double) *(unsigned short*)value;
    break;

case CASE(NC_INT,NC_CHAR):
    *((char*)memory) = (char) *(int*)value;
    break;
case CASE(NC_INT,NC_BYTE):
    *((char*)memory) = (char) *(int*)value;
    break;
case CASE(NC_INT,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(int*)value;
    break;
case CASE(NC_INT,NC_SHORT):
    *((short*)memory) = (short) *(int*)value;
    break;
case CASE(NC_INT,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(int*)value;
    break;
case CASE(NC_INT,NC_INT):
    *((int*)memory) = (int) *(int*)value;
    break;
case CASE(NC_INT,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(int*)value;
    break;
case CASE(NC_INT,NC_INT64):
    *((long long*)memory) = (long long) *(int*)value;
    break;
case CASE(NC_INT,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(int*)value;
    break;
case CASE(NC_INT,NC_FLOAT):
    *((float*)memory) = (float) *(int*)value;
    break;
case CASE(NC_INT,NC_DOUBLE):
    *((double*)memory) = (double) *(int*)value;
    break;

case CASE(NC_UINT,NC_CHAR):
    *((char*)memory) = (char) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_BYTE):
    *((char*)memory) = (char) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_SHORT):
    *((short*)memory) = (short) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_INT):
    *((int*)memory) = (int) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_INT64):
    *((long long*)memory) = (long long) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_FLOAT):
    *((float*)memory) = (float) *(unsigned int*)value;
    break;
case CASE(NC_UINT,NC_DOUBLE):
    *((double*)memory) = (double) *(unsigned int*)value;
    break;


case CASE(NC_INT64,NC_CHAR):
    *((char*)memory) = (char) *(long long*)value;
    break;
case CASE(NC_INT64,NC_BYTE):
    *((char*)memory) = (char) *(long long*)value;
    break;
case CASE(NC_INT64,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(long long*)value;
    break;
case CASE(NC_INT64,NC_SHORT):
    *((short*)memory) = (short) *(long long*)value;
    break;
case CASE(NC_INT64,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(long long*)value;
    break;
case CASE(NC_INT64,NC_INT):
    *((int*)memory) = (int) *(long long*)value;
    break;
case CASE(NC_INT64,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(long long*)value;
    break;
case CASE(NC_INT64,NC_INT64):
    *((long long*)memory) = (long long) *(long long*)value;
    break;
case CASE(NC_INT64,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(long long*)value;
    break;
case CASE(NC_INT64,NC_FLOAT):
    *((float*)memory) = (float) *(long long*)value;
    break;
case CASE(NC_INT64,NC_DOUBLE):
    *((double*)memory) = (double) *(long long*)value;
    break;

case CASE(NC_UINT64,NC_CHAR):
    *((char*)memory) = (char) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_BYTE):
    *((char*)memory) = (char) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_SHORT):
    *((short*)memory) = (short) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_INT):
    *((int*)memory) = (int) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_INT64):
    *((long long*)memory) = (long long) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_FLOAT):
    *((float*)memory) = (float) *(unsigned long long*)value;
    break;
case CASE(NC_UINT64,NC_DOUBLE):
    *((double*)memory) = (double) *(unsigned long long*)value;
    break;


case CASE(NC_FLOAT,NC_CHAR):
    *((char*)memory) = (char) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_BYTE):
    *((char*)memory) = (char) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_SHORT):
    *((short*)memory) = (short) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_INT):
    *((int*)memory) = (int) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_INT64):
    *((long long*)memory) = (long long) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_FLOAT):
    *((float*)memory) = (float) *(float*)value;
    break;
case CASE(NC_FLOAT,NC_DOUBLE):
    *((double*)memory) = (double) *(float*)value;
    break;

case CASE(NC_DOUBLE,NC_CHAR):
    *((char*)memory) = (char) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_BYTE):
    *((char*)memory) = (char) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_UBYTE):
    *((unsigned char*)memory) = (unsigned char) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_SHORT):
    *((short*)memory) = (short) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_USHORT):
    *((unsigned short*)memory) = (unsigned short) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_INT):
    *((int*)memory) = (int) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_UINT):
    *((unsigned int*)memory) = (unsigned int) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_INT64):
    *((long long*)memory) = (long long) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_UINT64):
    *((unsigned long long*)memory) = (unsigned long long) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_FLOAT):
    *((float*)memory) = (float) *(double*)value;
    break;
case CASE(NC_DOUBLE,NC_DOUBLE):
    *((double*)memory) = (double) *(double*)value;
    break;

    default: ncstat = NC_EINVAL; THROW(ncstat); goto fail;

    }

    memory += nctypesizeof(dsttype);
fail:
    return THROW(ncstat);
}
