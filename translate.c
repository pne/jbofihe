/***************************************
  $Header$

  Translation functions.
  ***************************************/

/* COPYRIGHT */

#include <gdbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "functions.h"

static int inited = 0;
static GDBM_FILE db;

/* ================================================== */

static void
init(void) 
{
  if (!inited) {
    inited = 1;
    db = gdbm_open("./dictionary.dbm", 0, GDBM_READER, 0, NULL);
  }
}

/* ================================================== */


char *
translate(char *word)
{
  datum loj, eng;
  static char buf[1024];
  int i;

  init();
  
  loj.dptr = word;
  loj.dsize = strlen(word);
  eng = gdbm_fetch(db, loj);
  if (eng.dptr) {
    for (i=0; i<eng.dsize; i++) {
      buf[i] = eng.dptr[i];
    }
    buf[i] = 0;
#if 1
    free(eng.dptr);
#endif
    return buf;
  } else {
    return NULL;
  }
  
}

/* ================================================== */

static void
append_trans(char *trans, char *result, int initial)
{
  if (trans) {
    if (initial) {
      strcpy(result, trans);
    } else {
      strcat(result, "-");
      strcat(result, trans);
    }
  } else {
    if (initial) {
      strcpy(result, "?");
    } else {
      strcat(result, "-?");
    }
  }
}


/* ================================================== */

static char *
translate_comp(char *w, int *conversion)
{
  char buf[64], buf2[4];
  static char result[256];
  char *trans;
  strcpy(buf, w);
  if (*conversion) {
    buf2[0] = '0' + *conversion;
    buf2[1] = 0;
    strcat(buf, buf2);
    trans = translate(buf);
    if (!trans) {
      result[0] = 0;
      switch (*conversion) {
        case 2:
          strcpy(result, "2nd conv-");
          break;
        case 3:
          strcpy(result, "3rd conv-");
          break;
        case 4:
          strcpy(result, "4th conv-");
          break;
        case 5:
          strcpy(result, "5th conv-");
          break;
      }
      trans = translate(w);
      if (trans) {
        strcat (result, trans);
      } else {
        strcat (result, "??");
      }
      return result;
    }
  } else {
    trans = translate(w);
  }
  *conversion = 0;
  return trans;
}

/* ================================================== */

/* Lookup a lujvo that isn't matched in the ordinary dictionary, by
   smashing it into consituent rafsi and glueing these together. */
static char *
translate_lujvo(char *word)
{
  static char result[4096];
  char buf[64];
  char *w, *trans;
  int initial;
  char *yy;
  int ypos=0;
  int conversion=0;

  w = word;
  result[0] = 0;
  initial = 1;

  do {
    if (strlen(w) == 5) {
      /* trailing gismu */
      trans = translate_comp(w, &conversion);
      append_trans(trans, result, initial);
      break;
    } else {
      /* Not 5 letter, see what else */
      if (strlen(w) < 5) {
        /* Don't bother about whether this is initial or not, we need to be able to lookup
           single rafsi for breaking fuivla up. */
        
        strcpy(buf,"%");
        strcat(buf,w);
        trans = translate_comp(buf, &conversion);
        if (trans) {
          if (!initial) {
            strcat(result, "-");
          }
          strcat(result, trans);
        } else {
          if (!initial) {
            strcat(result, "-");
          }
          strcat(result, "?");
        }
        break;
      } else {
        /* Try to pull off leading rafsi component and process remainder */
        yy = strchr(w, 'y');
        if (yy) {
          ypos = yy - w;
        }
        if (yy && ypos == 3) {
          /* 3 letter rafsi with y after */
          if (!strncmp(w, "sel", 3)) {
            conversion = 2;
          } else if (!strncmp(w, "ter", 3)) {
            conversion = 3;
          } else if (!strncmp(w, "vel", 3)) {
            conversion = 4;
          } else if (!strncmp(w, "xel", 3)) {
            conversion = 5;
          } else {
            buf[0] = '%';
            strncpy(buf+1,w,3);
            buf[4] = 0;
            trans = translate_comp(buf, &conversion);
            append_trans(trans, result, initial);
          }
          w += 4; /* and go around again */
        } else if (yy && ypos == 4) {
          /* 4 letter rafsi with y after */
          buf[0] = '%';
          strncpy(buf+1,w,4);
          buf[5] = 0;
          trans = translate_comp(buf, &conversion);
          append_trans(trans, result, initial);
          w += 5; /* and go around again */

        } else {
          /* 'y' does not terminate leading rafsi, or there is no 'y'
             in the word at all.  Try to pull off 3 letter rafsi, or 4
             letter one of form CV'V.  Remember to remove following
             glue character if necessary, can only apply to first
             rafsi. */
          buf[0] = '%';
          if (w[2] == '\'') {
            strncpy(buf+1,w,4);
            buf[5] = 0;
            trans = translate_comp(buf, &conversion);
            append_trans(trans, result, initial);

            if (strchr("aeiou", w[1]) &&
                strchr("aeiou", w[3]) &&
                initial &&
                w[4] &&
                (w[4] == 'r' ||
                 (w[4] == 'n' && w[5] == 'r'))) {
              w += 5;
            } else {
              w += 4;
            }
          } else {
            if (!strncmp(w, "sel", 3)) {
              conversion = 2;
            } else if (!strncmp(w, "ter", 3)) {
              conversion = 3;
            } else if (!strncmp(w, "vel", 3)) {
              conversion = 4;
            } else if (!strncmp(w, "xel", 3)) {
              conversion = 5;
            } else {
              strncpy(buf+1,w,3);
              buf[4] = 0;
              trans = translate_comp(buf, &conversion);
              append_trans(trans, result, initial);
            }
            if (strchr("aeiou", w[1]) &&
                strchr("aeiou", w[2]) &&
                initial &&
                w[3] &&
                (w[3] == 'r' ||
                 (w[3] == 'n' && w[4] == 'r'))) {
              w += 4;
            } else {
              w += 3;
            }
          }          
        }
      }
    }

    initial = 0;

  } while (*w);
  return result;
}

/* ================================================== */

static int
is_consonant(char c)
{
  return (strchr("bcdfgjklmnprstvxz", c) != 0);
}

/* ================================================== */

static int
is_consonant_not_r(char c)
{
  return (strchr("bcdfgjklmnpstvxz", c) != 0);
}

/* ================================================== */

/* ================================================== */

/* ================================================== */

char *
translate_unknown(char *w)
{
  static char buf[2048];
  int len, i;
  int hyph;
  char *p, *q;
  char *ltrans;

  init();

  /* See whether the word is a fuivla.  If so, lookup the leading
     portion as a lujvo/rafsi, otherwise lookup the whole thing as a
     lujvo. */
  
  /* Stage 3 fuivla characterised by starting with a CVC or 4 letter
     rafsi or lujvo, then a hyphen, then a lojbanised version of the
     import */
  
  len = strlen(w);
  hyph = 0;
  /* Seek location of hyphen.  Import word must have at least 2 letters. */
  for (i=1; i<len-2; i++) {
    if (is_consonant_not_r(w[i-1]) && (w[i] == 'r') && is_consonant_not_r(w[i+1])) {
      hyph = i;
      break;
    }
    
    if (is_consonant(w[i-1]) && is_consonant(w[i+1])) {
      if ((w[i-1] == 'n') && (w[i+1] == 'r') && (w[i] == 'l')) {
        hyph = i;
        break;
      } else if ((w[i-1] == 'r') && (w[i+1] == 'n') && (w[i] == 'l')) {
        hyph = i;
        break;
      } else if (w[i] == 'n') {
        hyph = i;
        break;
      }
    }
  }

  if (hyph) {
    for (p=w, q=buf, i=0; i<hyph; p++, q++, i++) {
      *q = *p;
    }
    *q = 0;
    ltrans = translate_lujvo(buf);
    strcpy(buf, ltrans);
    strcat(buf, "-[");
    strcat(buf, w + hyph + 1);
    strcat(buf, "]");
    return buf;
  } else {
    /* Need to try for a stage 4 fuivla */

    return translate_lujvo(w);
  }

}

/* ================================================== */

/*

  This section of the file deals with what I call 'advanced
  translation'.  The idea is to have (or be able to generate)
  different glosses for the various positions of brivla, depending on
  the context in which they arise in the text.

  The contexts that are recognized are :

  le X : noun (N)
  le A cu X : verb (V)
  le A cu X Y : adjective/adverb (=qualifier Q)
  [X->] le A cu Y : case tag (T)

  We identify 4 classes of word.  Different places of the same brivla
  may fall into different classes.  These are (patterns shown in []) :

  (D) discrete noun [x1 is a _, x1 are _ (plural)]
      e.g. nanmu (x1 is a man)
  (S) substance noun [x1 is _, x1 is _-like]
      e.g. djacu [x1 is some water, x1 is water-like)
  (A) actor noun [x1 is a _ (actor), x1 _ (action verb)]
      e.g. bajra (x1 is a runner, x1 runs)
  (P) property (adjective) [x1 is _ (adjective)]
      e.g. badri (x1 is sad)

  (Perhaps some other classes will be defined later.)

  We write the entries in the dictionary like this

  nanmu1:D;man
  djacu1:S;water
  bajra1:A;run
  badri1:P;sad

  The glosses are generated according to the following table, where X is the
  gloss following the semicolon in the dictionary :

                                     CONTEXT
CLASS   |     N(oun)       V(erb) (4)          Q(ualifier)        T(ag) (2)
--------+--------------------------------------------------------------------
  D     |     X(s) (5)      being X(s) (6)          X              X(s) (6)
        |
  S     |       X            being X (6)            X               X (6)
        |
  A     |    X-er(s) (3)     X-ing (3)            X-ing (3)      X-er(s) (3)
        |
  P     |    X thing         being X                X            X thing

  Notes
  (1) a->an if X starts with a vowel.

  (2) Glosses not put in for the x1 place, since they are apparent
  from the form of the gloss applied to the selbri itself.

  (3) Double final consonant of X where required (X ends in VC?)
  
  (4) Verb forms are written as participles for 2 reasons.  First,
  they avoid worrying about to use is/are, plus using is/are looks
  silly with other than present tense.  Second, it looks good as part
  of an abstraction (le nu nanmu) - this almost flows as real English!

  (5) (es) if word ends in s, z, ch.  (ies) if it ends in (-Cy).

  (6) Replace X or X(s) by the noun form, if a special noun entry is
  found in the dictionary (unless then overridden by an explicit V or
  T entry)

  If this ruleset is insufficient for some reason, some of the glosses
  can be specifically provided.  For example, the DN combination for
  nanmu1 will give the gloss 'man(s)', which is bad.  In this case,
  the NVQT context can be specified~:
  
  nanmu1n:man/men

  In this case, the verb form will automatically become 'being a
  man/men', unless a nanmu1v entry is found.

-------------------

  Other horrors to look into :

  lo X should gloss as 'a,any' for D,A,P and 'some' for S


 */

static char consonants[] = "bcdfghjklmnpqrstvwxz";
static char vowels[] = "aeiou";

static char buffers[16][1024];
static int bufptr=0;
#define GETBUF() (&buffers[bufptr=(bufptr+1)&0xf][0])

static int
starts_with_preposition(char *x)
{
  char *y;
  y = x;
  while (isspace(*y)) y++;
  if (!strncmp(y, "in", 2) ||
      !strncmp(y, "at", 2) ||
      !strncmp(y, "with", 4) ||
      !strncmp(y, "through", 7)
      ) {
    return 1;
  } else {
    return 0;
  }

}

/*++++++++++++++++++++++++++++++++++++++
  Make a word plural, applying standard English rules for when to use
  -es or -ies instead of plain -s.

  static char * make_plural

  char *x
  ++++++++++++++++++++++++++++++++++++++*/

static char *
make_plural(char *x)
{
  char *result;
  int n;
  char *star_pos;

  result = GETBUF();

  star_pos = strchr(x, '*');
  if (star_pos) {
    char head[1024];
    char *result2;
    char *p, *q;
    result2 = GETBUF();
    p = x, q = head;
    while (p < star_pos) *q++ = *p++;
    *q = 0;
    p++;
    strcpy(result2, make_plural(head));
    if (strchr(p, '*')) {
      strcat(result2, make_plural(p));
    } else {
      strcat(result2, p);
    }
    return result2;
  } else {
    n = strlen(x);
    if (n>0 && (x[n-1] == 's' || x[n-1] == 'z' || x[n-1] == 'x')) {
      sprintf(result, "%s(es)", x);
    } else if (n>1 && x[n-1] == 'h' && x[n-2] == 'c') {
      sprintf(result, "%s(es)", x);
    } else if (n>1 && x[n-1] == 'y' && strchr(consonants, x[n-2])) {
      sprintf(result, "%s(ies)", x);
    } else {
      sprintf(result, "%s(s)", x);
    }

    return result;
  }

}


/*++++++++++++++++++++++++++++++++++++++
  Append "-er" to a word

  static char * append_er

  char *x
  ++++++++++++++++++++++++++++++++++++++*/

static char *
append_er(char *x)
{
  static char result[128];
  int n;
  char *star_pos;

  star_pos = strchr(x, '*');
  if (star_pos) {
    char head[1024];
    static char result2[1024];
    char *p, *q;
    p = x, q = head;
    while (p < star_pos) *q++ = *p++;
    *q = 0;
    p++;
    strcpy(result2, append_er(head));
    if (!starts_with_preposition(p)) {
      strcat(result2, " of");
    }
    strcat(result2, p);
    return result2;
  } else {
    n = strlen(x);
    if (n > 1 && strchr(vowels, x[n-2]) && strchr(consonants, x[n-1])) {
      if (n > 2 && strchr(vowels, x[n-3])) {
        sprintf(result, "%s-er(s)", x);
      } else {
        sprintf(result, "%s-%cer(s)", x, x[n-1]);
      }
    } else if (n > 0 && x[n-1] == 'e') {
      sprintf(result, "%s-r(s)", x);
    } else {
      sprintf(result, "%s-er(s)", x);
    }
    return result;
  }

}

/*++++++++++++++++++++++++++++++++++++++
  Append "-ing" to a word

  static char * append_ing

  char *x
  ++++++++++++++++++++++++++++++++++++++*/

static char *
append_ing(char *x)
{
  static char result[128];
  int n;
  char *star_pos;

  star_pos = strchr(x, '*');
  if (star_pos) {
    char head[1024];
    static char result2[1024];
    char *p, *q;
    p = x, q = head;
    while (p < star_pos) *q++ = *p++;
    *q = 0;
    p++;
    strcpy(result2, append_ing(head));
    strcat(result2, p);
    return result2;
  } else {

    n = strlen(x);
    if (n > 1 && strchr(vowels, x[n-2]) && strchr(consonants, x[n-1])) {
      if (n > 2 && strchr(vowels, x[n-3])) {
        sprintf(result, "%s-ing", x);
      } else {
        sprintf(result, "%s-%cing", x, x[n-1]);
      }
    } else if (n > 1 && x[n-1] == 'e' && !strchr(vowels, x[n-2])) {
      strncpy(result, x, n-1);
      result[n-1] = 0;
      strcat(result, "-ing");
    } else {
      sprintf(result, "%s-ing", x);
    }
    
    return result;
  }

}

/*++++++++++++++++++++++++++++++++++++++
  'Advanced' translate.

  char * adv_translate Returns the english gloss of the word passed.

  char *w

  TransContext ctx
  ++++++++++++++++++++++++++++++++++++++*/

char *
adv_translate(char *w, int place, TransContext ctx)
{
  char *trans;
  char w1[128], w1n[128];
  char buffer[1024];
  static char result[1024];
  char ctx_suffix[4] = "nvqt";
  enum {CL_DISCRETE, CL_SUBSTANCE, CL_ACTOR, CL_PROPERTY} wordclass;

  /* Try looking up the explicit gloss asked for */
  sprintf(buffer, "%s%1d%c", w, place, ctx_suffix[ctx]);
  trans = translate(buffer);

  if (trans) {
    /* Full translation found. */
    return trans;
  }

  /* OK, no full translation found.  Lookup the wn form */
  sprintf(buffer, "%s%1d", w, place);
  trans = translate(buffer);
  if (trans) {
    if (trans[1] == ';') {
      switch (trans[0]) {
        case 'D':
        case 'E': /* place holder for adding 'event' type later */
          wordclass = CL_DISCRETE;
          break;
        case 'S':
          wordclass = CL_SUBSTANCE;
          break;
        case 'A':
          wordclass = CL_ACTOR;
          break;
        case 'P':
          wordclass = CL_PROPERTY;
          break;
        default:
          fprintf(stderr, "Dictionary contains bogus extended entry for [%s]\n", buffer);
          return NULL;
          break;
      }
      
      strcpy(w1, trans+2);

      switch (wordclass) {
        case CL_DISCRETE:

          sprintf(buffer, "%s%1dn", w, place);
          trans = translate(buffer);
          if (trans) {
            strcpy(w1n, trans);
          } else {
            w1n[0] = 0;
          }

          switch (ctx) {
            case TCX_NOUN:
              strcpy(result, make_plural(w1));
              break;
            case TCX_VERB:
              if (*w1n) {
                sprintf(result, "being %s", w1n);
              } else {
                sprintf(result, "being %s", make_plural(w1));
              }
              break;
            case TCX_QUAL:
              strcpy(result, w1);
              break;
            case TCX_TAG:
              if (*w1n) {
                strcpy(result, w1n);
              } else {
                strcpy(result, make_plural(w1));
              }
              break;
          }

          break;

        case CL_SUBSTANCE:

          sprintf(buffer, "%s%1dn", w, place);
          trans = translate(buffer);
          if (trans) {
            strcpy(w1n, trans);
          } else {
            w1n[0] = 0;
          }

          switch (ctx) {
            case TCX_NOUN:
              strcpy(result, w1);
              break;
            case TCX_VERB:
              if (*w1n) {
                sprintf(result, "being %s", w1n);
              } else {
                sprintf(result, "being %s", w1);
              }
              break;
            case TCX_QUAL:
              strcpy(result, w1);
              break;
            case TCX_TAG:
              if (*w1n) {
                strcpy(result, w1n);
              } else {
                strcpy(result, w1);
              }
              break;
          }
          break;

        case CL_ACTOR:
          switch (ctx) {
            case TCX_NOUN:
            case TCX_TAG:
              strcpy(result, append_er(w1));
              break;
            case TCX_VERB:
            case TCX_QUAL:
              strcpy(result, append_ing(w1));
              break;
          }
          break;

        case CL_PROPERTY:
          switch (ctx) {
            case TCX_NOUN:
              sprintf(result, "%s thing", w1);
              break;
            case TCX_VERB:
              sprintf(result, "being %s", w1);
              break;
            case TCX_QUAL:
              strcpy(result, w1);
              break;
            case TCX_TAG:
              sprintf(result, "%s thing", w1);
              break;
          }
          break;
      }

      return result;

    } else {
      fprintf(stderr, "No advanced entry for [%s]\n", buffer);
      /* Not an advanced entry, we have to just return the word as-is */
      return trans;
    }
  } else {
    /* If we can't get any place-dependent translation, don't bother -
       the gismu headword entry is probably misleading and does more
       harm than good. */
    fprintf(stderr, "No advanced entry for [%s]\n", buffer);
    trans = translate(w);
    if (trans) {
      strcpy(result, trans);
      strcat(result, "??");
      return result;
    } else {
      trans = translate_unknown(w);
      if (trans) {
        strcpy(result, trans);
        strcat(result, "??");
        return result;
      } else {
        strcpy(result, "??");
        return result;
      }
    }
  }
}
