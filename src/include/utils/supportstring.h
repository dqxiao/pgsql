/*support string.h
 * utils/supportstring.h
 *something string function for further usage!
 */

#ifndef SUPPORTSTRING_H
#define SUPPORTSTRING_H


extern int find(const char* lpszSource, const char* lpszSearch );

extern char * get_substr(char *src,int start,int end);

extern int split(char **dest,char *src, char * sepator);

extern char * find_attribute(char *src,char * attribute_name,char *end_mark);

extern char * add_blank(char * command);

extern int get_first_word_location(char * src, char * target);
extern int get_last_word_location(char * src,char * target);

extern void erase_word(char * src,char * target);

extern char * strip_const(const char * old);

extern char *replace(const char *s, char ch, const char *repl);

extern void strip(char * s);


#endif
