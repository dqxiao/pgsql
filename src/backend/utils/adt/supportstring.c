#include "postgres.h"

#include <ctype.h>
#include "utils/supportstring.h"


int
find( const char* lpszSource, const char* lpszSearch )
{
	const char* cSource = lpszSource;
	const char* cSearch = lpszSearch;
    if ( ( NULL == lpszSource ) || ( NULL == lpszSearch ) ) return -1;

    for ( ; ; )
    {
        if ( 0 == *lpszSearch )
        {
            return (int)(lpszSource - ( lpszSearch - cSearch ) - cSource );
        }
        if ( '\0' == *lpszSource ) return -1;
        if ( *lpszSource != *lpszSearch )
        {
            lpszSource -= lpszSearch - cSearch - 1;
            lpszSearch = cSearch;
            continue;
        }
        ++ lpszSource;
        ++ lpszSearch;
    }
    return -1;
}

char *
get_substr(char *src, int start, int end ){
	//char result[1024];

    char *substr=NULL;
    int j;
    int i=0;
    if(start>=0&& end<= strlen(src) && end>=start){
        substr=malloc((end-start+2)*sizeof(char));
        j=0;
        for(i=start;i<=end;i++){
            substr[j++]=*(src+i);
        }

        substr[j]='\0';

    }
    //strcpy(result,substr);
    return substr;
    //return result;
}

int
split(char ** dest,char * src, char * sepator)
{
	int end,pend;
	int start=0;
    int num=0;
    char * p=src;
    for(;;){
        end=find(p,sepator);
        if(end==-1){
            break;
        }
        else{
            pend=start+end-1;
            dest[num++]=get_substr(p, start, pend);
            p+=end+strlen(sepator);
            if(p=='\0'){
                break;
            }
        }
    }
    if(p!='\0'){
        dest[num++]=p;
    }
    return num;
}

char *
find_attribute(char *src,char * attribute_name,char *end_mark){
    int start=find(src,attribute_name);
    int length=(int)strlen(attribute_name);
    if(start==-1){
        return NULL;
    }
    else{
        int end=find(src+start,end_mark);
        char *s=(char *)malloc(end*sizeof(char));
        if(end!=-1){
            s=get_substr(src,start+length+1, start+end-1);
        }
        else{
            // last one
           if(start+length+1==strlen(src)) return NULL;
           s=get_substr(src,start+length+1, strlen(src));
        }


        return s;
    }
}
char *
add_blank(char * command){
    char *s=malloc(1024*sizeof(char));
    strcpy(s,command);
    if(*(s+strlen(command)-2)!=' '){
        *(s+strlen(command)-1)=' ';
        *(s+strlen(command))=';';
        s[strlen(command)+1]='\0';
    }
    else{
        return command;
    }
    return s;

}

int
get_first_word_location(char * src, char * target){
    size_t location=0;
    bool flag=false;
    char *s=(char *) malloc(64*sizeof(char));

    char * start=src;

    while(src&&sscanf(src,"%s",s)==1&&(*src!=';')){
        int cur_len=(int) strlen(s);
        s[cur_len]='\0';


        if(strcmp(s,target)==0){
            flag=true;
            break;
        }
        src+=cur_len;
        src++;
    }
    if(!flag) return -1;

    location=src-start;
    return (int)location;
};

int
get_last_word_location(char * src,char * target){
    size_t location=0;
    bool flag=false;
    char *s=(char *) malloc(64*sizeof(char));

    char * start=src;
    char * last_end=src;
    while(src&&sscanf(src,"%s",s)==1&&(*src!=';')){
        int cur_len=(int) strlen(s);
        s[cur_len]='\0';


        if(strcmp(s,target)==0){
            flag=true;
            last_end=src;
        }
        src+=cur_len;
        src++;
    }
    if(!flag) return -1;

    location=last_end-start;
    return (int)location;
}

void
erase_word(char * src,char * target){
    int location=find(src,target);
    int length=strlen(target);
    if(length==0) return;
    if(location==-1) return ;
    else{
        char * temp=malloc(strlen(src)*sizeof(char));
        temp=strcat(temp,get_substr(src,0,location));
        temp=strcat(temp,get_substr(src,location+length,strlen(src)));
        src=temp;
    }
}

char *
strip_const(const char * old){
	char * new=malloc((strlen(old)+1)*sizeof(char));
	strcpy(new,old);
	return new;
}




char *
replace(const char *s, char ch, const char *repl) {
    int count = 0;
    const char *t;
    for(t=s; *t; t++)
        count += (*t == ch);

    size_t rlen = strlen(repl);
    char *res =(char *) malloc(strlen(s) + (rlen-1)*count + 1);
    char *ptr = res;
    for(t=s; *t; t++) {
        if(*t == ch) {
            memcpy(ptr, repl, rlen);
            ptr += rlen;
        } else {
            *ptr++ = *t;
        }
    }
    *ptr = 0;
    return res;
}


void
strip(char * s){
	char *p2 = s;
	    while(*s != '\0') {
	    	if(*s != '\t' && *s != '\n' && *s!=' ') {
	    		*p2++ = *s++;
	    	} else {
	    		++s;
	    	}
	    }
	    *p2 = '\0';
}
