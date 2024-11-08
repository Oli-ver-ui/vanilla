#include "header.h"
#include "mem.h"

typedef struct _MIndexH             MH_Index;
typedef struct _IndexH              H_Index;
typedef struct _Internal_Entry      I_Entry;
typedef struct _Index               Index;
typedef struct _MHeader             MHeader;

/*	Index Types:
**
**	Type 1:
**	There are NV Value entries & NV RecNo entries
**						
**	Type 2:
**	There are NV Value entries
**
**	Type 3:
**	There are NV Value entries & NV Count entries.
**	for i (i=0 .. NV-1) there are Count[i] RecNo entries
**		for each i
**	
**	Type 4:
**	Similar to Type 3, only the repeated values exits
**	in clusters of sequential records
**
*/

typedef struct _T1
{
	void     *Value;/*Pointer to start of Value block */
	int		*RecNo;/*Record Number array (= NULL for type 2)*/
}T1;

typedef	struct _T2
{
	void		*Value;/*Pointer to start of Value block */
}T2;

typedef	struct _T3
{
	void     *Value;/*Pointer to start of Value block */
	int      *RecNo;/*Record Number array (= NULL for type 2)*/
	int      *Count;/*Count array ( = NULL for type 1 & type 2)*/
}T3;

typedef	struct _T4
{
	void		*Value;/*Pointer to start of Value block */
	int		*RecCount;/*Record Number, Count  array */
	int		*WadCount; /*Counter Array of Wads 1:1 with values*/
}T4;

typedef union _IType
{
	T1		t1;
	T2		t2;
	T3		t3;
	T4		t4;
	
}IType;

struct _Index {

   int      Type;		/*Index type */
   int      NV;		/*Number of Value entries */

/*	int		Rptr;		Ignore: these are for internal use */
/*	int		Cptr;		Ignore: these are for internal use */


	IType		It;		/*Union of Index type structure*/

};


struct _MIndexH {
   int      NumFields;
   int      Offset;
   int      *f_len;
   char     **field;
   int      *sub;   /* subscript into "field" (if field is an array field) */
   int      *offset;
};
struct _MHeader {
   int      NumFields;
   int      **f_len;
   char     **field;
   int      **sub;   /* subscript into "field" (if field is an array field) */
   int      **offset;
   uchar    *map;
   int      size;
   char     *filename;
};  
  
struct _IndexH {
   int type;
   int NV;
	int Rptr;
	int Cptr;
};  

struct _Internal_Entry {
   union _data data;
   int RecNo;
};  




/* PUBLIC: These functions are the ONLY ones
** that should be called to access the indecies
** DO NOT use the PRIVATE functions (please)
*/

/*Read/Querry*/
MHeader *Open_Index(char *filename);
MHeader *Open_Index_For_Fragment(char *fragname);
void Close_Index(MHeader *mh);

Index *LoadFieldIndex(FIELD *f, int sub, MHeader *mh);

/*Write*/
void Make_Index(char *fields_str, LIST *tables);




/*PRIVATE: */
void  Write_Index(FIELD *f, int sub, int n ,void *ptr);
int   Write_Field_Index(FIELD *f, int sub, int n, I_Entry *ptr, int type, int fd);
void  Write_Master_Index(int fd, MH_Index *mih);

int make_index(FIELD *f, int sub);
int comparitor(void *A, void *B);
int check_index_type(I_Entry *ptr, int num, FIELD *f);
int has_index(MH_Index *mih,FIELD *f, int sub);

MH_Index *LoadMasterIndex(char *filename);
int   Load_MH_Map(MHeader *mh);

void Build_Index_Header(H_Index *ih, int type, FIELD *f, int n);

void Replace_Index(MH_Index *mih,int n, void *ptr,FIELD *f,int sub, int type, int fd);
void Add_Index(MH_Index *mih,int n, void *ptr,FIELD *f, int sub, int type, int fd);
void New_Index(int n, void *ptr,FIELD *f, int sub, int type, int fd);

void Destroy_MH(MHeader *mh);
void Destroy_MH_Index(MH_Index *mih);
FuncPtr select_cmp(FIELD *f);

#define  MI_OFFSET   sizeof(long)
#define	ALIGN			8
