/*
 * cachedResult.h
 *
 *  Created on: Dec 2, 2013
 *      Author: dongqingxiao
 *
 * location: executor/cachedResult.h
 */

#ifndef CACHEDRESULT_H_
#define CACHEDRESULT_H_

#include "postgres.h"
#include "c.h"
#include "utils/tuplestore.h"
#include "executor/tuptable.h"
#include "executor/tuptable.h"
#include "parser/sql_translator.h"

typedef struct PhyiscalString{
	char values[1024];
}PhyiscalString;

typedef struct PhysicalBigString{
	char values[10240];
}PhysicalBigString;

typedef struct PhysicalAnnotation{
	char content[256];
	char summaryResult[256]; /*all the summary result*/
}PhysicalAnnotation;

typedef struct PhysicalRep{
	char summaryInstance[10];
	char summaryResult[10];
	char annotationList[10];
} PhysicalRep;

typedef struct PhysicalTuple{
	PhysicalBigString annovalue;
	PhyiscalString attrvalue;
}PhysicalTuple;



typedef struct CachedResultElementData{
	PhysicalTuple tuples[10];
	int qid;
	int numTuple;
	PhyiscalString  testValue;
}CachedResultElementData;


typedef CachedResultElementData * CachedResultElement;



extern void cachedElementInit();
extern void printCachedLength();
extern void putTupleIntoCache(TupleTableSlot *slot);
extern void queryCachedElement(int qid);
extern void zoomInexecutor(ZoomInCmd zcmd);
extern void reportQID();


#endif /* CACHEDRESULT_H_ */
