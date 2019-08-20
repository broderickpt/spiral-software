/****************************************************************************
**
*W  system4.c                    GAP source                      Frank Celler
*W                                                         & Martin Schoenert
*W                                                         & Dave Bayer (MAC)
*W                                                  & Harald Boegeholz (OS/2)
*W                                                         & Paul Doyle (VMS)
*W                                                  & Burkhard Hoefling (MAC)
*W                                                    & Steve Linton (MS/DOS)
**
**
*Y  Copyright (C) 2018-2019, Carnegie Mellon University
*Y  All rights reserved.  See LICENSE for details.
*Y  
*Y  This work is based on GAP version 3, with some files from version 4.  GAP is
*Y  Copyright (C) (1987--2019) by the GAP Group (www.gap-system.org).
**
**  The  files   "system.c" and  "sysfiles.c"  contains all  operating system
**  dependent  functions.  This file contains  all system dependent functions
**  except file and stream operations, which are implemented in "sysfiles.c".
**  The following labels determine which operating system is actually used.
**
**  Under UNIX autoconf  is used to check  various features of  the operating
**  system and the compiler.  Should you have problem compiling GAP check the
**  file "bin/CPU-VENDOR-OS/config.h" after you have done a
**
**     ./configure ; make config
**
**  in the root directory.  And then do a
**
**     make compile
**
**  to compile and link GAP.
*/

const char * GAP4_Revision_system_c =
   "@(#)Id: system.c,v 4.96 2002/05/04 13:45:53 gap Exp";

#ifndef SYS_STDIO_H                     /* standard input/output functions */
# include <stdio.h>
# define SYS_STDIO_H
#endif

#ifndef WIN32
#ifndef SYS_UNISTD_H                    /* sbrk,read,write                 */
# include <unistd.h>
# define SYS_UNISTD_H 
#endif
#endif

#include "system4.h"

#ifdef WIN32
#include <windows.h>
#endif


#ifndef SYS_STDLIB_H                    /* ANSI standard functions         */
# if SYS_ANSI
#  include      <stdlib.h>
# endif
# define SYS_STDLIB_H
#endif


#ifdef __MWERKS__
# if !SYS_MAC_MWC /* BH:__MWERKS__ is also true for  SYS_MAC_MWC */
#  define SYS_IS_MAC_MPW             1
#  define SYS_HAS_CALLOC_PROTO       1
# endif
#endif

#if SYS_MAC_MWC
# include "macdefs.h"
# include "macpaths.h"
# include "macte.h"
# include "macedit.h"
# include "maccon.h"
# include "macpaths.h"
# include "macintr.h"
#endif

#if SYS_DARWIN
#define task_self mach_task_self
#endif


/****************************************************************************
**

*F * * * * * * * * * * * command line settable options  * * * * * * * * * * *
*/

/****************************************************************************
**

*V  SyStackAlign  . . . . . . . . . . . . . . . . . .  alignment of the stack
**
**  'SyStackAlign' is  the  alignment  of items on the stack.   It  must be a
**  divisor of  'sizof(Bag)'.  The  addresses of all identifiers on the stack
**  must be  divisable by 'SyStackAlign'.  So if it  is 1, identifiers may be
**  anywhere on the stack, and if it is  'sizeof(Bag)',  identifiers may only
**  be  at addresses  divisible by  'sizeof(Bag)'.  This value is initialized
**  from a macro passed from the makefile, because it is machine dependent.
**
**  This value is passed to 'InitBags'.
*/
UInt SyStackAlign = SYS_STACK_ALIGN;

/****************************************************************************
**
*V  SyCacheSize . . . . . . . . . . . . . . . . . . . . . . size of the cache
**
**  'SyCacheSize' is the size of the data cache.
**
**  This is per  default 0, which means that  there is no usuable data cache.
**  It is usually changed with the '-c' option in the script that starts GAP.
**
**  This value is passed to 'InitBags'.
**
**  Put in this package because the command line processing takes place here.
*/
UInt SyCacheSize = NUM_TO_UINT(0);

/****************************************************************************
**
*V  SyMsgsFlagBags  . . . . . . . . . . . . . . . . .  enable gasman messages
**
**  'SyMsgsFlagBags' determines whether garabage collections are reported  or
**  not.
**
**  Per default it is false, i.e. Gasman is silent about garbage collections.
**  It can be changed by using the  '-g'  option  on the  GAP  command  line.
**
**  This is used in the function 'SyMsgsBags' below.
**
**  Put in this package because the command line processing takes place here.
*/
UInt SyMsgsFlagBags = NUM_TO_UINT(0);


/****************************************************************************
**
*V  SyStorMax . . . . . . . . . . . . . . . . . . . maximal size of workspace
**
**  'SyStorMax' is the maximal size of the workspace allocated by Gasman.
**  in kilobytes
**
**  This is per default 256 MByte,  which is often a  reasonable value.  It is
**  usually changed with the '-o' option in the script that starts GAP.
**
**  This is used in the function 'SyAllocBags'below.
**
**  Put in this package because the command line processing takes place here.
*/
Int SyStorMax = 256 * NUM_TO_INT(1024);
Int SyStorOverrun = 0;

/****************************************************************************
**
*V  SyStorKill . . . . . . . . . . . . . . . . . . maximal size of workspace
**
**  'SyStorKill' is really the maximal size of the workspace allocated by 
**  Gasman. GAP exists before trying to allocate more than this amount
**  of memory.
**
**  This is per default disabled (i.e. = 0).
**  Can be changed with the '-K' option in the script that starts GAP.
**
**  This is used in the function 'SyAllocBags'below.
**
**  Put in this package because the command line processing takes place here.
*/
Int SyStorKill = NUM_TO_INT(0);


/****************************************************************************
**
*V  SyStorMin . . . . . . . . . . . . . .  default size for initial workspace
**
**  'SyStorMin' is the size of the initial workspace allocated by Gasman.
**
**  This is per default  24 Megabyte,  which  is  often  a  reasonable  value.
**  It is usually changed with the '-m' option in the script that starts GAP.
**
**  This value is used in the function 'SyAllocBags' below.
**
**  Put in this package because the command line processing takes place here.
*/
Int SyStorMin = SY_STOR_MIN;


/****************************************************************************
**
*V  syStackSpace  . . . . . . . . . . . . . . . . . . . amount of stack space
**
**  'syStackSpace' is the amount of stackspace that GAP gets.
**
**  Under TOS and on the  Mac special actions must  be  taken to ensure  that
**  enough space is available.
*/
#if SYS_TOS_GCC2
# define __NO_INLINE__
int _stksize = 64 * 1024;   /* GNU C, amount of stack space    */
static UInt syStackSpace = 64 * 1024;
#endif

#if SYS_MAC_MPW || SYS_MAC_MWC 
static UInt syStackSpace = NUM_TO_UINT(2) * NUM_TO_UINT(1024) * NUM_TO_UINT(1024);
#endif

#if SYS_MAC_MWC	
char * SyMinStack = (char*) NUM_TO_INT(-1);
#endif

/****************************************************************************
**

*F * * * * * * * * * * * * * * gasman interface * * * * * * * * * * * * * * *
*/


/****************************************************************************
**
*F  SyMsgsBags( <full>, <phase>, <nr> ) . . . . . . . display Gasman messages
**
**  'SyMsgsBags' is the function that is used by Gasman to  display  messages
**  during garbage collections.
*/
#if SYS_MAC_MWC
UInt syLastFreeWorkspace = 0;  
	/* amout of free workspace during last collection */
#endif

void SyMsgsBags (
    UInt                full,
    UInt                phase,
    Int                 nr )
{
    Char                cmd [3];        /* command string buffer           */
    Char                str [32];       /* string buffer                   */
    Char                ch;             /* leading character               */
    UInt                i;              /* loop variable                   */
    Int                 copynr;         /* copy of <nr>                    */

    /* convert <nr> into a string with leading blanks                      */
    copynr = nr;
    ch = '0';  str[7] = '\0';
    for ( i = 7; i != 0; i-- ) {
        if      ( 0 < nr ) { str[i-1] = '0' + ( nr) % 10;  ch = ' '; }
        else if ( nr < 0 ) { str[i-1] = '0' + (-nr) % 10;  ch = '-'; }
        else               { str[i-1] = ch;                ch = ' '; }
        nr = nr / 10;
    }
    nr = copynr;

#if SYS_MAC_MWC
 	if (phase == 5)
 		syLastFreeWorkspace = nr; /* save for status message in about box */
#endif

    /* ordinary full garbage collection messages                           */
    if ( 1 <= SyMsgsFlagBags && full ) {
        if ( phase == 0 ) { SyFputs( "#G  FULL ", 3 );                     }
        if ( phase == 1 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 2 ) { SyFputs( str, 3 );  SyFputs( "kb live  ", 3 ); }
        if ( phase == 3 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 4 ) { SyFputs( str, 3 );  SyFputs( "kb dead  ", 3 ); }
        if ( phase == 5 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 6 ) { SyFputs( str, 3 );  SyFputs( "kb free\n", 3 ); }
    }

    /* ordinary partial garbage collection messages                        */
    if ( 2 <= SyMsgsFlagBags && ! full ) {
        if ( phase == 0 ) { SyFputs( "#G  PART ", 3 );                     }
        if ( phase == 1 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 2 ) { SyFputs( str, 3 );  SyFputs( "kb+live  ", 3 ); }
        if ( phase == 3 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 4 ) { SyFputs( str, 3 );  SyFputs( "kb+dead  ", 3 ); }
        if ( phase == 5 ) { SyFputs( str, 3 );  SyFputs( "/",         3 ); }
        if ( phase == 6 ) { SyFputs( str, 3 );  SyFputs( "kb free\n", 3 ); }
    }
#if !SYS_MAC_MWC
    /* package (window) mode full garbage collection messages              */
    if ( phase != 0 ) {
        if ( 3 <= phase ) nr *= 1024;
        cmd[0] = '@';
        cmd[1] = ( full ? '0' : ' ' ) + phase;
        cmd[2] = '\0';
        i = 0;
        for ( ; 0 < nr; nr /=10 )
            str[i++] = '0' + (nr % 10);
        str[i++] = '+';
        str[i++] = '\0';
        syWinPut( 1, cmd, str );
    }
#endif
}


/****************************************************************************
**
*F  SyAllocBags( <size>, <need> ) . . . allocate memory block of <size> kilobytes
**
**  'SyAllocBags' is called from Gasman to get new storage from the operating
**  system.  <size> is the needed amount in kilobytes (it is always a multiple of
**  512 KByte),  and <need> tells 'SyAllocBags' whether  Gasman  really needs
**  the storage or only wants it to have a reasonable amount of free storage.
**
**  Currently  Gasman  expects this function to return  immediately  adjacent
**  areas on subsequent calls.  So 'sbrk' will  work  on  most  systems,  but
**  'malloc' will not.
**
**  If <need> is 0, 'SyAllocBags' must return 0 if it cannot or does not want
**  to extend the workspace,  and a pointer to the allocated area to indicate
**  success.   If <need> is 1  and 'SyAllocBags' cannot extend the workspace,
**  'SyAllocBags' must abort,  because GAP assumes that  'NewBag'  will never
**  fail.
**
**  <size> may also be negative in which case 'SyAllocBags' should return the
**  storage to the operating system.  In this case  <need>  will always be 0.
**  'SyAllocBags' can either accept this reduction and  return 1  and  return
**  the storage to the operating system or refuse the reduction and return 0.
**
**  If the operating system does not support dynamic memory managment, simply
**  give 'SyAllocBags' a static buffer, from where it returns the blocks.
*/



#if WIN32
#define _MAX(a,b) (a) > (b) ? (a) : (b) 

UInt * * * syWorkspace = 0;

#ifdef WIN32_USE_MALLOC

UInt * * * SyAllocBags (
    Int                 size,
    UInt                need )
{
	unsigned *p;
	
	size *= 1024;
		
    if (need ==1)
	{
		if ( syWorkspace == (UInt***)0 ) 
		{
			syWorkspace = (UInt***)malloc(size);
			for (p = syWorkspace; p < syWorkspace + size / sizeof(unsigned); p++)
				*p = 0;

			return syWorkspace;
		}
		else 
		{
			fputs("gap: cannot extend the workspace any more\n",stderr);
			SyExit( 1 );
		}
	}
	else
		return 0;
}
#else

LPTSTR lpNxtBlock;               // address of the next block to ask for
LPVOID lpvBase;                  // base address

#define WIN32_MAXMEM	2048
#define WIN32_DECREMENT	64
#define WIN32_MINMEM	64


UInt * * * SyAllocBags (
    Int                 size,
    UInt                need )
{
	static int init_vm = 0;
	BOOL bSuccess;                // flag
	LPVOID lpvResult = NULL;
	__int64 memsize = WIN32_MAXMEM;

	size *= 1024;					// we work using bytes

	if (!init_vm)
	{
		// Reserve pages in the process's virtual address space.
		do {
			lpvBase = VirtualAlloc(
								NULL,                 // system selects address
								memsize*1024*1024,		 // size of allocation
								MEM_RESERVE,          // allocate reserved pages
								PAGE_NOACCESS);       // protection = no access
			memsize -= WIN32_DECREMENT;
		} while ((!lpvBase) && (memsize > WIN32_MINMEM));
		// Memory allocation failed
		if (!lpvBase) 
		{
			fputs("gap: init WIN32 VirtualAlloc(MEM_RESERVE) failed\n",stderr);
			SyExit(1);
		}
		else
		{
			memsize += WIN32_DECREMENT;
			//fprintf(stdout, "gap: allocated %lld MB\n", memsize);
		}
	
		init_vm = 1;
		syWorkspace = lpvBase;

		lpvResult = VirtualAlloc(
								 lpvBase,			 // next page to commit
								 size,		         // what we want, in bytes
								 MEM_COMMIT,         // allocate a committed page
								 PAGE_READWRITE);    // read/write access

		if (!lpvBase) 
		{
			fputs("gap: init WIN32 VirtualAlloc(MEM_COMMIT) failed\n",stderr);
			SyExit(1);
		}

		lpNxtBlock = (LPSTR)lpvBase + size;
	}
	else
	{
		if (size > 0)
		{
			lpvResult = VirtualAlloc(
								 lpNxtBlock,		 // next page to commit
								 size,		         // what we want, in bytes
								 MEM_COMMIT,         // allocate a committed page
								 PAGE_READWRITE);    // read/write access

		if (!lpvResult) 
		{
			fputs("gap: expand WIN32 VirtualAlloc(MEM_COMMIT) failed\n",stderr);
			SyExit(1);
		}

			lpNxtBlock += size;
		}
		else
		{
			bSuccess = VirtualFree(lpvBase, size, MEM_DECOMMIT);
			if (!bSuccess)
			{
				fputs("gap: WIN32 VirtualFree() failed\n",stderr);
				SyExit(1);
			}

			lpNxtBlock -= size;
		}
	}
	return syWorkspace;
}

#endif

#endif

/****************************************************************************
**
*f  SyAllocBags( <size>, <need> ) . . . . . . . BSD/USG/OS2 EMX/MSDOS/TOS/VMS
**
**  For UNIX,  OS/2, MS-DOS, TOS,  and VMS, 'SyAllocBags' calls 'sbrk', which
**  will work on most systems.
**
**  Note that   it may  happen that  another   function   has  called  'sbrk'
**  between  two calls to  'SyAllocBags',  so that the  next  allocation will
**  not be immediately adjacent to the last one.   In this case 'SyAllocBags'
**  returns the area to the operating system,  and either returns 0 if <need>
**  was 0 or aborts GAP if <need> was 1.  'SyAllocBags' will refuse to extend
**  the workspace beyond 'SyStorMax' or to reduce it below 'SyStorMin'.
*/
#if SYS_BSD||SYS_USG||SYS_OS2_EMX||SYS_MSDOS_DJGPP||SYS_TOS_GCC2||SYS_VMS||HAVE_SBRK

UInt * * * syWorkspace = 0;
UInt       syWorksize;


UInt * * * SyAllocBags (
    Int                 size,			/* size requested in K bytes */
    UInt                need )
{
    UInt * * *          ret;
    UInt adjust = 0;

	// printf("SyAllocBags: Called for memory: size = %dk, need = %s\n", size, (need? "True" : "False"));
	/* just malloc() the block of memory and move on... */
	size *= 1024;						/* multiply to get bytes */
	ret = (UInt ***)malloc(size);
	if (ret == (UInt ***)NULL) {
		/* couldn't get the memory we need */
		fputs("gap: cannot extend the workspace any more\n",stderr);
        SyExit( 1 );
    }

	syWorkspace = (UInt***)ret;
	syWorksize += size;
		
	return syWorkspace;

#ifdef OLD_WAY_OF_DOING_IT
	
    /* force alignment on first call                                       */
    if ( syWorkspace == (UInt***)0 ) {
#ifdef SYS_IS_64_BIT
        syWorkspace = (UInt***)sbrk( 8 - (UInt)sbrk(0) % 8 );
#else
        syWorkspace = (UInt***)sbrk( 4 - (UInt)sbrk(0) % 4 );
#endif
        syWorkspace = (UInt***)sbrk( 0 );
    }

    /* get the storage, but only if we stay within the bounds              */
    /* if ( (0 < size && syWorksize + size <= SyStorMax) */
    /* first check if we would get above SyStorKill, if yes exit! */
    if ( SyStorKill != 0 && 0 < size && SyStorKill < syWorksize + size ) {
        fputs("gap: will not extend workspace above -K limit, bye!\n",stderr);
        SyExit( 2 );
    }
    if (0 < size )
      {
#ifndef SYS_IS_64_BIT
	while (size > 1024*1024)
	  {
	    ret = (UInt ***)sbrk(1024*1024*1024);
	    if (ret != (UInt ***)-1  && ret != (UInt***)((char*)syWorkspace + syWorksize*1024))
	      {
		sbrk(-1024*1024*1024);
		ret = (UInt ***)-1;
	      }
	    if (ret == (UInt ***)-1)
	      break;
	    size -= 1024*1024;
	    syWorksize += 1024*1024;
	    adjust++;
	  }
#endif
	ret = (UInt ***)sbrk(size*1024);
	if (ret != (UInt ***)-1  && ret != (UInt***)((char*)syWorkspace + syWorksize*1024))
	  {
	    sbrk(-size*1024);
	    ret = (UInt ***)-1;
	  }
	
      }
    else if  (size < 0 && SyStorMin <= syWorksize + size)  {
#ifndef SYS_IS_64_BIT
      while (size < -1024*1024)
	{
	  ret = (UInt ***)sbrk(-1024*1024*1024);
	  if (ret == (UInt ***)-1)
	    break;
	  size += 1024*1024;
	  syWorksize -= 1024*1024;
	}
#endif
	ret = (UInt ***)sbrk(size*1024);
    }
    else {
      ret = (UInt***)-1;
    }
    


    /* update the size info                                                */
    if ( ret != (UInt***)-1 ) {
        syWorksize += size;
       /* set the overrun flag if we became larger than SyStorMax */
       if ( syWorksize  > SyStorMax)  {
	 SyStorOverrun = -1;
	 SyStorMax=syWorksize*2; /* new maximum */
       }
    }

    /* test if the allocation failed                                       */
    if ( ret == (UInt***)-1 && need ) {
        fputs("gap: cannot extend the workspace any more\n",stderr);
        SyExit( 1 );
    }

    /* otherwise return the result (which could be 0 to indicate failure)  */
    if ( ret == (UInt***)-1 )
        return 0;
    else
        return (UInt***)(((Char *)ret) - 1024*1024*1024*adjust);

#endif							// OLD_WAY_OF_DOING_IT
	
}

#endif


/****************************************************************************
**
*f  SyAllocBags( <size>, <need> ) . . . . . . . . . . . . . . . . . . .  MACH
**
**  Under MACH virtual memory managment functions are used instead of 'sbrk'.
*/
#if SYS_MACH || HAVE_VM_ALLOCATE

#include <mach/mach.h>

vm_address_t syBase  = 0;
Int          sySize  = 0;

UInt * * * SyAllocBags (
    Int                 size,
    UInt                need )
{
    UInt * * *          ret;
    vm_address_t        adr;

    /* check that we stay within our bounds                                */
    if ( 0 < size && SyStorMax < sySize + size )
        ret = (UInt***) -1;
    else if ( size < 0 && sySize + size < SyStorMin )
        ret = (UInt***) -1;

    size = size*1024;
    /* check that <size> is divisible by <vm_page_size>                    */
    if ( size % vm_page_size != 0 ) {
        fputs( "gap: memory block size is not a multiple of vm_page_size",
               stderr );
        SyExit(1);
    }

    /* check that we don't try to shrink uninialized memory                */
    else if ( size <= 0 && syBase == 0 ) {
        fputs( "gap: trying to shrink uninialized vm memory\n", stderr );
        SyExit(1);
    }

    /* allocate memory anywhere on first call                              */
    else if ( 0 < size && syBase == 0 ) {
        if ( vm_allocate(task_self(),&syBase,size,TRUE) == KERN_SUCCESS ) {
            sySize = size;
            ret = (UInt***) syBase;
        }
        else
            ret = (UInt***) -1;
    }

    /* don't shrink memory but mark it as deactivated                      */
    else if ( size < 0 ) {
        adr = (vm_address_t)( (char*) syBase + (sySize+size) );
        if ( vm_deallocate(task_self(),adr,-size) == KERN_SUCCESS ) {
            ret = (UInt***)( (char*) syBase + sySize );
            sySize += size;
        }
        else
            ret = (UInt***) -1;
    }

    /* get more memory from system                                         */
    else {
        adr = (vm_address_t)( (char*) syBase + sySize );
        if ( vm_allocate(task_self(),&adr,size,FALSE) == KERN_SUCCESS ) {
            ret = (UInt***) ( (char*) syBase + sySize );
            sySize += size;
        }
        else
            ret = (UInt***) -1;
    }

    /* test if the allocation failed                                       */
    if ( ret == (UInt***)-1 && need ) {
        fputs("gap: cannot extend the workspace any more\n",stderr);
        SyExit(1);
    }

    /* otherwise return the result (which could be 0 to indicate failure)  */
    if ( ret == (UInt***)-1 )
        return 0;
    else
        return ret;

}

#endif


/****************************************************************************
**
*f  SyAllocBags( <size>, <need> ) . . . . . . . . . . . . . . . . . . MAC MPW
**
**  For the MAC under MPW we currently use 'calloc'.  This  does not allow to
**  extend the arena, but this is a problem of the memory manager anyhow.
*/
#if SYS_MAC_MPW
#error "You can't compile GAP on this platform. It should be fixed. "

/* this stuff is obsolete and should be converted to SyAllocBags */

#ifndef SYS_HAS_CALLOC_PROTO
extern  char *          calloc ( int, int );
#endif
char * syWorkspace;

char * SyGetmem ( size )
    long                size;
{
  size = size*1024;
    /* get the memory                                                      */
    /*N 1993/05/29 martin try to make it possible to extend the arena      */
    if ( syWorkspace == 0 ) {
        syWorkspace = calloc( (int)size/4, 4 );
        syWorkspace = (char*)(((long)syWorkspace + 3) & ~3);
        return syWorkspace;
    }
    else {
        return (char*)-1;
    }
}

#endif



/****************************************************************************
**
*f  SyAllocBags( <size>, <need> ) . . . . . . . . . . . . . . . . . . MAC MWC
**
**  For Mac under CodeWarrior, we use 'NewPtr to allocate as much memory
**  as possible at startup, then hand it on to GAP as required.
*/
#if SYS_MAC_MWC

UInt * * * 		syWorkspace;
long       		syWorksize = 0;  /* currently allocated amount in KB*/
long			SyStorLimit;     /* maximum allocable amount in KB*/

UInt * * * SyAllocBags (
    Int                 size,
    UInt                need )
{
	UInt*** ret;
	long *p;
	unsigned long count;
	
    if ( (0 < size && (syWorksize + size <= SyStorMax || 
    				   (need && syWorksize + size <= SyStorLimit)))
      || (size < 0 && SyStorMin <= syWorksize + size) ) {
      
		ret = (UInt***)((char*)syWorkspace + (syWorksize << NUM_TO_UINT(10)) );
        syWorksize += size;
		syLastFreeWorkspace += size;
		
       /* set the overrun flag if we became larger than SyStorMax */
       if ( syWorksize > SyStorMax && size > 0)  {
	 		SyStorOverrun = -1;
			SyStorMax=syWorksize+1; /* new maximum */
       }
		/* clear memory, 256 bytes at a time */
 		p = (long *) ret;
		if (size > 0) {
			count = size * (1024UL / sizeof(*p) / 64);
			while (count--) {
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; 
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; 
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; 
				*p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0; *p++ = 0;
			}
		}
        return ret;
    }
    else 
 	   if ( need ) {
	        syEchos("gap: cannot extend the workspace any more\n",3);
	        SyExit( 1 );
	    } 
	return (UInt***) 0;
}
#endif


/****************************************************************************
**
*F  SyAbortBags( <msg> )  . . . . . . . . . abort GAP in case of an emergency
**
**  'SyAbortBags' is the function called by Gasman in case of an emergency.
*/
void SyAbortBags (
    Char *              msg )
{
    SyFputs( msg, 3 );
    abort();
    /*SyExit( 2 );*/
}

/****************************************************************************
**

*F * * * * * * * * * * * * * initialize package * * * * * * * * * * * * * * *
*/


/****************************************************************************
**
*F  InitSystem4( <argc>, <argv> )  . . .  initialize system package from GAP4
**
**  'InitSystem4' is called very early during the initialization from  'main'.
**  It is passed the command line array  <argc>, <argv>  to look for options.
**
**  For UNIX it initializes the default files 'stdin', 'stdout' and 'stderr',
**  installs the handler 'syAnsIntr' to answer the user interrupts '<ctr>-C',
**  scans the command line for options, tries to  find  'LIBNAME/init.g'  and
**  '$HOME/.gaprc' and copies the remaining arguments into 'SyInitfiles'.
*/

#ifndef SYS_HAS_MALLOC_PROTO
# if SYS_ANSI                           /* ANSI decl. from H&S 16.1, 16.2  */
extern void * malloc ( size_t );
extern void   free ( void * );
# else                                  /* TRAD decl. from H&S 16.1, 16.2  */
extern char * malloc ( unsigned );
extern void   free ( char * );
# endif
#endif


#if SYS_TOS_GCC2
# ifndef SYS_BASEPAGE_H                 /* definition of basepage          */
#  include      <basepage.h>
#  define SYS_BASEPAGE_H
# endif
#endif


#if SYS_MAC_MPW || SYS_MAC_MWC
# ifndef SYS_HAS_TOOL
#  ifndef SYS_MEMORY_H                  /* Memory stuff:                   */
#   include     <Memory.h>              /* 'GetApplLimit', 'SetApplLimit', */
#   define SYS_MEMORY_H                 /* 'MaxApplZone', 'StackSpace',    */
#  endif                                /* 'MaxMem'                        */
# endif
#endif


#if SYS_MAC_MWC
# include <gestalt.h>
# include <folders.h>
#endif


static UInt ParseMemory( Char * s)
{
  UInt size;
  size = atoi(s);
  if ( s[SyStrlen(s)-1] == 'k'
       || s[SyStrlen(s)-1] == 'K' )
    size = size * 1024;
  if ( s[SyStrlen(s)-1] == 'm'
       || s[SyStrlen(s)-1] == 'M' )
    size = size * 1024 * 1024;
  if ( s[SyStrlen(s)-1] == 'g'
       || s[SyStrlen(s)-1] == 'G' )
    size = size * 1024* 1024 * 1024;
  return size;
}

#define ONE_ARG(opt) \
        case opt: \
            if ( argc < 3 ) { \
                FPUTS_TO_STDERR("gap4: option " #opt " must have an argument.\n"); \
                goto usage; \
            } 

void InitSystem4 (
    Int                 argc,
    Char *              argv [] )
{
#if SYS_MAC_MWC
    char				first;  /* dummy for checking stack ptr */
    Int                 pre = 0;        /* amount to reserve for shared libs */
#else
    Int                 pre = 100*1024; /* amount to pre'malloc'ate        */
#endif
    Char *              ptr;            /* pointer to the pre'malloc'ated  */
    Char *              ptr1;           /* more pre'malloc'ated  */
    UInt                i;              /* loop variable                   */
#if SYS_MAC_MWC
    Size		mem;
    OSErr 		err;
    char				last;  /* dummy for checking stack ptr */
#endif

#if SYS_MAC_MPW
# ifndef SYS_HAS_TOOL
    /* Increase the amount of stack space available to GAP.                */
    /* Following "Inside Macintosh - Memory" 1992, pages 1-42.             */
    /* For use with MPW 'SIOW.o' *after* changing instruction word         */
    /* at 3F94 from 'A063' (call to '_MaxApplZone') to '4E71' (NOP).       */
    /* 'fix_SIOW.c' is the source for an MPW tool, which does this safely. */
    /* Otherwise bungee-jumping the stack will lead to fatal head injuries.*/
    /*                                              Dave Bayer, 1994/07/14 */
    SetApplLimit( GetApplLimit() - (syStackSpace - StackSpace() + 1024) );
    MaxApplZone();
    if ( StackSpace() < syStackSpace ) {
        FPUTS_TO_STDERR("gap4: cannot get enough stack space.\n");
        SyExit( 1 );
    }
# endif
#endif

#if SYS_MAC_MWC	
# if !TARGET_API_MAC_CARBON && !powerc 
    SyMinStack = GetApplLimit() - (syStackSpace - StackSpace()) + 1024;
    SetApplLimit( SyMinStack );
    /* compute the least possible value for the stack pointer */
    SyMinStack = (&last < &first ? &last : &first) - StackSpace () + 8192;
# else /* we need more reserve stack for the PPC, apparently */
    SyMinStack = (&last < &first ? &last : &first) - StackSpace () + 65536;
# endif
    MaxApplZone();
#endif

    /* scan the command line for options                                   */
    while ( argc > 1 && argv[1][0] == '-' ) {
        if ( SyStrlen(argv[1]) != 2 ) {
            FPUTS_TO_STDERR("gap: sorry, options must not be grouped '");
            FPUTS_TO_STDERR(argv[1]);  FPUTS_TO_STDERR("'.\n");
            goto usage;
        }

        switch ( argv[1][1] ) {
#if SYS_MAC_MWC
        /* '-W <memory>', change the value of 'gMaxLogSize'                */
	ONE_ARG('W');
	    gMaxLogSize = ParseMemory(argv[2]);
            ++argv; --argc; break;
#endif

        /* '-a <memory>', set amount to pre'm*a*lloc'ate                   */
	ONE_ARG('a');
	    pre = ParseMemory( argv[2] );
            ++argv; --argc; break;

        /* '-c', change the value of 'SyCacheSize'                         */
	ONE_ARG('c');
	    SyCacheSize = ParseMemory( argv[2]);
            ++argv; --argc; break;

        /* '-g', Gasman should be verbose                                  */
        ONE_ARG('g');
            SyMsgsFlagBags = (SyMsgsFlagBags + 1) % 3;
            ++argv; --argc; break;

	/* '-m <memory>', change the value of 'SyStorMin'                  */
        ONE_ARG('m');
	    SyStorMin = ParseMemory( argv[2])/1024;
            ++argv; --argc; break;

        /* '-o <memory>', change the value of 'SyStorMax'                  */
        ONE_ARG('o');
	    SyStorMax = ParseMemory( argv[2])/1024;
            ++argv; --argc; break;

        /* '-K <memory>', set the value of 'SyStorKill'                    */
	ONE_ARG('K');
	    SyStorKill = ParseMemory( argv[2] ) /1024;
            ++argv; --argc; break;

        /* '-h', print a usage help                                        */
        case 'h':
            goto fullusage;

        /* default, skip unknown option, GAP3 should handle it             */
        default: break;
	  /*
            FPUTS_TO_STDERR("gap: '");  FPUTS_TO_STDERR(argv[1]);
            FPUTS_TO_STDERR("' option is unknown.\n");
            goto usage;*/

        }
        ++argv; --argc;
    }

    /* fix max if it is lower than min                                     */
    if ( SyStorMax < SyStorMin )
        SyStorMax = SyStorMin;

#if SYS_MAC_MWC
    /* find out how much memory we can now allocate in the zone            */
    if (gPrintBufferSize < NUM_TO_UINT(32)*NUM_TO_UINT(1024))
	gEditorScratch = NUM_TO_UINT(32)*NUM_TO_UINT(1024);
    else 
	gEditorScratch = gPrintBufferSize;
			
    SyStorLimit = MaxMem( &mem );
    SyStorLimit -= gEditorScratch + gMaxLogSize + pre;  
    /* make SyStorLimit divisible by the minimum allocatable unit */

#if GAPVER == 4
    SyStorLimit /= NUM_TO_UINT(1024);
    SyStorLimit -= SyStorLimit % NUM_TO_UINT(512);
#elif GAPVER == 3
    SyStorLimit -= SyStorLimit % 1024;
#endif

    /* try to set SyStorMax so that the user gets a warning before memory is too low */
    if (SyStorMax > SyStorLimit)
	SyStorMax = SyStorLimit - NUM_TO_UINT(512);
    
    if ( SyStorMin <= 0 ) 
	SyStorMin = SyStorMax;

    syWorkspace = (UInt***) NewPtr (SyStorLimit*NUM_TO_UINT(1024));  /* allocate all we can get */

    if ( SyStorMax < SyStorMin || !syWorkspace) {
	SyFputs("gap4: please use the 'Get Info' command in the Finder 'File' menu\n",  3 );  
	SyFputs("      to increase the minimum amount of memory and the preferred amount of memory\n", 3);
	SyFputs("      as described in the documentation of GAP for MacOS.\n", 3 );
	SyExit( 1 );
    }
#else
    /* premalloc stuff                                                     */
    // ptr = (Char *)malloc( pre );
    // ptr1 = (Char *)malloc(4);
    // if ( ptr != 0 )  free( ptr );
#endif

#if SYS_TOS_GCC2
    /* for TOS we compute the amount of allocatable memory                 */
    if ( SyStorMin <= 0 ) {
        SyStorMin = (UInt)_base->p_hitpa - (UInt)_base->p_lowtpa
                   - _base->p_tlen - _base->p_dlen - _base->p_blen
                   - _stksize - pre - 8192 + SyStorMin;
    }
#endif

#if SYS_MAC_MPW
# ifndef SYS_HAS_TOOL
    /* find out how much memory we can now allocate in the zone            */
    if ( SyStorMin <= 0 ) {
        SyStorMin = MaxMem( &i ) - SyStorMin - 384*1024;
        if ( SyStorMin < 1024*1024 ) {
            FPUTS_TO_STDERR("gap4: please use the 'Get Info' command in the Finder 'Desk' menu\n");
            FPUTS_TO_STDERR("      to set the minimum amount of memory to at least 2560 KByte,\n");
            FPUTS_TO_STDERR("      and the preferred amount of memory to 5632 KByte or more.\n");
            SyExit( 1 );
        }
    }
# endif
#endif
    /* now we start                                                        */
    return;

    /* print a usage message                                               */
 usage:
    FPUTS_TO_STDERR("usage: gap4 [OPTIONS] [FILES]\n");
    FPUTS_TO_STDERR("       use '-h' option to get help.\n");
    FPUTS_TO_STDERR("\n");
    SyExit( 1 );
  
 fullusage:
    FPUTS_TO_STDERR("usage: gap4 [OPTIONS] [FILES]\n");
    FPUTS_TO_STDERR("\n");
    FPUTS_TO_STDERR("  -g          show GASMAN messages (full garbage collections)\n");
    FPUTS_TO_STDERR("  -g -g       show GASMAN messages (all garbage collections)\n");
    FPUTS_TO_STDERR("  -m <mem>    set the initial workspace size\n");
    FPUTS_TO_STDERR("  -o <mem>    set hint for maximal workspace size (GAP may allocate more)\n");
    FPUTS_TO_STDERR("  -K <mem>    set maximal workspace size (GAP never allocates more)\n");
    FPUTS_TO_STDERR("  -c <mem>    set the cache size value\n");
    FPUTS_TO_STDERR("  -a <mem>    set amount to pre-malloc-ate\n");
    FPUTS_TO_STDERR("              postfix 'k' = *1024, 'm' = *1024*1024, 'g' = *1024*1024*1024\n");
    FPUTS_TO_STDERR("\n");
    SyExit( 1 );
}


/****************************************************************************
**
*E  system.c  . . . . . . . . . . . . . . . . . . . . . . . . . . . ends here
*/

