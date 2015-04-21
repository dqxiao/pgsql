/*-------------------------------------------------------------------------
 *
 * printtup.h
 *
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/printtup.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRINTTUP_H
#define PRINTTUP_H

#include "utils/portal.h"
#include "postgres.h"
#include "utils/datum.h"

extern DestReceiver *printtup_create_DR(CommandDest dest);

extern void SetRemoteDestReceiverParams(DestReceiver *self, Portal portal);

extern void SendRowDescriptionMessage(TupleDesc typeinfo, List *targetlist,
						  int16 *formats);

extern void debugStartup(DestReceiver *self, int operation,
			 TupleDesc typeinfo);
extern void debugtup(TupleTableSlot *slot, DestReceiver *self);
extern void debugtuple(TupleTableSlot *slot);
extern void Ptypedebugtuple(TupleTableSlot * 	slot, TupleTableSlot * 	aslot);
extern void UnionAnnotuple(TupleTableSlot * 	slot,List * ex_anno);
/* XXX these are really in executor/spi.c */
extern void spi_dest_startup(DestReceiver *self, int operation,
				 TupleDesc typeinfo);
extern void spi_printtup(TupleTableSlot *slot, DestReceiver *self);

extern char * getannossummary(TupleTableSlot *slot);

// add by
extern void print_datum(Datum value, bool typByVal, int typLen);
extern  char * getAttrsStr(TupleTableSlot *slot);

#endif   /* PRINTTUP_H */
