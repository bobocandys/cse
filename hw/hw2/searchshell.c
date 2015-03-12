/*
 * Copyright 2011 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

static void Usage(void);

// Private helper function to get query and print results
static char * getQuery(char **query, int *qsize);
static void printResult(LinkedList retlist, DocTable dt);

int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.

  DocTable dt;
  MemIndex midx;
  LinkedList retlist;
  char *input;
  int res, qsize = 0;
  char *query[128];

  // Crawl the file
  printf("Indexing %s\n", argv[1]);
  res = CrawlFileTree(argv[1], &dt, &midx);

  if (res == 0) {
    // Failure to crawl the file
    Usage();
  }

  // Loop until end-of-fine detected
  while (true) {
    // Get the query from user
    input = getQuery(query, &qsize);

    if (input == NULL) {
      break;
    }

    // process the query
    retlist = MIProcessQuery(midx, (char **)query, qsize);

    // If we get valid results, print them
    if (retlist != NULL) {
      printResult(retlist, dt);
    }

    free(input);
    qsize = 0;
  }

  FreeDocTable(dt);
  FreeMemIndex(midx);

  return EXIT_SUCCESS;
}


static void printResult(LinkedList retlist, DocTable dt) {
  LLIter it = LLMakeIterator(retlist, 0);
  Verify333(it != NULL);
  SearchResult *res;

  do {
    LLIteratorGetPayload(it, (void **)&res);
    DocID_t docid = res->docid;
    HWSize_t rank = res->rank;

    // Get the doc name from the docid
    char *docname = DTLookupDocID(dt, docid);

    // Print the result
    printf("  %s (%u)\n", docname, rank);
  } while (LLIteratorNext(it));

  LLIteratorFree(it);
  FreeLinkedList(retlist, (LLPayloadFreeFnPtr)free);
}


static char * getQuery(char **query, int *qsize) {
  char *input = (char *)malloc(256);
  char *saveptr;  // For strtok_r
  int i;

  // Prompt
  printf("enter query: \n");

  if (fgets(input, 255, stdin) == NULL) {
    free(input);
    return NULL;
  }

  for (i = 0; i < 256; i++) {
    if (input[i] == '\n') {
      input[i] = '\0';
      break;
    }
  }

  query[*qsize] = strtok_r(input, " ", &saveptr);

  if (query[*qsize] == NULL) {
    // Invalid query
    Usage();
  }

  while (query[*qsize] != NULL) {
    (*qsize)++;
    query[*qsize] = strtok_r(NULL, " ", &saveptr);
  }

  return input;
}


static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(-1);
}

