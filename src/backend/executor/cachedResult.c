/*
 * cachedResult.c
 *
 *  Created on: Dec 2, 2013
 *      Author: dongqingxiao
 */
#include "postgres.h"
#include "c.h"
#include "utils/tuplestore.h" /*for the tuplestore information*/
#include "nodes/pg_list.h" /*list operation!*/
#include "nodes/nodes.h"
#include "utils/builtins.h" /*for reading!*/
#include "utils/supportstring.h"
#include "utils/datum.h" // for copying
#include "executor/tuptable.h"
#include "executor/cachedResult.h"
#include "nodes/nodes.h"
#include "utils/palloc.h"
#include "nodes/memnodes.h"
#include "utils/memutils.h"
#include "executor/anno_tuple.h"
#include "access/printtup.h"
#include "utils/palloc.h"
#include "utils/elog.h"



//static List * cachedList=(List *) NULL;
static CachedResultElementData cachedData[10];
static int QID=1;

static void travelCachedElement(CachedResultElement cre);

static List* filterSummaryTuple(Summary_tuple tts_summary,ZoomInCmd zcmd);
static char * inListArray(List * l);


void reportQID(){

	//elog(INFO,"current QID:%d ",QID-1);
}


char *
inListArray(List *l){
	ListCell * lf;
	char * astring=(char *)malloc(128*sizeof(char));

	memset(astring,0,128);
	foreach(lf,l){
		       	char * numstr=lfirst(lf);
		       	printf("numstr: %s\n",numstr);
		        strcat(astring,numstr);
		        strcat(astring,",");
		    }
	astring=get_substr(astring,0,strlen(astring)-2);

	printf("astring:%s#\n",astring);
	return astring;


}

List *
filterSummaryTuple(Summary_tuple tts_summary,ZoomInCmd zcmd){
	List * finalResult=(List *)NULL;

	bool allFlag=zcmd->allAnno;

	List * reps=tts_summary->reps;
	ListCell * lf;

	printf("filter Summary Tuple\n");
	foreach(lf,reps){
		/**/
		SingleRep rep=(SingleRep)lfirst(lf);
		List * elementList;
		elementList=rep->elements_list;
		if(allFlag)
		{

			printf("all\n");
			printf("rep->summmaryID:%s \n",rep->summary_ID);
			printf("rep->result:%s \n",rep->value);

			List * felementList=rep->elements_list;
			ListCell * lff;

			foreach(lff,felementList){
				Elements el=(Elements)lfirst(lff);
				finalResult=list_union(finalResult,el->anno_IDs);

			}


		}else{
			printf("rep->summmaryID:%s#\n",rep->summary_ID);
			printf("rep->result:%s#\n",rep->value);
			printf("zcmd->instanceID:%s#\n",zcmd->instanceID);
			printf("zcmd->summaryResult:%s#\n",zcmd->summaryResult);

			if(strcmp(rep->summary_ID,zcmd->instanceID)==0){

				List * felementList=rep->elements_list;

				ListCell * lff;
				foreach(lff,felementList){
					Elements el=(Elements)lfirst(lff);
					printf("el->result:%s# \n",el->result);
					if(strcmp(el->result,zcmd->summaryResult)==0){
						finalResult=list_union(finalResult,el->anno_IDs);
						break;
					}

				}

			}

		}
	}


	return finalResult;

}


void
cachedElementInit(){


	printf("QID:%d \n",QID);

	if(QID<10){

		cachedData[QID].qid=QID;
		cachedData[QID].numTuple=0; /*initiate the number of tuples*/
	}




	QID++;

}

void
pushSlotInto(TupleTableSlot *slot,PhysicalTuple * ptuple){

	if(slot->tts_summary_value){
		strcpy(ptuple->annovalue.values,slot->tts_summary_value);
	}
	strcpy(ptuple->attrvalue.values,getAttrsStr(slot));

	/**/
}

void
putTupleIntoCache(TupleTableSlot *slot){

		int num=cachedData[QID-1].numTuple;

		strcpy(cachedData[QID-1].testValue.values,"test string");

		if(slot->tts_summary_value){
			printf("tts_summary_value:%s \n",slot->tts_summary_value);
		}
		pushSlotInto(slot,&(cachedData[QID-1].tuples[num]));
		cachedData[QID-1].numTuple++;

}

void
printCachedLength(){


}


void
travelCachedElement(CachedResultElement cre){

	/**/
	int number=cre->numTuple;

	int i;

	for(i=0;i<number;i++){

		printf("annotation attributes: %s \n",cre->tuples[i].annovalue.values);
		printf("attributes value: %s \n",cre->tuples[i].attrvalue.values);
	}
}


void
filterCachedElement(CachedResultElement cre,ZoomInCmd zcmd){


	if(zcmd->allTuple!=1){
		printf("travel not all tuples \n");

		ParsedClause * pc=(ParsedClause *) palloc(sizeof(ParsedClause));
		clauseParser(zcmd->whereClause,pc);

		char **dest;
		int idx;
		dest=malloc(10*sizeof(char *));

		for(idx=0;idx<cre->numTuple;idx++){
			char * attrsValues=cre->tuples[idx].attrvalue.values;
			char * annoValues=cre->tuples[idx].annovalue.values;
			printf("attrsvaluestr :%s \n",attrsValues);

			int attrnum=split(dest,attrsValues,"||");
			int attridx;
			PhysicalAttribute * pa=(PhysicalAttribute *)palloc(sizeof(PhysicalAttribute));

			printf("we have %d attributes \n",attrnum);
			for(attridx=0;attridx<attrnum-1;attridx++){

				attributeParser(dest[attridx],pa);

				if(strcmp(pa->attributesName,pc->attributeName)==0){
					printf("at least attribute name \n");
					if(strcmp(pa->value,pc->value)==0){
						printf("we should keep\n");
						printf("keep attrsValue :%s \n",attrsValues);
						printf("keep annoValue :%s \n",annoValues);
						Summary_tuple tts_summary;
						tts_summary=palloc(sizeof(Summary_tuple_Data));
						initSummaryTupleStruce(tts_summary);
						init_anno_list(tts_summary,annoValues);
						/*we need filter out the real information we need*/

						init_reps_from_anno_list(tts_summary);
						List * alist=filterSummaryTuple(tts_summary,zcmd);
						char * idStr=inListArray(alist);
						printf("inlistArray:%s \n",inListArray(alist));
						char * sqlquery=(char *)malloc(256*sizeof(char));
						char * formate="select * from anno_table where id in (%s)";

						sprintf(sqlquery,formate,idStr);

						printf("sqlquery:%s#\n",sqlquery);

						//
						SelfExecuteQuery(sqlquery);

						break;
					}
					break;
				}

			}

		}



	}
	else{
		int idx;
		for(idx=0;idx<cre->numTuple;idx++){
			char * attrsValues=cre->tuples[idx].attrvalue.values;
			char * annoValues=cre->tuples[idx].annovalue.values;

			Summary_tuple tts_summary;
			tts_summary=palloc(sizeof(Summary_tuple_Data));
			initSummaryTupleStruce(tts_summary);
			init_anno_list(tts_summary,annoValues);
			/*we need filter out the real information we need*/

			init_reps_from_anno_list(tts_summary);
			List * alist=filterSummaryTuple(tts_summary,zcmd);
			char * idStr=inListArray(alist);
			printf("inlistArray:%s \n",inListArray(alist));
			char * sqlquery=(char *)malloc(256*sizeof(char));
			char * formate="select * from anno_table where id in (%s)";

			sprintf(sqlquery,formate,idStr);

			printf("sqlquery:%s#\n",sqlquery);

			//
			SelfExecuteQuery(sqlquery);


		}

	}
}

void
queryCachedElement(int qid)
{

	printf("call query CachedElement \n");

	if(qid>QID){
		printf("sorry,it is out of range \n");
	}
	else{


		int number=cachedData[qid].numTuple;
		travelCachedElement(&cachedData[qid]);
		}


}

void
zoomInexecutor(ZoomInCmd zcmd){

	printf("zoom in executor \n");
	if(zcmd->qid<QID){
		filterCachedElement(&cachedData[zcmd->qid],zcmd);
	}else{
		elog(INFO,"out of range \n");
	}

}
