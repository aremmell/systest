#ifndef _SYSTEST_MACROS_H_INCLUDED
#define _SYSTEST_MACROS_H_INCLUDED

//
// Terminal colors
//
#define SYSTEST_ESC_START "\x1b["
#define SYSTEST_ESC_END   "m"
#define SYSTEST_ESC_RESET SYSTEST_ESC_START "0" SYSTEST_ESC_END

/** A couple fun characters that can be sent to stdio. */
#define SYSTEST_R_ARROW "\xe2\x86\x92"
#define SYSTEST_L_ARROW "\xe2\x86\x90"
#define SYSTEST_BULLET  "\xe2\x80\xa2"

/** macros for use with printf and friends. */
#define _ESCSEQ(codes, s) SYSTEST_ESC_START codes SYSTEST_ESC_END s SYSTEST_ESC_RESET
#define _CLR(attr, fg, s) _ESCSEQ(#attr ";" #fg, s)

#define ULINE(s) _ESCSEQ("4", s)
#define EMPH(s)  _ESCSEQ("3", s)
#define BOLD(s)  _ESCSEQ("1", s)

#define RED(s)  _CLR(0, 31, s)
#define REDB(s) _CLR(1,31, s)
#define REDD(s) _CLR(2,31, s)

#define LRED(s)  _CLR(0, 91, s)
#define LREDB(s) _CLR(1, 91, s)
#define LREDD(s) _CLR(2, 91, s)

#define GREEN(s)  _CLR(0, 32, s)
#define GREENB(s) _CLR(1, 32, s)
#define GREEND(s) _CLR(2, 32, s)

#define LGREEN(s)  _CLR(0, 92, s)
#define LGREENB(s) _CLR(1, 92, s)
#define LGREEND(s) _CLR(1, 92, s)

#define YELLOW(s)  _CLR(0, 33, s)
#define YELLOWB(s) _CLR(1, 33, s)
#define YELLOWD(s) _CLR(2, 33, s)

#define LYELLOW(s)  _CLR(0, 93, s)
#define LYELLOWB(s) _CLR(1, 93, s)
#define LYELLOWD(s) _CLR(2, 93, s)

#define BLUE(s)  _CLR(0, 34, s)
#define BLUEB(s) _CLR(1, 34, s)
#define BLUED(s) _CLR(2, 34, s)

#define LBLUE(s)  _CLR(0, 94, s)
#define LBLUEB(s) _CLR(1, 94, s)
#define LBLUED(s) _CLR(2, 94, s)

#define MAGENTA(s)  _CLR(0, 35, s)
#define MAGENTAB(s) _CLR(1, 35, s)
#define MAGENTAD(s) _CLR(2, 35, s)

#define LMAGENTA(s)  _CLR(0, 95, s)
#define LMAGENTAB(s) _CLR(1, 95, s)
#define LMAGENTAD(s) _CLR(2, 95, s)

#define CYAN(s)  _CLR(0, 36, s)
#define CYANB(s) _CLR(1, 36, s)
#define CYAND(s) _CLR(2, 36, s)

#define LCYAN(s)  _CLR(0, 96, s)
#define LCYANB(s) _CLR(1, 96, s)
#define LCYAND(s) _CLR(2, 96, s)

#define LGRAY(s)  _CLR(0, 37, s)
#define LGRAYB(s) _CLR(1, 37, s)
#define LGRAYD(s) _CLR(2, 37, s)

#define DGRAY(s)  _CLR(0, 90, s)
#define DGRAYB(s) _CLR(1, 90, s)
#define DGRAYD(s) _CLR(2, 90, s)

#define WHITE(s)  _CLR(0, 97, s)
#define WHITEB(s) _CLR(1, 97, s)
#define WHITED(s) _CLR(2, 97, s)

//
// Misc.
//
#define bool_to_str(b) (b ? "true" : "false")
#define prn_str(str) (str ? str : "NULL")
#define _validstr(str) (NULL != str && '\0' != *str)
#define _validptr(p) (NULL != p)

#endif // ! _SYSTEST_MACROS_H_INCLUDED
