#include "executor/anno_tuple.h"
#include "postgres.h"
#include "c.h"
#include "nodes/pg_list.h" /*list operation!*/
#include "nodes/nodes.h"
#include "utils/builtins.h" /*for reading!*/
#include "utils/supportstring.h"
#include "utils/datum.h" // for copying
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char * find_g_attribute(char *src,char *attribute_name);
static char *find_s_attribute(char *src,char *attribute_name);
static void init_Anno_table(Anno temp,char *src);
//static void init_Anno_table(Anno_table anno_table,char *src);
//static void init_Data_anno(Data_anno data_anno, char *src);
static void init_Data_anno(Anno temp, char *src);
static void init_A_sm_r(A_sm_r asr,char *src);
static A_sm_r create_A_sm_r(char *src);
static List * init_A_sm_r_list(List * asmr_list,char * resdes);
static void init_Anno(Anno an,char *src);
//static Anno create_Anno(char *src);
static Elements create_elements(char * result);
static void add_ID_elements(Elements els,char * anno_id);
static Elements found_result_element(List * elements_list,char * result);
static SingleRep find_summary_id_in_reps(List * reps,char * summary_id);
static void Add_temp_srep(SingleRep srep,char *result, char * anno_id);
//static SingleRep create_rep(char * summary_id);
static void get_value_elements_1(List * elements_list,char * value);
static void init_rep_value(SingleRep srep);




/*implementation
 *
 *
 */
char *
find_g_attribute(char *src,char *attribute_name){
    return find_attribute(src,attribute_name,"--");
}
char *
find_s_attribute(char *src,char *attribute_name){
    return find_attribute(src,attribute_name,"-");
}


void
init_Anno_table(Anno temp,char *src){

    temp->content->rID=find_g_attribute(src, "id");
    temp->content->author=find_g_attribute(src, "author");
    temp->content->value=find_g_attribute(src, "value");
    temp->content->timestamp=find_g_attribute(src, "timestamp");
}

void
init_Data_anno(Anno temp, char *src){
	//char * tuple_columns;
	//char ** dest;

	//int num;
	//int i;
    temp->desc->tuple_id=find_g_attribute(src,"tuple_id");
    temp->desc->tuple_columns=find_g_attribute(src,"tuple_columns");
    temp->desc->table_name=find_g_attribute(src,"table_name");

    /*
    tuple_columns=temp->desc->tuple_columns;
    dest=(char **)malloc(64*sizeof(char *));
    num=split(dest,tuple_columns,",");

    if(num==0){
    	printf("split problem \n");
    	//return;
    }
    else{
    	for(i=0;i<num;i++){

    		int attrnum=atoi(dest[i]);
    		temp->desc->tuple_column_list=lappend_int(temp->desc->tuple_column_list,attrnum);
    	}

    }
    */
    //free(dest);
}

void
init_A_sm_r(A_sm_r asr,char *src){
    asr->summary_type=find_s_attribute(src,"summary_method");
    asr->summary_id=asr->summary_type;
    asr->result=find_s_attribute(src,"result");

}

A_sm_r
create_A_sm_r(char *src){
    A_sm_r temp=(A_sm_r)malloc(sizeof(A_sm_r_data));
    init_A_sm_r(temp,src);
    return temp;
}

List *
init_A_sm_r_list(List * asmr_list,char * resdes){
	char **dest;
	int num, i;
	List * temp;
    dest=(char **)malloc(MAx_Summary_method_num*256*sizeof(char));
    num=split(dest,resdes,"|");

    temp=asmr_list; //copy
    if(num==0) return temp; // skip

    for(i=0;i<num;i++){
        temp=lappend(temp,create_A_sm_r(dest[i]));
    }
    return temp;
}

void
init_Anno(Anno an,char *src){
	char *resdes;
	//char * tempsrc;
	/*
	an->content=(Anno_table)malloc(sizeof(Anno_table));
	an->content->rID=(char *)NULL;
	an->content->author=(char *)NULL;
	an->content->timestamp=(char *)NULL;
	an->content->value=(char *)NULL;

	an->desc=(Data_anno)malloc(sizeof(Data_anno));
	an->desc->table_name=(char *)NULL;
	an->desc->tuple_columns=(char *)NULL;
	an->desc->tuple_id=(char *)NULL;
	an->desc->tuple_column_list=(List *)NULL;
	an->resdes=(char *)NULL;
	an->a_sm_r_list=(List *) NULL;
	*/






    init_Anno_table(an,src);
    //elog(INFO,"1: content->rID:%s",an->content->rID);
    //elog(INFO,"1:content->author:%s",an->content->author);
    //elog(INFO,"1:content->value:%s",an->content->value);


    init_Data_anno(an,src);
    //elog(INFO,"Data_anno->tuple_columns:%s",an->desc->tuple_columns);
    //elog(INFO,"2:content->rID:%s",an->content->rID);
    //elog(INFO,"2:content->author:%s",an->content->author);
    //elog(INFO,"2:content->value:%s",an->content->value);


    an->resdes=find_g_attribute(src,"resdes");
    resdes=find_g_attribute(src,"resdes");
    an->a_sm_r_list=init_A_sm_r_list(an->a_sm_r_list,resdes);

    //elog(INFO,"asmr_list_length:%d",list_length(an->a_sm_r_list));
    //elog(INFO,"3:anno_table->rID:%s",an->content->rID);
    //elog(INFO,"an->desc->tuple_columns:%s",an->desc->tuple_columns);
    //elog(INFO,"resdes:%s",an->resdes);
}

/*
Anno
create_Anno(char *src){
	Anno temp=(Anno)malloc(sizeof(Anno));
	temp->content=(Anno_table)malloc(sizeof(Anno_table));
	temp->desc=(Data_anno)malloc(sizeof(Data_anno));
	temp->a_sm_r_list=(List *) NULL;
	init_Anno(temp,src);
	elog(INFO,"2:temp->desc->tuple_columns:%s",temp->desc->tuple_columns);

	return temp;
}
*/



void
init_anno_list(Summary_tuple st,char *src){
	char ** dest;
	int num,i;

    if(!src) return ;// skip

    dest=malloc(Max_Anno_num*sizeof(char *));
    num=split(dest,src,"||");
    if(num==0) return;

    for(i=0;i<num;i++)
    {
       // st->anno_list=lappend(st->anno_list,create_Anno(dest[i]));
    	Anno an=(Anno)malloc(sizeof(Anno_data));


    	an->content=(Anno_table)malloc(sizeof(Anno_table_data));
    	/*
    	an->content->rID=(char *)NULL;
		an->content->author=(char *)NULL;
		an->content->timestamp=(char *)NULL;
		an->content->value=(char *)NULL;
		*/

		an->desc=(Data_anno)malloc(sizeof(Data_anno_data));
		/*
		an->desc->table_name=(char *)NULL;
		an->desc->tuple_columns=(char *)NULL;
		an->desc->tuple_id=(char *)NULL;
		*/
		an->desc->tuple_column_list=(List *)NULL;
		//an->resdes=(char *)NULL;

		an->a_sm_r_list=(List *) NULL;





    	init_Anno(an,dest[i]);
    	// real confuse me why

    	st->anno_list=lappend(st->anno_list,an);
    }

    //travel_summary(st);
}


/*representation*/
Elements
create_elements(char * result){
    Elements temp_elem=(Elements)malloc(sizeof(ElementsData));
    temp_elem->result=result;
    temp_elem->anno_IDs=(List *) NULL;
    return temp_elem;
}

void
add_ID_elements(Elements els,char * anno_id){
    els->anno_IDs=lappend(els->anno_IDs,anno_id);
}

Elements
found_result_element(List * elements_list,char * result){
	ListCell *  lf;
    if(elements_list==NULL) return (Elements)NULL;

    foreach(lf,elements_list){
        Elements el=(Elements)lfirst(lf);
        if(strcmp(el->result,result)==1){
            return el;
        }
    }
    return (Elements)NULL;
}

/*reps*/



SingleRep
find_summary_id_in_reps(List * reps,char * summary_id){

	ListCell * list_item;
    if(reps==NULL) return (SingleRep) NULL;;

    foreach(list_item,reps){
        Node * single_rep_node=lfirst(list_item);
        SingleRep srep=(SingleRep) single_rep_node;
        /*here we get*/

        if(strcmp(srep->summary_ID,summary_id)==0){
            return srep;
        }
    }

    return (SingleRep) NULL;
}






void
Add_temp_srep(SingleRep srep,char *result, char * anno_id){

	if(!result) return;
    Elements found_elements=found_result_element(srep->elements_list,result);
    if(found_elements==NULL){
        Elements temp_elem=create_elements(result);
        add_ID_elements(temp_elem,anno_id);
        srep->elements_list=lappend(srep->elements_list,temp_elem);
    }
    else{
        add_ID_elements(found_elements,anno_id);
    }


}

/*
SingleRep
create_rep(char * summary_id){
    //initiate memory
	int end;
    SingleRep temp_srep=(SingleRep) malloc(sizeof(SingleRep));
    temp_srep->summary_ID=summary_id;
    temp_srep->elements_list=(List *) NULL;
    temp_srep->value=malloc(sizeof(char *));
    //temp_srep->value=malloc(1024*sizeof(char ));
    //temp_srep->value=strcat(temp_srep->value,summary_id); // change by dongqing
   // elog(INFO,"334 summary_id:%s",summary_id);
    end=find(summary_id,"_");

    if(end!=-1){
        // find the summay_type
        temp_srep->summary_type=get_substr(summary_id,0,end-1);
    }
    else{
        temp_srep->summary_type="unknown";
    }
    //elog(INFO,"344 summary_type:%s", temp_srep->summary_type);

    return temp_srep;
}
*/

void
get_value_elements_1(List * elements_list,char * value){
	char * temp_value;
	char * formate;
    temp_value=(char *) malloc(1024*sizeof(char));
    formate="%s:%d";
    if(!elements_list){
        value="NULL";
    }
    else{
        ListCell * lf;
        //printf("elements_length:%d \n",list_length(elements_list));
        foreach(lf,elements_list){

            Elements el=(Elements)lfirst(lf);
            MemSet(temp_value,0,1024);
            //printf("el->result :%s \n",el->result);

            sprintf(temp_value,formate,el->result,list_length(el->anno_IDs));
            strcat(value,temp_value);

        }

    }

}

void
init_rep_value(SingleRep srep){

    //elog(INFO,"373summary_type:%s",srep->summary_type);

    /*List serarch */

    if(strcmp(srep->summary_type,"classifier")==0){

    	srep->value=get_substr(srep->summary_ID,0,strlen(srep->summary_ID));
    	strcat(srep->value,"->");
        get_value_elements_1(srep->elements_list,srep->value);
        return;
    }
    if(strcmp(srep->summary_type,"cluster")==0){
    	srep->value=srep->summary_ID;
    	strcat(srep->value,"->");
        get_value_elements_1(srep->elements_list,srep->value);
        return;
    }
    if(strcmp(srep->summary_ID,"snippet")==0){
    	srep->value=srep->summary_ID;
    	strcat(srep->value,"->");
        get_value_elements_1(srep->elements_list,srep->value);
        return;
    }
    //srep->value="unknown type.= =!";



}



void
init_reps_from_anno_list(Summary_tuple st){
    printf("init_reps_from_anno_list \n");
	return;

    List * anno_list=st->anno_list;
    ListCell * list_item;
    ListCell * rep_item;



    foreach(list_item,anno_list)
    {
        Node * s_an_node=(Node *) lfirst(list_item);
        Anno s_an=(Anno) s_an_node;
        /*here we get the anno*/
        //elog(INFO,"%s","331");
        List * asml=s_an->a_sm_r_list;

        ListCell * lf;
        foreach(lf,asml)
        {

            A_sm_r sasmr=(A_sm_r) lfirst(lf);
            /*here we get the each summary result, try to compare them */
            if(!sasmr->summary_id) return;
            SingleRep found_srep=find_summary_id_in_reps(st->reps,sasmr->summary_id);
            if(found_srep==NULL){
                //SingleRep temp_srep=create_rep(sasmr->summary_id);
            	char * summary_id=sasmr->summary_id;
            	int end;
            	SingleRep temp_srep=(SingleRep) malloc(sizeof(SingleRepData));
            	temp_srep->summary_ID=summary_id;
            	temp_srep->elements_list=(List *) NULL;
            	temp_srep->value=malloc(sizeof(char *));

            	end=find(summary_id,"_");

            	    if(end!=-1){
            	        // find the summay_type
            	        temp_srep->summary_type=get_substr(summary_id,0,end-1);
            	    }
            	    else{
            	        temp_srep->summary_type="unknown";
            	    }

                Add_temp_srep(temp_srep,sasmr->result,s_an->content->rID);
                st->reps=lappend(st->reps,temp_srep);

            }
            else
            {
                Add_temp_srep(found_srep,sasmr->result,s_an->content->rID);
            }

        }


    }

    /*initiate single rep's value*/

    //elog(INFO,"reps_length:%d",list_length(st->reps));

    foreach(rep_item,st->reps){
        SingleRep rep=(SingleRep)lfirst(rep_item);
        init_rep_value(rep);
    }


}

char *
Represtation(Summary_tuple st){


	List * reps;
	ListCell * list_item;
	//char * value;
	char * result;
	int replength;

	//printf("call this function");
	/**/
	reps=st->reps;
	replength=list_length(st->reps);
	if(replength==0){
		//printf("NULL");
		return NULL ;
	}
	result=(char *)malloc(replength*1024*sizeof(char));
	MemSet(result,0,replength*1024);

	//printf("rep_length:%d\n",list_length(reps));

	foreach(list_item,reps)
	{
		 SingleRep rep=(SingleRep)lfirst(list_item);
		 strcat(result,rep->value);
		 strcat(result,";\n");
	}



	return result;

}


void
travel_summary(Summary_tuple st){
	/*make sure nothing will be missing*/
	 List * anno_list=st->anno_list;
	 ListCell * lf;

	 foreach(lf,anno_list){
	    	ListCell * asrlf;
	        Anno sanno=(Anno) lfirst(lf);
	        printf("travel:tuple_columns:%s \n",sanno->desc->tuple_columns);
	        //printf("travel:desc->tuple_columns_list:%d\n",list_length(sanno->desc->tuple_column_list));
	        printf("travel:a_smr_list_length:%d \n",list_length(sanno->a_sm_r_list));

	        printf("travel:content->rID :%s \n",sanno->content->rID);

	        foreach(asrlf,sanno->a_sm_r_list){
	        	A_sm_r asmr=(A_sm_r) lfirst(asrlf);
	        	printf("travel:asmr->summary_id %s \n",asmr->summary_id);


	        }
	    }
}

void filter(Summary_tuple st,List * attrl){

	List * keep_anno=(List *)NULL;
	List * anno_list=st->anno_list;
	ListCell * lf;
		 //elog(INFO,"filter");
		    foreach(lf,anno_list){
		        Anno sanno=(Anno) lfirst(lf);
		        List * tl=sanno->desc->tuple_column_list;
		        ListCell *tlf;
		        bool keep=false;


		        //elog(INFO,"%s",sanno->desc->tuple_columns);
		        //elog(INFO,"desc->tuple_columns_list:%d",list_length(tl));
		        foreach(tlf,tl){
		        	ListCell * atlf;
		        	int attrnum=(int )lfirst(tlf);
		        	//elog(INFO,"attrnum:%d",attrnum);

		        	foreach(atlf,attrl){
		        		int num=(int) lfirst(atlf);
		        		if(num==attrnum){
		        			keep=true;
		        			break;
		        		}
		        	}

		        	if(!keep){
		        		//printf("need remove \n");

		        	}
		        	else{
		        		keep_anno=lappend(keep_anno, sanno);
		        	}
		        }
	 }
		    st->anno_list=keep_anno;
		    //printf("currently list_length %d \n",list_length(st->anno_list));


}



void
summaryHashTable_init(){
	HASHCTL ctl;
	Assert(summaryhashtable==NULL);

	memset(&ctl,0,sizeof(ctl));

	ctl.keysize=sizeof(AnnoHashkey);
	ctl.entrysize=sizeof(AnnoHashElem);
	ctl.hash=string_hash;
	summaryhashtable=hash_create("summary_hash_table", 16, &ctl, HASH_ELEM | HASH_FUNCTION);
}

void insertHash(char * value){

	AnnoHashkey key;
	AnnoHashElem * elem;
	bool found;
	//List * asmrlist;

	elem=malloc(sizeof(AnnoHashElem));
	elem->a_sm_r_list=(List *)NULL;

	strcpy(key.id,find_s_attribute(value,"id"));
	elem=hash_search(summaryhashtable,(void *)&key,HASH_ENTER, &found);

	if(found){
		printf("insert hash all ready exits sorry \n");

	}else{
		//printf("549 good news \n");
		elem->a_sm_r_list=init_A_sm_r_list(elem->a_sm_r_list,value);
		//printf("550 length:%d",list_length(elem->a_sm_r_list));
	}
	/* for checking
	elem=hash_search(summaryhashtable,(void *)& key, HASH_FIND,&found);
	if(found){
		printf("558 good \n");
	}
	*/

}

List *
searchHash(char * value){

	AnnoHashkey key;
	AnnoHashElem * elem;
	bool found;
	elem=malloc(sizeof(AnnoHashElem));
	//elem->a_sm_r_list=(List *)NULL;

	strcpy(key.id,value);
	//printf("key.id:%s \n",key.id);
	elem=hash_search(summaryhashtable,(void *)&key,HASH_FIND, &found);
	if(found){
		//printf("hash serach find what we need \n");
		//printf("629 hash serach find summary_list:%d \n",list_length(elem->a_sm_r_list));
		return elem->a_sm_r_list;
	}
	else{
		//printf("something wrong with hash table \n");
	}
	return (List*)NULL;
}

void
fillsummary(Summary_tuple st){
	 List * anno_list;
	 ListCell * lf;

	 anno_list=st->anno_list;

	 foreach(lf,anno_list){
		 Anno an=(Anno) lfirst(lf);
		 char * idstr=an->content->rID;

		 an->a_sm_r_list=searchHash(idstr);

		 //printf("search key value annotable->rID:%s \n",an->content->rID);
		 /*
		 printf("search key value annotabel->author: %s \n",an->content->author);
		*/

		 //printf("init an' a_sm_r_list :%d \n",list_length(an->a_sm_r_list));

	 }

}

void initSummaryTupleStruce(Summary_tuple st){

	st->anno_list=(List *)NULL;
	st->reps=(List *)NULL;
}
