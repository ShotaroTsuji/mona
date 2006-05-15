/*!
  \file   MlcLimits.h
  \brief  mona c limits

  Copyright (c) 2002-2004 shadow
  All rights reserved.<BR>
  \b License NYSL<BR>
  \b Create 2004/02/29
  \author  shadow

  $Revision$
  $Date$
*/
#ifndef _MONA_LIB_LIMITS_
#define _MONA_LIB_LIMITS_

#ifdef __cplusplus
extern "C" {
#endif

#define CHAR_BIT 8
#define SCHAR_MIN -127
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define CHAR_MIN M_SCHAR_MIN
#define CHAR_MAX M_UCHAR_MAX

#define INT_MIN -2147483647
#define INT_MAX 2147483647
#define UINT_MAX 4294967295U

#define LONG_MIN -2147483647
#define LONG_MAX 2147483647
#define ULONG_MAX 4294967295U

#ifdef __cplusplus
}
#endif

#endif