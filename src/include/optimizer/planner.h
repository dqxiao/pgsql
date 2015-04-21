/*-------------------------------------------------------------------------
 *
 * planner.h
 *	  prototypes for planner.c.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/optimizer/planner.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANNER_H
#define PLANNER_H

#include "nodes/plannodes.h"
#include "nodes/pg_list.h"
#include "nodes/relation.h"


/* Hook for plugins to get control in planner() */
typedef PlannedStmt *(*planner_hook_type) (Query *parse,
													   int cursorOptions,
												  ParamListInfo boundParams);
extern PGDLLIMPORT planner_hook_type planner_hook;


extern PlannedStmt *planner(Query *parse, int cursorOptions,
		ParamListInfo boundParams);
extern PlannedStmt *standard_planner(Query *parse, int cursorOptions,
				 ParamListInfo boundParams);

extern Plan *subquery_planner(PlannerGlobal *glob, Query *parse,
				 PlannerInfo *parent_root,
				 bool hasRecursion, double tuple_fraction,
				 PlannerInfo **subroot);

extern Expr *expression_planner(Expr *expr);

extern bool plan_cluster_use_sort(Oid tableOid, Oid indexOid);

extern void check_modify_plan_list(List * plantree_list); // add by dongqing xiao
extern void check_modify_remove_plan_node(Plan * plantree);// add by dongqing xiao for explain
extern void plan_from_query(char * query,PlannedStmt * stmt); // add by dongqing for further modify ..

extern void check_modify_remove_plannedstmt(PlannedStmt * stmt); // add by dongqing xiao for explain
#endif   /* PLANNER_H */
