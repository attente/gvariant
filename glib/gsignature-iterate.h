/*
 * Copyright © 2007, 2008 Ryan Lortie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * See the included COPYING file for more information.
 */

#ifndef _gsignature_iterate_h_
#define _gsignature_iterate_h_

#define g_signature_iterate_start(signature) \
  G_STMT_START {                                                \
    const char *signature##_str = (const char *) signature;     \
    int bracketed = 0;                                          \
                                                                \
    while (*signature##_str == 'a' ||                           \
           *signature##_str == 'm')                             \
      signature##_str++;                                        \
                                                                \
    do                                                          \
      {                                                         \
        char signature##_char = *(signature##_str++);           \
                                                                \
        switch (signature##_char)                               \
        {                                                       \
          case '{':                                             \
          case '(':                                             \
            bracketed++;                                        \
            break;                                              \
                                                                \
          case '}':                                             \
          case ')':                                             \
            bracketed--;                                        \
            break;                                              \
        }                                                       \
                                                                \
        switch (signature##_char)                               \
        {

#define g_signature_iterate_together(s1, s2) \
  G_STMT_START {                                                \
    const char *s1##_str = (const char *) s1;                   \
    const char *s2##_str = (const char *) s2;                   \
    int bracketed = 0;                                          \
                                                                \
    while (*s1##_str == 'a' || *s1##_str == 'm')                \
      if (*s2##_str++ != *s1##_str++)                           \
        return FALSE;                                           \
                                                                \
    do                                                          \
      {                                                         \
        char s1##_char = *s1##_str++;                           \
        char s2##_char = *s2##_str++;                           \
                                                                \
        switch (s1##_char)                                      \
        {                                                       \
          case '{':                                             \
          case '(':                                             \
            bracketed++;                                        \
            break;                                              \
                                                                \
          case '}':                                             \
          case ')':                                             \
            bracketed--;                                        \
            break;                                              \
        }                                                       \
                                                                \
        switch (s1##_char)                                      \
        {                                                       \
          default:                                              \
            if (s1##_char != s2##_char)                         \
              return FALSE;                                     \
          break;

#define g_signature_iterate_end \
        }                                                       \
      }                                                         \
    while (bracketed);                                          \
  } G_STMT_END

#define g_signature_iterate_return(signature) \
        }                                                       \
      }                                                         \
    while (bracketed);                                          \
                                                                \
    return signature##_str;                                     \
  } G_STMT_END

#endif /* _gsignature_iterate_h_ */
