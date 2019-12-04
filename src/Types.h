
#ifndef __XC_TYPES_H__
#define __XC_TYPES_H__

//////////////////////////////////////////////////////////////////////////
// base types definition
// compilers should be restricted to ensure the following equalities!
typedef signed char             schar;    ///< sizeof(uchar) == sizeof(schar) == sizeof(char) == 1
typedef unsigned char           uchar;    
typedef unsigned int            uint;    ///< sizeof(uint) == sizeof(int) == 4
typedef unsigned short          ushort;    ///< sizeof(ushort) == sizeof(short) == 2
typedef unsigned long           ulong;    ///< sizeof(ulong) == sizeof(long) == 4
#ifdef WIN32
typedef __int64                 int64;    ///< sizeof(int64) == sizeof(uint64) == 8
typedef unsigned __int64        uint64;
#elif defined(__GNUC__)
#ifndef int64
typedef long long               int64;
#endif
#ifndef uint64
typedef unsigned long long      uint64;
#endif
#elif defined(__TCS__)
typedef signed   long long int  int64;
typedef unsigned long long int  uint64;
#endif

typedef char                   VD_INT8;
typedef unsigned char          VD_UINT8;   
typedef short                  VD_INT16;
typedef unsigned short         VD_UINT16;
typedef int                    VD_INT32;
typedef unsigned int           VD_UINT32;

typedef void *                 VD_HANDLE;
typedef int                    VD_BOOL;
typedef void                   VD_VOID;

#define VD_NULL_LONG           0xFFFFFFFF
#define VD_TRUE                1
#define VD_FALSE               0

#define __trip printf("-W-::%s(%d)", __FILE__, __LINE__);

#endif// __DH_TYPES_H__

