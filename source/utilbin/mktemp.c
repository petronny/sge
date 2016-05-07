/*
 * Copyright (c) 1996, 1997, 2001, 2004, 2008, 2010
 *	Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* This a version of the portable mktemp from http://www.mktemp.org,
   modified for SGE.  It avoids the autoconfiscation, with the
   portability assumptions of SGE, and amalgamates the mkdtemp
   implementation with the main file.  (We can assume mkstemp, but not
   mkdtemp.)  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _PATH_TMP
#define _PATH_TMP "/tmp"
#endif

char *__progname;

void usage (void);

char *my_mkdtemp(char *template);

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, fd, uflag = 0, quiet = 0, tflag = 0, Tflag = 0, makedir = 0;
	char *cp, *template, *tempfile, *prefix = _PATH_TMP;
	size_t plen;
	extern char *optarg;
	extern int optind;

	__progname = argv[0];

	while ((ch = getopt(argc, argv, "dp:qtuV")) != -1)
		switch (ch) {
		case 'd':
			makedir = 1;
			break;
		case 'p':
			prefix = optarg;
			tflag = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'T':
			if (optarg) {
				Tflag = 1;
				prefix = optarg;
			}
			/* FALLTHROUGH */
		case 't':
			tflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case 'V':
			printf("%s version 1.7x\n", __progname);
			exit(0);
		default:
			usage();
	}

	/* If no template specified use a default one (implies -t mode) */
	switch (argc - optind) {
	case 1:
		template = argv[optind];
		break;
	case 0:
		template = "tmp.XXXXXXXXXX";
		tflag = 1;
		break;
	default:
		usage();
	}

	if (tflag) {
		if (strchr(template, '/')) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: template must not contain directory separators in -t mode\n", __progname);
			exit(1);
		}

		if (!Tflag) {
			cp = getenv("TMPDIR");
			if (cp != NULL && *cp != '\0')
				prefix = cp;
		}
		plen = strlen(prefix);
		while (plen != 0 && prefix[plen - 1] == '/')
			plen--;

		tempfile = (char *)malloc(plen + 1 + strlen(template) + 1);
		if (tempfile == NULL) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: cannot allocate memory\n", __progname);
			exit(1);
		}
		(void)memcpy(tempfile, prefix, plen);
		tempfile[plen] = '/';
		(void)strcpy(tempfile + plen + 1, template);	/* SAFE */
	} else {
		if ((tempfile = strdup(template)) == NULL) {
			if (!quiet)
				(void)fprintf(stderr,
				    "%s: cannot allocate memory\n", __progname);
			exit(1);
		}
	}

	if (makedir) {
		if (my_mkdtemp(tempfile) == NULL) {
			if (!quiet) {
				(void)fprintf(stderr,
				    "%s: cannot make temp dir %s: %s\n",
				    __progname, tempfile, strerror(errno));
			}
			exit(1);
		}

		if (uflag)
			(void)rmdir(tempfile);
	} else {
		if ((fd = mkstemp(tempfile)) < 0) {
			if (!quiet) {
				(void)fprintf(stderr,
				    "%s: cannot create temp file %s: %s\n",
				    __progname, tempfile, strerror(errno));
			}
			exit(1);
		}
		(void)close(fd);

		if (uflag)
			(void)unlink(tempfile);
	}

	(void)puts(tempfile);
	free(tempfile);

	exit(0);
}

void
usage()
{

	(void)fprintf(stderr,
	    "Usage: %s [-V] | [-dqtu] [-p prefix] [template]\n",
	    __progname);
	exit(1);
}

/*
 * Very simple-minded mkdtemp() replacement.
 */
char *
my_mkdtemp(template)
	char *template;
{
	char *otemplate;
	int i, oerrno, error;

	otemplate = strdup(template);
	if (!otemplate)
		return(NULL);

	for (i = 0; i < 1000; i++) {
		error = mktemp(template) == NULL;
		if (error)
			break;
		error = mkdir(template, S_IRUSR|S_IWUSR|S_IXUSR);
		if (!error || errno != EEXIST)
			break;
		(void)strcpy(template, otemplate); /* SAFE */
	}
	oerrno = errno;
	free(otemplate);
	errno = oerrno;
	return(error ? NULL : template);
}
