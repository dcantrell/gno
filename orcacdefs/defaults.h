/*************************************************
 * Start of GNO v2.0.6 definitions.
 *
 * $Id: defaults.h,v 1.6 1998/01/25 21:05:28 gdr-ftp Exp $
 */

#define	__appleiigs__
#define	__GNO__

#pragma path "/usr/include"
#pragma path "/HFSinclude"
#pragma path "/lang/orca/libraries/orcacdefs"

/*
 * These should be identically equal.  Orca/C headers use one, BSD headers
 * use the other.  Unfortunately, this construct will miss definitions
 * given in source files -- it is only caught on the occ command line.
 */
#if defined(__KeepNamespacePure__) && !defined(_ANSI_SOURCE)
#define _ANSI_SOURCE
#endif
#if defined(_ANSI_SOURCE) && !defined(__KeepNamespacePure__)
#define __KeepNamespacePure__
#endif

/*
 * End of GNO v2.0.6 definitions.
 *************************************************/
