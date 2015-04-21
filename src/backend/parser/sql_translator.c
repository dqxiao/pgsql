/*
 * implementation of parser/sql_translator.h
 */
#include "postgres.h"
#include "nodes/pg_list.h"
#include "nodes/readfuncs.h"
#include "nodes/parsenodes.h"
#include "nodes/primnodes.h"
#include "parser/parser.h"
#include "parser/parsetree.h"
#include "parser/analyze.h"
#include "tcop/tcopprot.h"

#include "parser/sql_translator.h"
#include "utils/builtins.h"
#include "utils/supportstring.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"

static char *simpletranslator(const char *query_string,int mode);
static char * standardCompound(const char * query_string);
static char * summaryCompound(const char * query_string);
static char * hybridCompound(const char * query_string);

/*support function*/
static void getAttrno(Query * query, ArtCmd * artCmd);
static void sgetAttrno(List * tl,List * rtable,ArtCmd * artCmd);

static char * extraQuery(const char * query_string);
static bool targetstar(SelectStmt * stmt,char * old_cmd);
static int get_first_word_location_with_start(char * src,char * target,int start);
static int get_last_word_location_with_end(char * src, char * target, int end);
static int get_from_last_index(SelectStmt * stmt);
static int get_from_first_index(SelectStmt * stmt);
//static char * add_summary_anno_table(List * relnames);
static char * add_special_anno_table(char * table_name,ArtCmd * artCmd, int i);
static char * add_simple_anno_table(char * table_name,ArtCmd * artCmd, int i);
static char * add_anno_table(char * table_name,ArtCmd * artCmd,int i,int mode);
static char * add_summary_anno_table(List * relnames,RAttrno * ras);
void getRels(SelectStmt *stmt, ArtCmd * artCmd,int mode);
static void getPartCmd(char *old_cmd,SelectStmt *stmt,ArtCmd * artCmd,int mode);

void simpleIntegration(ArtCmd * artCmd);
void annosIntegration(ArtCmd * artCmd );

/*string related function*/
static char * listtoarraystring(List * srclist);

/* implementations */
char * dealWithAddAnnotation(const char * query_string);



bool
targetstar(SelectStmt * stmt,char * old_cmd)
{
	List * restarget=stmt->targetList;
	ListCell * list_item;

	if(list_length(restarget)>1) return false;

	foreach(list_item,restarget){
		Node * sp_node=(Node *) lfirst(list_item);
		ResTarget * rt=(ResTarget *)sp_node;
		if(*(old_cmd+rt->location)=='*')
			return true;

	}

	return false;

}

char *
add_summary_anno_table(List * relnames,RAttrno * ras){

    char * asep=(char *) malloc(max_len*sizeof(char));
    char * awhere=(char *) malloc(1024*sizeof(char));
    char * sep=(char *) malloc(64*sizeof(char));

    char * preparedf="ANS as( select tuple_id,string_agg('id:'|| data_anno.aid||'-'||'resdes:'||resdes,'||') as annos from SRanno,data_anno where data_anno.aid=SRanno.id and (%s) group by data_anno.tuple_id) ,";

    ListCell * lf;
    int count=0;
    foreach(lf,relnames){
        Minrel * minrel=(Minrel*)lfirst(lf);
        char * relname=minrel->relname;
        char * formate="(table_name= \'%s\' and %s && tuple_column )  or";
        RAttrno ra=ras[count];
        List * attrs=ra.attrnos;
        sprintf(sep,formate,relname,listtoarraystring(attrs));

        strcat(awhere,sep);
        count+=1;
    }
    awhere=get_substr(awhere,0,strlen(awhere)-4);

    sprintf(asep,preparedf,awhere);

    //printf("new-add one:%s \n",asep);

    return asep;
}

char *
add_delegate_special_anno_table(char *table_name,ArtCmd *artCmd, int i){

	char * sp_anno=(char *)malloc(max_len*sizeof(char));
	char * sp_TD=(char *)malloc(max_len*sizeof(char));
	char * final_anno=(char *)malloc(2*max_len*sizeof(char));

	/**/
	RAttrno * ras=artCmd->attrnos;
	RAttrno ra=ras[i];
	List * attrs=ra.attrnos;

	/*Prepared for the something we need*/

	char * sp_TD_formate="%s_TDanno as ( select id,aid,tuple_id,table_name,array_to_string(tuple_column,',')as tuple_columns from %s where  %s.table_name=\'%s\' and %s && tuple_column ) ";

	char * sp_formate="%s_anno as ( select tuple_id, string_agg('id:'||%s_TDanno.aid||'--'||'table_name:'||table_name||'--'||'tuple_id:'|| tuple_id|| '--'||'tuple_columns:'||tuple_columns||'--'||'resdes:'||resdes,'||') as td_sr from %s_TDanno,%s where %s_TDanno.aid=%s.id group by tuple_id )";


	/*create the real query*/

	sprintf(sp_TD,sp_TD_formate,table_name,DATA_ANNO,DATA_ANNO,table_name,listtoarraystring(attrs));

	sprintf(sp_anno,sp_formate,table_name,table_name,table_name,SRANNO,table_name,SRANNO);

	/**/
	strcat(final_anno,sp_TD);
	strcat(final_anno," , ");
	strcat(final_anno,sp_anno);



	return final_anno;

}

char *
add_special_anno_table(char * table_name,ArtCmd * artCmd, int i){
    char *sp_anno=(char *)malloc(max_len*sizeof(char));
    char *sp_TD=malloc(max_len*sizeof(char));
    char *finnal_anno=(char *) malloc(2*max_len*sizeof(char));
    RAttrno * ras=artCmd->attrnos;
    RAttrno ra=ras[i];
    List * attrs=ra.attrnos;

    char * sp_TD_formate="%s_TDanno as ( select %s.*,%s.table_name,tuple_id,aid,array_to_string(tuple_column,',')as tuple_columns from %s,%s where %s.id=%s.id and %s.table_name=\'%s\' and %s && tuple_column ) ";

    char * sp_formate="%s_anno as ( select tuple_id, string_agg('id:'||%s_TDanno.aid||'--'||'author:'||COALESCE(author,'unkown')||'--'||'timestamp:'|| COALESCE(timestamp,'2015-07-12 13:14:51')||'--'||'value:'||value||'--'||'table_name:'||table_name||'--'||'tuple_id:'|| tuple_id|| '--'||'tuple_columns:'||tuple_columns||'--'||'resdes:'||resdes,'||') as td_sr from %s_TDanno,%s where %s_TDanno.aid=%s.id group by tuple_id )";

    //printf("%s for debugging \n",listtoarraystring(attrs));
    sprintf(sp_TD,sp_TD_formate,table_name,ANNO_TABLE,DATA_ANNO,DATA_ANNO,ANNO_TABLE,ANNO_TABLE,DATA_ANNO,DATA_ANNO,table_name, listtoarraystring(attrs));

    //
    sprintf(sp_anno,sp_formate,table_name,table_name,table_name,SRANNO,table_name,SRANNO);


    strcat(finnal_anno,sp_TD);
    strcat(finnal_anno," , ");
    strcat(finnal_anno,sp_anno);


    return finnal_anno;
}

char * add_simple_anno_table(char * table_name,ArtCmd * artCmd, int i){
	char * TD_anno=(char *)malloc(max_len*sizeof(char));
    char * sp_anno=(char *)malloc(max_len*sizeof(char));
    char * final=(char *)malloc(2*max_len*sizeof(char));
	RAttrno * ras=artCmd->attrnos;
	RAttrno ra=ras[i];
	List * attrs=ra.attrnos;

	char * sp_TD_formate="%s_TDanno as ( select %s.*,%s.table_name,aid,tuple_id,array_to_string(tuple_column,',')as tuple_columns from %s,%s where %s.id=%s.id and %s.table_name=\'%s\' and %s && tuple_column ) ";
	sprintf(TD_anno,sp_TD_formate,table_name,ANNO_TABLE,DATA_ANNO,DATA_ANNO,ANNO_TABLE,ANNO_TABLE,DATA_ANNO,DATA_ANNO,table_name, listtoarraystring(attrs));

    char * sp_formate="%s_anno as ( select tuple_id, string_agg('id:'||%s_TDanno.aid||'--'||'author:'||COALESCE(author,'unkown')||'--'||'timestamp:'|| COALESCE(timestamp,'2015-07-12 13:14:51')||'--'||'value:'||value||'--'||'table_name:'||table_name||'--'||'tuple_id:'|| tuple_id|| '--'||'tuple_columns:'||tuple_columns,'||') as td_sr from %s_TDanno group by tuple_id )";
    sprintf(sp_anno,sp_formate,table_name,table_name,table_name);

    strcat(final,TD_anno);
    strcat(final," , ");
    strcat(final,sp_anno);

	return final;

}

char * add_anno_table(char * table_name,ArtCmd * artCmd,int i,int mode)
{
	switch(mode)
	{
		case 2:
			//return add_special_anno_table(table_name,artCmd,i);
			if(strcmp(delegated_mode,"T")==0){
				return add_delegate_special_anno_table(table_name,artCmd,i);
			}else{
				return add_special_anno_table(table_name,artCmd,i);
			}
			break;
		case 3:
			return add_simple_anno_table(table_name,artCmd,i);
			break;
		case 1:
			return add_simple_anno_table(table_name,artCmd,i);
			break;

	}

}


int
get_first_word_location_with_start(char * src,char * target,int start){
    int result=get_first_word_location(src+start, target);
    if(result!=-1){
        return result+start;
    }
    return result;
};

int
get_last_word_location_with_end(char * src, char * target, int end){
    char * temp=malloc(end*sizeof(char));
    int result;
    temp=get_substr(src,0,end);
    result=get_last_word_location(temp,target);
    free(temp);
    return result;
}


int
get_from_last_index(SelectStmt * stmt){

    // get the last target location,
    // any legal select stmt should have at least one target relname
    List * fromclause;
    ListCell * list_item;
    Node * rangel;
    RangeVar * rv;

    fromclause=stmt->fromClause;
    Assert(fromclause!=NULL);
    list_item=list_tail(fromclause);
    Assert(list_item!=NULL);

    rangel=(Node*) lfirst(list_item);
    rv=(RangeVar *) rangel;
    return rv->location;
}

int
get_from_first_index(SelectStmt * stmt){
    List * fromclause;
    ListCell * list_item;
    Node * rangel;
    RangeVar *rv;

    fromclause=stmt->fromClause;


    list_item=list_head(fromclause);
    Assert(list_item!=NULL);

    rangel=(Node*) lfirst(list_item);
    rv=(RangeVar *) rangel;
    return rv->location;

}




char *
listtoarraystring(List * srclist){
    ListCell * lf;
    char * astring=(char *)malloc(128*sizeof(char));
    strcat(astring,"\'{ ");
    foreach(lf,srclist)
    {
        int num;
        char numstr[3];

        num=(int)lfirst(lf);
        sprintf(numstr,"%d",num);
        strcat(astring,numstr);
        strcat(astring,",");
    }
    astring=get_substr(astring,0,strlen(astring)-2);
    strcat(astring,"}\'");

    return astring;
}

/*get attribute numbers
 */
void
sgetAttrno(List * tl,List * rtable,ArtCmd * artCmd){
    ListCell * lf;

    foreach(lf,tl)
    {

        Node * lfn=(Node *)lfirst(lf); // node

        TargetEntry * tle=(TargetEntry *) lfn; // targetentry
        Node * expr=(Node *)tle->expr;

        if(expr && !tle->resjunk)
        {
            //printf("exist not junk expr \n");

            switch(nodeTag(expr))
            {
                case T_Var:
                {
                    Var * var=(Var *) expr;
                    char * relname;
                    char * attname;
                    int varno;
                    int varattno;

                    List * lattrno;

                    RangeTblEntry *rte;
                    Assert(var->varno > 0 &&
                           (int) var->varno <= list_length(rtable));

                    varno=(int)var->varno;
                    varattno=(int)var->varattno;
                    rte = rt_fetch(var->varno, rtable);
                    relname = rte->eref->aliasname;
                    attname = get_rte_attribute_name(rte, var->varattno);

                    //printf("%s.%s \n", relname, attname);
                    //printf("varno:%d,varattno:%d \n",var->varno,var->varattno);


                    lattrno=artCmd->attrnos[varno-1].attrnos;
                    lattrno=lappend_int(lattrno,varattno);

                    artCmd->attrnos[varno-1].attrnos=lattrno;
                    break;
                }
                case T_Aggref:
                {
                    Aggref * aggref=(Aggref *)expr;
                    List * args=aggref->args;
                    sgetAttrno(args,rtable,artCmd);
                    break;
                }
                default:
                    printf("sorry \n");
            }

        }
    }

}

/*
 * mainly for allocate space then get attribute numbers for tables
 */
void
getAttrno(Query * query, ArtCmd * artCmd)
{


    List * rtable;
    List * tl;
    int i;
    RAttrno * rattrno;
    rtable=query->rtable;
    tl=query->targetList;
    artCmd->attrnos=(RAttrno *)malloc(list_length(rtable)*sizeof(RAttrno));

    for(i=0;i<list_length(rtable);i++){
        artCmd->attrnos[i].attrnos=(List *)NULL;
    }

    sgetAttrno(tl,rtable,artCmd);

    rattrno=artCmd->attrnos;
    for(i=0;i<list_length(rtable);i++){
        RAttrno ra=rattrno[i];
        List * atrs=ra.attrnos;
        //printf("atrs' length %d \n",list_length(atrs));
        //printf("intostring %s \n",listtoarraystring(atrs));
    }

}



void
getRels(SelectStmt *stmt, ArtCmd * artCmd,int mode){
	List * fromClause;
	ListCell * list_item;
	int i;
	i=0;
	fromClause=stmt->fromClause;

	artCmd->relnames=(List *)NULL;
	artCmd->additional_with_clauses=(List *)NULL;

	// summary-hash aggragete
	if(list_length(fromClause)!=0 && mode>1){
		artCmd->additional_with_clauses=lappend(artCmd->additional_with_clauses,SR_clause);
	}

    foreach(list_item,fromClause)
    {
        Minrel * minrel=(Minrel *) malloc(sizeof(Minrel));
        Node * rangelist=(Node *) lfirst(list_item);
        RangeVar *rv=(RangeVar *) rangelist;
        char* sp_anno_stmt;

        char * formate="%s_anno";
        char * add_rel=malloc(64*sizeof(char));
        char * leftjoinformate=" %s left join %s on (%s.id=%s.tuple_id)";

        sprintf(add_rel,formate,rv->relname);

        sp_anno_stmt=add_anno_table(rv->relname,artCmd,i,mode); // change one


        minrel->relname=rv->relname;
        minrel->oldrep=rv->relname;

        minrel->changerep=malloc(1024*sizeof(char *));


        if(rv->alias!=NULL)
        {
            char * aliasformate="%s as %s";
            sprintf(minrel->oldrep,aliasformate,rv->relname,rv->alias);

        }

        sprintf(minrel->changerep,leftjoinformate,minrel->oldrep,add_rel,minrel->relname,add_rel);

        artCmd->relnames=lappend(artCmd->relnames,minrel);

        artCmd->additional_with_clauses=lappend(artCmd->additional_with_clauses,sp_anno_stmt);

        i++;
    }
    artCmd->ANS_clause=add_summary_anno_table(artCmd->relnames,artCmd->attrnos);
    //printf("until here \n");
    //SQL_printf("until here");

}

void
getPartCmd(char *old_cmd,SelectStmt *stmt,ArtCmd * artCmd,int mode){
	int sftindex,ssindex;
	int lftindex,efindex,ewindex,bwindex;
	int tempgroup,tempsort;

	getRels(stmt,artCmd,mode);


	/* separate query based on SelectStmt*/

	if(stmt->withClause)
		artCmd->withclause=true;
	else
		artCmd->withclause=false;

	artCmd->targetstar=targetstar(stmt,old_cmd);


	sftindex=get_from_first_index(stmt);
	ssindex=get_last_word_location_with_end(old_cmd,"from",sftindex);
	artCmd->bs_clause=get_substr(old_cmd,0,ssindex-1);

    lftindex=get_from_last_index(stmt);
    efindex=lftindex;
    ewindex=lftindex;
    bwindex=lftindex;
    if(stmt->groupClause){
    	artCmd->groupclause=true;
    }

    if(stmt->whereClause!=NULL){
        artCmd->old_where_clause="";
        efindex=get_first_word_location_with_start(old_cmd,"where",efindex);
        Assert(efindex>=lftindex);
    }else{
        artCmd->old_where_clause=NULL;
        if(stmt->groupClause){
            efindex=get_first_word_location_with_start(old_cmd,"group",efindex);
            //
        }
        else{

            if(stmt->sortClause){
                //elog(INFO,"%s","try to deal with sort Clause");
                efindex=get_first_word_location_with_start(old_cmd,"sort",efindex);
            }
            else{
                /* nothing can be used! get to the end of */
                efindex=strlen(old_cmd)-1;
            }
        }
    }

    artCmd->bf_clause=get_substr(old_cmd,ssindex,efindex-1);
    tempgroup=get_first_word_location_with_start(old_cmd,"group",bwindex);
    tempsort=get_first_word_location_with_start(old_cmd,"sort",bwindex);

    ewindex=efindex;

    if(tempgroup!=-1){
        ewindex=tempgroup;
        //elog(INFO,"get by group:%s",old_cmd+tempgroup);
    }
    if(tempsort!=-1){
        if(tempsort<ewindex)
            ewindex=tempsort;
        //elog(INFO,"get by sort:%s",old_cmd+tempsort);
    }


    if(*(old_cmd+ewindex)==';'){
        ewindex-=1;
    }
    /*here we found the right location of end where clause*/
    //bwindex=efindex;

    if(stmt->whereClause)
    {
        artCmd->old_where_clause=get_substr(old_cmd,efindex,ewindex-1);
    }

    artCmd->rc_clause=get_substr(old_cmd,ewindex,strlen(old_cmd));

    /*until here we fill all we need into arc_cmd*/

    /* waiting for final quit it*/
    elog(INFO,"bs:%s",artCmd->bs_clause);
    elog(INFO,"bf:%s",artCmd->bf_clause);
    elog(INFO,"ow:%s",artCmd->old_where_clause);
    elog(INFO,"rc:%s",artCmd->rc_clause);
}
void
simpleIntegration(ArtCmd * artCmd){
	List * relnames;
	List * additional_with_clauses;
	ListCell * lf;
	bool withclause;
	bool targetstar;
	int relnum;
	char * new_bs;
	char * new_select;
	char * new_from;
	char * new_query;
	char * new_DA;
	char * real_DA;

	relnames=artCmd->relnames;
	additional_with_clauses=artCmd->additional_with_clauses;
	withclause=artCmd->withclause;
	targetstar=artCmd->targetstar;
	relnum=list_length(relnames);

	new_bs=(char *)malloc((relnum+2)*max_len*sizeof(char));
	new_select=(char *)malloc(relnum*128*sizeof(char));
	new_from=(char *)malloc(relnum*256*sizeof(char));
	new_query=(char *)malloc(3*max_len*sizeof(char));
	new_DA=(char *)malloc(max_len*sizeof(char));

	real_DA=(char *)malloc(max_len*sizeof(char));

	new_bs=strcat(new_bs,"with ");
	foreach(lf,additional_with_clauses){
		char * swith=(char *)lfirst(lf);
		strcat(new_bs,swith);
		strcat(new_bs," , ");
	}


	if(withclause){
		erase_word(artCmd->bs_clause,"with");
	}else{
		new_bs=get_substr(new_bs,0,strlen(new_bs)-3); // erase the last one " , "
	}

	if(!artCmd->groupclause)
	{
		// nothing with groupclause
		char * start_DA=" ";
		new_DA=strcat(new_DA,start_DA);
		new_DA=strcat(new_DA,artCmd->bs_clause);

		/*change from course*/
		strcat(new_from," from ");
		foreach(lf,relnames){
			Minrel *sminrel=(Minrel *) lfirst(lf);
			strcat(new_select," , ");
			strcat(new_select,sminrel->relname);
			strcat(new_select,"_anno.*");

			strcat(new_from,sminrel->changerep);
			strcat(new_from," , ");
		}

		new_from=get_substr(new_from,0,strlen(new_from)-3);


		if(!artCmd->targetstar){
			new_DA=strcat(new_DA,new_select);
		}

		/*here deal with from*/
		new_DA=strcat(new_DA,new_from);


		if(artCmd->old_where_clause!=NULL){
			new_DA=strcat(new_DA,artCmd->old_where_clause);
		}

		if(artCmd->rc_clause!=NULL){
			new_DA=strcat(new_DA,artCmd->rc_clause);
		}

	}
	else{

		// with group clause case
		char * new_DAtemp;
		new_DAtemp=(char *)malloc(max_len*sizeof(char));

		strcat(new_DAtemp,", temp_da as (");

		new_DAtemp=strcat(new_DAtemp,"select * ");

		/*change from course*/
		strcat(new_from," from ");
		foreach(lf,relnames){
			Minrel *sminrel=(Minrel *) lfirst(lf);
			strcat(new_select," , ");
			strcat(new_select,sminrel->relname);
			strcat(new_select,"_anno.*");

			strcat(new_from,sminrel->changerep);
			strcat(new_from," , ");
		}


		new_from=get_substr(new_from,0,strlen(new_from)-3);

		/*select * from .. */

		new_DAtemp=strcat(new_DAtemp,new_from);


		if(artCmd->old_where_clause!=NULL){
			new_DAtemp=strcat(new_DAtemp,artCmd->old_where_clause);
		}


		strcat(new_DAtemp," )  ");

		strcat(new_DA,new_DAtemp);
		strcat(real_DA,artCmd->bs_clause);
		strcat(real_DA,",count(*),string_agg(td_sr,'||')"); // April 4 changed
		strcat(real_DA,"from temp_da ");
		strcat(real_DA,artCmd->rc_clause);

		strcat(new_DA,real_DA);
	}
	strcat(new_query,new_bs);
	strcat(new_query,new_DA);

	artCmd->real_query=new_query;

}


void
annosIntegration(ArtCmd * artCmd ){
    List * relnames;
    List * additional_with_clauses;
    ListCell * lf;
    bool withclause;
    bool targetstar;
    int relnum;
    char * new_bs;
    char * new_select;
    char * new_from;
    char * new_query;
    char * new_DA;

    relnames=artCmd->relnames;
    additional_with_clauses=artCmd->additional_with_clauses;
    withclause=artCmd->withclause;
    targetstar=artCmd->targetstar;
    relnum=list_length(relnames);

    new_bs=(char *)malloc((relnum+2)*max_len*sizeof(char));
    new_select=(char *)malloc(relnum*128*sizeof(char));
    new_from=(char *)malloc(relnum*256*sizeof(char));
    new_query=(char *)malloc(3*max_len*sizeof(char));
    new_DA=(char *)malloc(max_len*sizeof(char));



    new_bs=strcat(new_bs,"with ");
    foreach(lf,additional_with_clauses){
        char * swith=(char *)lfirst(lf);
        strcat(new_bs,swith);
        strcat(new_bs," , ");
    }

    new_bs=strcat(new_bs,artCmd->ANS_clause); //add the ANS into with clauses

    if(withclause){
        erase_word(artCmd->bs_clause,"with");
    }else{
        new_bs=get_substr(new_bs,0,strlen(new_bs)-3); // erase the last one " , "
    }


    if(!artCmd->groupclause)
    {
        char * start_DA=", DA as ( ";
        new_DA=strcat(new_DA,start_DA);
        new_DA=strcat(new_DA,artCmd->bs_clause);





        /*change from course*/
        strcat(new_from," from ");
        foreach(lf,relnames){
            Minrel *sminrel=(Minrel *) lfirst(lf);
            strcat(new_select," , ");
            strcat(new_select,sminrel->relname);
            strcat(new_select,"_anno.*");

            strcat(new_from,sminrel->changerep);
            strcat(new_from," , ");
        }

        new_from=get_substr(new_from,0,strlen(new_from)-3);


        if(!artCmd->targetstar){
            //new_query=strcat(new_query,new_select);
            new_DA=strcat(new_DA,new_select);
        }

        /*here deal with from*/
        new_DA=strcat(new_DA,new_from);

        //new_query=strcat(new_query,new_from);

        if(artCmd->old_where_clause!=NULL){
            //new_query=strcat(new_query,artCmd->old_where_clause);
            new_DA=strcat(new_DA,artCmd->old_where_clause);
        }

        if(artCmd->rc_clause!=NULL){
            //new_query=strcat(new_query,artCmd->rc_clause);
            new_DA=strcat(new_DA,artCmd->rc_clause);


        }

        //
        if(find(new_DA,";")!=-1){
            new_DA=get_substr(new_DA,0,strlen(new_DA)-3);
        }

        //erase_word(new_DA," ; ");
        /*until here we finish newDA*/
        strcat(new_DA," ) ");
    }
    else{

        char * new_DAtemp;
        new_DAtemp=(char *)malloc(max_len*sizeof(char));

        strcat(new_DAtemp,", temp_da as (");

        new_DAtemp=strcat(new_DAtemp,"select * ");

        /*change from course*/
        strcat(new_from," from ");
        foreach(lf,relnames){
            Minrel *sminrel=(Minrel *) lfirst(lf);
            strcat(new_select," , ");
            strcat(new_select,sminrel->relname);
            strcat(new_select,"_anno.*");

            strcat(new_from,sminrel->changerep);
            strcat(new_from," , ");
        }


        new_from=get_substr(new_from,0,strlen(new_from)-3);

        /*select * fromm .. */

        new_DAtemp=strcat(new_DAtemp,new_from);


        if(artCmd->old_where_clause!=NULL){
            //new_query=strcat(new_query,artCmd->old_where_clause);
            new_DAtemp=strcat(new_DAtemp,artCmd->old_where_clause);
        }


        strcat(new_DAtemp," ), ");


        //printf("new_DAtemp :%s \n", new_DAtemp);
        /*from here */

        strcat(new_DA,new_DAtemp);

        char * real_DA=(char *)malloc(max_len*sizeof(char));

        strcat(real_DA,"DA as (");

        strcat(real_DA,artCmd->bs_clause);
        strcat(real_DA,"from temp_da ");
        strcat(real_DA,artCmd->rc_clause);

        if(find(real_DA,";")!=-1){
            real_DA=get_substr(real_DA,0,strlen(real_DA)-3);
        }

        strcat(real_DA,") ");

        printf("real_DA:%s\n",real_DA);
        strcat(new_DA,real_DA);
    }
    char * final_one="select * from DA left join ANS on (1=1);";


    strcat(new_query,new_bs);
    strcat(new_query,new_DA);
    strcat(new_query,final_one);


    artCmd->real_query=new_query;

}



char *
standardCompound(const char * query_string){

	// variables
	List * rawParsetreeList;
	ArtCmd * artCmd;
	ListCell * listItem;
	char * query;

	// allocate space for variables
	artCmd=malloc(sizeof(ArtCmd));
	query=strip_const(query_string);

	rawParsetreeList=pg_parse_query(query);

	if(list_length(rawParsetreeList)!=1){
		return query_string;
	}

	/*for each parse tree in list */
	foreach(listItem,rawParsetreeList){
		Node       *parsetree = (Node *) lfirst(listItem);

		if(IsA(parsetree,SelectStmt)){
			Query *pquery;
			SelectStmt * stmt;

			stmt=(SelectStmt *) parsetree;
			query=add_blank(query);
			pquery=parse_analyze(parsetree, query,NULL, 0);

			//SQL_printf("get the pquery");

			/*catch structure information from query tree into artCmd*/
			getAttrno(pquery,artCmd);
			getPartCmd(query,stmt,artCmd,1);
			simpleIntegration(artCmd);
			return artCmd->real_query;
		}
		else{
			// do nothing just return old one
			return query;
		}
	}

	return query;

}


char *
summaryCompound(const char * query_string){

	// variables
	List * rawParsetreeList;
	ArtCmd * artCmd;
	ListCell * listItem;
	char * query;

	// allocate space for variables
	artCmd=malloc(sizeof(ArtCmd));
	query=strip_const(query_string);

	rawParsetreeList=pg_parse_query(query);

	if(list_length(rawParsetreeList)!=1){
		//elog(INFO,"can't deal with multiple querys");
		return query_string;
	}

	/*for each parse tree in list */
	foreach(listItem,rawParsetreeList){
		Node       *parsetree = (Node *) lfirst(listItem);

		if(IsA(parsetree,SelectStmt)){
			Query *pquery;
			SelectStmt * stmt;

			stmt=(SelectStmt *) parsetree;
			query=add_blank(query);
			pquery=parse_analyze(parsetree, query,NULL, 0);

			//SQL_printf("get the pquery");

			/*catch structure information from query tree into artCmd*/
			getAttrno(pquery,artCmd);
			getPartCmd(query,stmt,artCmd,2);
			simpleIntegration(artCmd);
			//printf("%s \n",artCmd->real_query);
			return artCmd->real_query;
		}
		else{
			// do nothing just return old one
			return query;
		}
	}

	return query;

}


char *
hybridCompound(const char * query_string){

	// variables
	List * rawParsetreeList;
	ArtCmd * artCmd;
	ListCell * listItem;
	char * query;

	// allocate space for variables
	artCmd=malloc(sizeof(ArtCmd));
	query=strip_const(query_string);

	rawParsetreeList=pg_parse_query(query);

	if(list_length(rawParsetreeList)!=1){
		//elog(INFO,"can't deal with multiple querys");
		return query_string;
	}

	/*for each parse tree in list */
	foreach(listItem,rawParsetreeList){
		Node       *parsetree = (Node *) lfirst(listItem);

		if(IsA(parsetree,SelectStmt)){
			Query *pquery;
			SelectStmt * stmt;

			stmt=(SelectStmt *) parsetree;
			query=add_blank(query);
			pquery=parse_analyze(parsetree, query,NULL, 0);

			//SQL_printf("get the pquery");

			/*catch structure information from query tree into artCmd*/
			getAttrno(pquery,artCmd);
			getPartCmd(query,stmt,artCmd,3);

            annosIntegration(artCmd); // differnece between three methods
			//printf("%s \n",artCmd->real_query);
			return artCmd->real_query;
		}
		else{
			// do nothing just return old one
			return query;
		}
	}

	return query;

}

int typeofQuery(const char * query_string){
	if(find(query_string,"select")==-1){
		// create or insert or update without select
		return -1;
	}

	if(find(query_string,"add annotation on")!=-1){
		// add annotation query
		return 5;
	}

	if(find(query_string,"zoom in")!=-1){
		// zoom in query
		return 11;
	}
	if(find(query_string,"from")==-1){
		return -1;
	}

	return 1;
}


char *
simpletranslator(const char *query_string,int mode){

	printf("simpleTranslator: for normal select query \n");

	switch(mode){
        case 0:
            return query_string;
        case 1:
            // only care of annotation
            return standardCompound(query_string);
            break;
        case 2:
            // summary_aware propagation _anno(summary)
            return summaryCompound(query_string);
            break;
        case 3:
            // withouth summary, but final with ANNS
            return hybridCompound(query_string);
            break;
	}
}


char * dealWithExplain(const char * query_string, int mode){
			int start;
			int length;
			char *real_query;
			char * part_query;
			char * temp_query;

			length=find(query_string,"select");

			real_query=(char *) malloc(1024*sizeof(char ));
			temp_query=(char *) malloc(max_len*sizeof(char));
			strcpy(temp_query,query_string);

			real_query=get_substr(temp_query,0,start+length-1);
			part_query=get_substr(temp_query,start+length,strlen(query_string));
			part_query=simpletranslator(part_query,mode);
			real_query=strcat(real_query,part_query);
			return real_query;
}


char * translator(const char * query_string, int mode){

	if(typeofQuery(query_string)==-1){
		return query_string;
	}
	if(typeofQuery(query_string)==5){
		printf("add annotation on query:\n");
		printf("input query_string:%s \n",query_string);
		return dealWithAddAnnotation(query_string);
	}

	return simpletranslator(query_string,mode);
}



char *
extraQuery(const char * query_string){
	// first step parse query string
	// then add select dosomejob('relname');
	// that is all we need to do.
	List * parsertreeList;
	ListCell * lf;


	parsertreeList=pg_parse_query(query_string);

	if(list_length(parsertreeList)!=1){
		return (char *)NULL;
	}


	foreach(lf,parsertreeList){
		Node       *parsetree = (Node *) lfirst(lf);
		char *extraquery=(char *)malloc(max_len*sizeof(char));

		if(IsA(parsetree,SelectStmt))
		{
			SelectStmt * stmt;
			List * fromClause;
			ListCell * lff;
			char * formate;
			stmt=(SelectStmt *) parsetree;

			fromClause=stmt->fromClause;
			formate="select doextrawork(\'%s\') ; ";

			foreach(lff,fromClause){
				 Node * rangelist=(Node *) lfirst(lff);
				 RangeVar *rv=(RangeVar *) rangelist;
				 char * relname=rv->relname;
				 char temp[128];


				 if(find(relname,"pg_")!=-1 || find(relname,"anno")!=-1|| find(relname,"addwork")!=-1 || find(relname,"summary")!=-1){
					 //printf("we search system table \n");
				 }
				 else{
					 sprintf(temp,formate,relname);
					 strcat(extraquery,temp);
				 }


			}
		return extraquery;

		}

		else{
			return (char *)NULL;
		}
	}




}


char * dealWithAddAnnotation(const char * query_string){
			int length;
			char addquery[max_len];
			char * partquery;
			char * addqueryFormate="select addquery(\'%s\') ;";

			length=find(query_string,";");

			partquery=get_substr(query_string,0,length-1);

			sprintf(addquery,addqueryFormate,partquery);
			return addquery;
}


char *
generalTranslator(const char * query_string){

//	//query: create
//	//query: add annotation on
//	// query
//
//	if(find(query_string,"add annotation on")!=-1){
//
//		//
//
//
//	}
//	else{
//		printf("this is for add annotation lazy mode ");
//		return query_string;
////		char newquery[max_len];
////		memset(newquery,0,max_len);
////
////		if(strcmp(addannotation_lazy_mode,"T")==0 && find(query_string,"from")!=-1 && strcmp(query_string,"select")!=-1) {
////			char * extraquery=extraQuery(query_string);
////			if(extraquery!=NULL){
////				strcpy(newquery,extraquery);
////			}
////			strcat(newquery,query_string);
////		}
////		else{
////			strcpy(newquery,query_string);
////		}
////
////
////
////
////		return newquery;
//		//return query_string;
//	}

}


void
zoomCmdParser(const char * query_string, ZoomInCmd  zcmd){

	/**/
	int qidStart;
	int whereStart;
	int onStart;
	int queryQID;
	int qidEnd;

	/**/

	qidStart=find(query_string,"QID");
	whereStart=find(query_string,"where");
	onStart=find(query_string,"on");

	printf("qidStart:%d \n",qidStart);
	printf("whereStart:%d \n",whereStart);
	printf("onStart:%d \n",onStart);


	if(whereStart==-1){
		zcmd->allTuple=TRUE;
		qidEnd=onStart;
	}
	if(onStart==-1){
		zcmd->allAnno=TRUE;
	}

	if(qidEnd==-1){
		qidEnd=strlen(query_string)-2;
	}

	char * queryStr=get_substr(query_string,qidStart+4,qidEnd);

	printf("queryStr:%s\n",queryStr);

	queryQID=atoi(queryStr);

	printf("queryQID:%d \n",queryQID);

	zcmd->qid=queryQID;


	if(whereStart!=-1){
		int whereEnd;
		whereEnd=onStart;

		if(whereEnd==-1){
			whereEnd=strlen(query_string)-2;
		}

		char * whereClause=get_substr(query_string,whereStart+6,whereEnd-1);
		strip(whereClause);



		strcpy(zcmd->whereClause,whereClause);

	}

	if(onStart!=-1){
		int onEnd;
		onEnd=strlen(query_string)-2;

		char * onstr=get_substr(query_string,onStart,onEnd);


		int InstanceEnd=find(onstr,".");

		char * instanceStr=get_substr(onstr,3,InstanceEnd-1);

		char * resultStr=get_substr(onstr,InstanceEnd+1,strlen(onstr));

		strip(resultStr);
		/**/
		strcpy(zcmd->instanceID,instanceStr);
		strcpy(zcmd->summaryResult,resultStr);

	}
	/*here we finished the */



	/**/
}



void
clauseParser(char * whereClause,ParsedClause * pc){

	/*for this time support only =*/

	int opLocation;

	printf("we need to parse where clause :%s \n",whereClause);
	opLocation=find(whereClause,"=");

	pc->attributeName=get_substr(whereClause,0,opLocation-1);
	pc->op=get_substr(whereClause,opLocation,opLocation);
	pc->value=get_substr(whereClause,opLocation+1,strlen(whereClause));

	printf("attributeName:%s#\n",pc->attributeName);
	printf("op: %s \n",pc->op);
	printf("value:%s#\n",pc->value);
}

void
attributeParser(char * attributeStr,PhysicalAttribute *pa){

	int opLocation;

	opLocation=find(attributeStr,":");

	pa->attributesName=get_substr(attributeStr,0,opLocation-1);
	pa->value=get_substr(attributeStr,opLocation+1,strlen(attributeStr));


	printf("parsed attributeName:%s#\n",pa->attributesName);
	printf("parsed value :%s#\n",pa->value);
}
void
initZoomInCmd(ZoomInCmd zcmd){
	zcmd->allAnno=FALSE;
	zcmd->allTuple=FALSE;
	zcmd->qid=-1;
	strcpy(zcmd->instanceID,"");

	strcpy(zcmd->summaryResult,"");
	//memset(zcmd->whereClause,0,124);
	strcpy(zcmd->whereClause,"");




}
