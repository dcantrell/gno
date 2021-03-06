/*
 * udl - Convert EOL formats freely between MS-DOS (CR/LF), Unix/Amiga (LF),
 *       and Apple (CR).
 *
 * Unix specific routines.
 *
 * $Id: udlunix.c,v 1.11 1999/01/15 07:36:15 gdr-ftp Exp $
 *
 * Copyright (c) 1993-1995 Soenke Behrens, Devin Reade
 */

#include "common.h"

extern char *strdup(const char *);

int main(int argc,char *argv[]) {
  FILE *infile, *outfile;
  int Tunix = FALSE;
  int Messy = FALSE;
  int GS = FALSE;
  int careful = FALSE;
  int converted;
  int c;
  int R_flag = FALSE;

  verbose = FALSE;
  program_name = NULL;
  tempfile = NULL;
  current_file = NULL;
  in_buffer = out_buffer = NULL;
  pathSlotsUsed = 0;
  pathSlots = 0;
  pathList = NULL;
  *currentDirectory = '\0';
  recursionDepth=0;

  /* In case of exit(), free the mem I allocated */
#ifdef HAS_ATEXIT
  atexit (cleanup);
#endif

  if ((program_name = strdup (argv[0])) == NULL) {
    fprintf(stderr,"%s: memory allocation failure\n",argv[0]);
    exit (EXIT_FAILURE);
  }

  if (argc < 2) {
    usage();
    exit (EXIT_FAILURE);
  }
  
  /* Get and process arguments */

  opterr = 1;
  while ((c = getopt (argc, argv, "pvugmRVh")) != EOF) {
    switch (c) {
    case 'v':
      verbose = TRUE;
      break;

    case 'p':
      careful = TRUE;
      break;

    case 'u':
      if (Tunix == TRUE || Messy == TRUE || GS == TRUE) {
        fprintf(stderr, "%s: You may not "
                "specify more than one conversion option\n",program_name);
        exit (EXIT_FAILURE);
      }
      Tunix = TRUE;
      break;
        
    case 'm':
      if (Tunix == TRUE || Messy == TRUE || GS == TRUE) {
        fprintf(stderr, "%s: You may not specify more than one "
                "conversion option\n",program_name);
        exit (EXIT_FAILURE);
      }
      Messy = TRUE;
      break;
        
    case 'g':
      if (Tunix == TRUE || Messy == TRUE || GS == TRUE) {
        fprintf(stderr, "%s: You may not specify more than one "
                "conversion option\n",program_name);
        exit (EXIT_FAILURE);
      }
      GS = TRUE;
      break;
        
    case 'R':
      R_flag++;
      break;

    case 'V':
      printf ("%s %s\n",program_name,UDL_VERSION);
      exit (EXIT_FAILURE);

    case '?':
    case 'h':
      usage();
      exit (EXIT_FAILURE);
        
    default:
      fprintf (stderr, "%s: Internal getopt error\n", program_name);
      exit (EXIT_FAILURE);
      break;
    }
  }

  /* no files specified */
  if (optind == argc) { 
    usage();
    exit (EXIT_FAILURE);
  }

  if (Tunix == FALSE && GS == FALSE && Messy == FALSE) {
    fprintf(stderr,"%s: You have to specify a destination "
            "format.\n",program_name);
    exit (EXIT_FAILURE);
  }

  if (verbose == TRUE) {
    printf ("%s %s\n",program_name,UDL_VERSION);
  }

  /* Now process the files I got */

  if ((in_buffer = malloc(BUFFERSIZE+1)) == NULL ||
      (out_buffer = malloc(BUFFERSIZE+1)) == NULL) {
    fprintf(stderr,"%s: Unable to buffer files\n",program_name);
    exit (EXIT_FAILURE);
  }
  
  /* save the directory we're in */
  if (getcwd(rootdir, MAXPATHLEN)==NULL) {
    fprintf(stderr,"%s: getcwd failed for .: %s\n", 
	    program_name, strerror(errno));
    exit (EXIT_FAILURE);
  }

  /* set the directory separator character. */
  dirbrk = '/';

  /* build the list of files to process */
  for (; optind<argc; optind++) {
    build_file_list(argv[optind],R_flag);
    chdir(rootdir);
    *currentDirectory = '\0';
  }

  /* files were all directories and no -R flag given */
  if (!pathList) {
    if (verbose) printf("%s: no files to process\n",program_name);
    exit(EXIT_SUCCESS);
  }

  for (c=0; pathList[c]; c++) {
    if((current_file = strdup(pathList[c])) == NULL) {
      fprintf(stderr,"%s: memory allocation failure\n", program_name);
      exit (EXIT_FAILURE);
    }

    if (verbose == TRUE) {
      printf("%s: Working on %s\n",program_name,
             current_file);
    }

    infile = tryopen(current_file,"r+b");
    tempfile = Mktemp(strcat(get_path(current_file), "udltmpXX"));
    outfile = tryopen(tempfile,"wb");

    if (careful) {
      converted = TRUE; /* always */
      
      if (GS)
        convert_gs(infile,outfile);
      else if (Tunix)
        convert_tunix(infile,outfile);
      else
        convert_messy(infile,outfile);
    } else {
      if (GS)
        converted = convert_fast_gs(infile,outfile);
      else if (Tunix)
        converted = convert_fast_tunix(infile,outfile);
      else
        converted = convert_fast_messy(infile,outfile);
    }

    if (fclose (infile) == EOF || fclose (outfile) == EOF) {
      perror ("closing files");
      exit (EXIT_FAILURE);
    }

    if (converted) { /* Temp file contains converted data */
      if (remove (current_file) != 0) {
        perror ("removing original file");
        exit (EXIT_FAILURE);
      }
      
      if (rename (tempfile,current_file) != 0) {
	perror ("cannot rename temporary file");
	exit (EXIT_FAILURE);
      }
    } else
      remove (tempfile);

    free (tempfile); tempfile = NULL;
    free(current_file);
    current_file = NULL;

  } /* for (c=0; pathList[c]; c++) */

  return (EXIT_SUCCESS);
}

/* End Of File */
