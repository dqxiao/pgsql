/*
 * parser/sql_translator.h
 */
#ifndef SQL_TRANSLATOR_H
#define SQL_TRANSLATOR_H

#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"

#define max_len 6028
#define DATA_ANNO "data_anno"
#define ANNO_TABLE "anno_table"
#define SUMMARY_RESULT "summary_result"
#define SRANNO "SRanno"
#define SR_clause "SRanno as (select id, string_agg('summary_method:'|| summary_method ||'-' || 'result:' ||result, '|' ) as resdes from summary_result group by id order by id )"

typedef struct RAttrno{
    List * attrnos;
}RAttrno;

typedef struct ArtCmd{
	char * ANS_clause;
	char * bs_clause; // before select_clause
	char * bf_clause; // before end_from_clause;
	char * old_where_clause; // old_where_clause, can be null
	char * rc_clause; // remaining_clause;
	List * additional_with_clauses; // for annotation creating

	List * relnames;    // target query name
	char * real_query; // final real query
	bool  targetstar; //
	bool withclause;  // with original with clause
	bool groupclause; // whether there is group
	RAttrno * attrnos;
} ArtCmd;



typedef struct Minrel{
    char * relname;
    char * oldrep;
    char * changerep;
}Minrel;




typedef struct ZoomInCmdData{
	int qid; /*qid*/
	char whereClause[124];
	char instanceID[24];
	char summaryResult[6];
	bool allTuple;
	bool allAnno;
}ZoomInCmdData;


typedef struct ParsedClause{
	char * attributeName;
	char * op;
	char * value;
}ParsedClause;

typedef struct PhysicalAttribute{
	char * attributesName;
	char * value;
}PhysicalAttribute;

typedef ZoomInCmdData *  ZoomInCmd;

#define SQLtranslate
#ifdef SQLtranslate
#define SQL_printf(s) printf("translator:%s",s)
#endif

// new added one
extern char * addannotation_lazy_mode;
extern char * delegated_mode;
//extern bool execption;




extern char * translator(const char *query_string,int mode);
extern int typeofQuery(const char * query_string);
extern char * generalTranslator(const char * query_string);
extern void zoomCmdParser(const char * query_string, ZoomInCmd zcmd);
extern void initZoomInCmd(ZoomInCmd zcmd);
extern void clauseParser(char * whereClause,ParsedClause * pc);
extern void attributeParser(char * attributeStr,PhysicalAttribute *pa);


#endif
