/*
 * "executor/anno_tuple.h"
 */
#ifndef ANNO_TUPLE_H
#define ANNO_TUPLE_H

#include "postgres.h"
#include "c.h"
#include "nodes/pg_list.h" /*list operation!*/
#include "utils/hsearch.h" /*hash table */
//#include  "utils/tuplestore.h"

#define Max_Anno_num 1024
#define MAx_Summary_method_num 10


/*Annotation summary_result data*/
typedef struct A_sm_r_data{
    char * summary_type;
    char * summary_id;
    char * result;
}A_sm_r_data;

typedef A_sm_r_data * A_sm_r;

/*
 * annotation_table data structure
 */
typedef struct Anno_table_data{
    char * rID;
    char * value;
    char * timestamp;
    char * author;/*all in same table,ID is primary key */
}Anno_table_data;
typedef Anno_table_data * Anno_table;

/*
 * annotation Datat_anno data struture
 */
typedef struct Data_anno_data{
    char * tuple_id;
    char * table_name;
    char * tuple_columns;
    List * tuple_column_list;
}Data_anno_data;
typedef Data_anno_data * Data_anno;

/*
 * single annotation structure
 *content: anno data
 *desc: Data_anno
 *a_sm_r_list: sumamry_result_list(contatin one or more summary_result)
 */

typedef struct Anno_data{
    Anno_table content;
    char * resdes;/*information about summary_list*/
    Data_anno desc; /*information about data_anno*/
    List * a_sm_r_list; /*annotation_summary_result_list*/
}Anno_data;
typedef Anno_data * Anno;

/*
 *elements structure
 *for classifier,cluster: is the label information:snippet:only the sequences information
 *anno_IDs:after filtered the anno_IDs (identifiers for all annotations)
 */

typedef struct ElementsData{
    char * result;
    List * anno_IDs; /*only store anno ids*/
}ElementsData;
typedef ElementsData * Elements;

/*
 *single rep Data based on the summary_ID
 *for each summary methods,each has one rep data
 *value: representation content
 *Summary_type:enum{1,2,3}
 *elements_list: store list of elements:elements.
 */
typedef struct SingleRepData{
    char * value;
    char * summary_type;
    char * summary_ID;
    List * elements_list; /*store List of elements: elements(result:IDList)*/
}SingleRepData;
typedef SingleRepData* SingleRep;


typedef struct Summary_tuple_Data{
    List * anno_list;
    List * reps;
}Summary_tuple_Data;

typedef Summary_tuple_Data * Summary_tuple;


typedef struct AnnoHashkey{
	char id[1024];
}AnnoHashkey;

typedef struct AnnoHashElem{
	AnnoHashkey key;
	List * a_sm_r_list;/*a_sm_r_list*/
}AnnoHashElem;





/*interface*/
extern void init_anno_list(Summary_tuple st,char * src);
extern void init_reps_from_anno_list(Summary_tuple st);
extern char * Represtation(Summary_tuple st);
extern void travel_summary(Summary_tuple st);
extern void filter(Summary_tuple st, List * attrl);
extern void summaryHashTable_init();
extern void insertHash(char * value);
extern List * searchHash(char * value);
extern void fillsummary(Summary_tuple st);
extern void initSummaryTupleStruce(Summary_tuple st);



extern void SelfExecuteQuery(const char *query);
/*propagation mode*/
extern char * propagation_mode; // add by dongqing xiao
extern int mode;
extern HTAB * summaryhashtable;
#endif
