#include "debug.h"
#include "e.h"
#include "data.h"
#include "file.h"
#include "util.h"

void
e_data_type_add_node(E_Data_Base_Type * base, char *prefix,
		     E_Data_Datatype type,
		     E_Data_Base_Type * list_type,
		     int offset, E_Data_Value def_val)
{
   E_Data_Node        *data_node;

   D_ENTER;

   data_node = NEW(E_Data_Node, 1);
   ZERO(data_node, E_Data_Node, 1);

   data_node->prefix = strdup(prefix);
   data_node->type = type;
   data_node->sub_type = list_type;
   data_node->offset = offset;
   switch (type)
     {
     case E_DATA_TYPE_INT:
	data_node->def_val.int_val = def_val.int_val;
	break;
     case E_DATA_TYPE_FLOAT:
	data_node->def_val.float_val = def_val.float_val;
	break;
     case E_DATA_TYPE_STR:
	if (data_node->def_val.str_val)
	  {
	     e_strdup(data_node->def_val.str_val, def_val.str_val);
	  }
	break;
     default:
	break;
     }
   base->nodes = evas_list_append(base->nodes, data_node);

   D_RETURN;
}

E_Data_Base_Type   *
e_data_type_new(void)
{
   E_Data_Base_Type   *t;

   D_ENTER;

   t = NEW(E_Data_Base_Type, 1);
   ZERO(t, E_Data_Base_Type, 1);

   D_RETURN_(t);
}

void               *
e_data_load(char *file, char *prefix, E_Data_Base_Type * type)
{
   E_DB_File          *db;
   char                buf[PATH_MAX];
   Evas_List          *l;
   char               *data;

   D_ENTER;

   if (!e_file_exists(file))
      D_RETURN_(NULL);
   db = e_db_open_read(file);

   if (!db)
      D_RETURN_(NULL);

   data = NEW(char, type->size);
   ZERO(data, char, type->size);

   for (l = type->nodes; l; l = l->next)
     {
	E_Data_Node        *node;

	node = l->data;
	switch (node->type)
	  {
	  case E_DATA_TYPE_INT:
	     {
		int                 val;

		val = 0;
		snprintf(buf, PATH_MAX, "%s/%s", prefix, node->prefix);
		if (e_db_int_get(db, buf, &val))
		   (*((int *)(&(data[node->offset])))) = val;
		else
		   (*((int *)(&(data[node->offset])))) = node->def_val.int_val;
	     }
	     break;
	  case E_DATA_TYPE_STR:
	     {
		char               *val;

		snprintf(buf, PATH_MAX, "%s/%s", prefix, node->prefix);
		if ((val = e_db_str_get(db, buf)))
		   (*((char **)(&(data[node->offset])))) = val;
		else
		   e_strdup((*((char **)(&(data[node->offset])))),
			    node->def_val.str_val);
	     }
	     break;
	  case E_DATA_TYPE_PTR:
	     {
		snprintf(buf, PATH_MAX, "%s/%s", prefix, node->prefix);
		(*((void **)(&(data[node->offset])))) = e_data_load(file, buf,
								    node->
								    sub_type);
	     }
	     break;
	  case E_DATA_TYPE_FLOAT:
	     {
		float               val;

		val = 0;
		snprintf(buf, PATH_MAX, "%s/%s", prefix, node->prefix);
		if (e_db_float_get(db, buf, &val))
		   (*((float *)(&(data[node->offset])))) = val;
		else
		   (*((float *)(&(data[node->offset])))) =
		      node->def_val.float_val;
	     }
	     break;
	  case E_DATA_TYPE_LIST:
	     {
		Evas_List          *l2;
		int                 i, count;

		l2 = NULL;
		snprintf(buf, PATH_MAX, "%s/%s/count", prefix, node->prefix);
		count = 0;
		e_db_int_get(db, buf, &count);
		for (i = 0; i < count; i++)
		  {
		     void               *data2;

		     snprintf(buf, PATH_MAX, "%s/%s/%i", prefix, node->prefix,
			      i);
		     data2 = e_data_load(file, buf, node->sub_type);
		     l2 = evas_list_append(l2, data2);
		  }
		(*((Evas_List **) (&(data[node->offset])))) = l2;
	     }
	     break;
	  case E_DATA_TYPE_KEY:
	     {
		snprintf(buf, PATH_MAX, "%s/%s", prefix, node->prefix);
		(*((char **)(&(data[node->offset])))) = strdup(buf);
	     }
	     break;
	  default:
	     break;
	  }
     }

   e_db_close(db);
   D_RETURN_(data);
}

void
e_data_free(E_Data_Base_Type * type, char *data)
{
   Evas_List          *l;

   D_ENTER;

   for (l = type->nodes; l; l = l->next)
     {
	E_Data_Node        *node;

	node = l->data;
	switch (node->type)
	  {
	  case E_DATA_TYPE_LIST:
	     {
		Evas_List          *l2;

		l2 = (*((Evas_List **) (&(data[node->offset]))));
		while (l2)
		  {
		     char               *data2;

		     data2 = l2->data;
		     l2 = evas_list_remove(l2, data2);
		     if(data2)
		       {
			 e_data_free(node->sub_type, (char *)data2);
			 FREE(data2);
		       }
		  }
	     }
	     break;
	  case E_DATA_TYPE_STR:
	  case E_DATA_TYPE_KEY:
	     {
		IF_FREE((*((char **)(&(data[node->offset])))));
	     }
	     break;
	  case E_DATA_TYPE_PTR:
	     {
		e_data_free(node->sub_type,
			    (*((void **)(&(data[node->offset])))));
		FREE((*((void **)(&(data[node->offset])))));
	     }
	     break;
	  case E_DATA_TYPE_INT:
	  case E_DATA_TYPE_FLOAT:
	     break;
	  default:
	     D("DATA WARNING: Data node %p corrupted!!!\n", node);
	     break;
	  }
     }

   D_RETURN;
}
