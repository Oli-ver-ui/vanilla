#include <stdio.h>
#include <stdlib.h>
#include "vidx.h"


/*GLOBALS*/

int		Comparitor_Path=0;
int		Comparitor_String_Length;
int		*index_type_array=NULL;
int		*wad_array=NULL;
int		*count_array=NULL;

#if DEBUG
int
RecCmp(void *A, void *B)
{
	I_Entry *a, *b;
	
	a=(I_Entry *)A;
	b=(I_Entry *)B;

	if (a->RecNo < b->RecNo) return (-1);
	if (a->RecNo > b->RecNo) return (1);

	fprintf(stderr,"Hey!  You have two different Records with equal No's:");
	fprintf(stderr," %d %d\n",a->RecNo , b->RecNo);

	return(0);
}


	

/*TESTING PURPOSES ONLY*/
void Driver1(char *filename,FIELD *f,int sub)
{
	Index *Idx;
	MHeader *mh;
	int i,j;
	TABLE *table;
	int reclen;
	uchar *q;

	I_Entry	A,B;
	FuncPtr	V;
	int Base=0;
	int Offset;

	uchar *a,*b;

	fprintf(stderr,"Performing Test 1\n");

	mh=(MHeader *)Open_Index(filename);
	Idx=LoadFieldIndex(f,sub,mh);
   table = f->label->table;
   
   reclen = table->label->reclen;

/*	V=(int (*)(const void *, const void *))select_cmp(f); */
	V=select_cmp(f);

   q = (uchar *)(table->buff->buf +
                 table->buff->frag->fraghead->offset +
                 f->start);

   if (f->dimension > 0){
       q = (uchar *)(q + f->size * sub);
   }

	switch (Idx->Type) {

	case 1:
		for (i=0;i<Idx->NV;i++){
			a=q+(Idx->It.t1.RecNo[i])*reclen;
			b=Idx->It.t1.Value+(f->size*i);
			A.data=ConvertData(a,f);
			A.RecNo=0;
			B.data=ConvertData(b,f);
			B.RecNo=0;
			if (comparitor((void *)&A,(void *)&B)){
					fprintf(stderr,"We have a mis match!\n");
			}
		}
		break;
	
	case 2:
		for (i=0;i<Idx->NV;i++){
			a=q+i*reclen;
			b=Idx->It.t2.Value+(f->size*i);
			A.data=ConvertData(a,f);
			A.RecNo=0;
			B.data=ConvertData(b,f);
			B.RecNo=0;
			if (comparitor((void *)&A,(void *)&B)){
					fprintf(stderr,"We have a mis match!\n");
			}
		}
		break;


	case 3:
		for (i=0;i<Idx->NV;i++){
			b=Idx->It.t3.Value+(f->size*i);
			B.data=ConvertData(b,f);
			B.RecNo=0;
			if (i==0){
				Base=Idx->It.t3.Count[i];
				Offset=0;
			}

			else {
				Base=Idx->It.t3.Count[i]-Idx->It.t3.Count[i-1];
				Offset=Idx->It.t3.Count[i-1];
			}

			for (j=0;j<Base;j++){

				a=q+(Idx->It.t3.RecNo[Offset+j]*reclen);
				A.data=ConvertData(a,f);
				A.RecNo=0;
				if (comparitor((void *)&A,(void *)&B)){
					fprintf(stderr,"Mis-match: field:%s RecNo:%d I-val:%d J-val:%d!\n",
									f->name,Idx->It.t3.RecNo[Offset+j],i,j);
				}
			}
		}
		break;

	case 4:
	 {
		int cumm1;
		int cumm2;
		int k;
		for (i=0;i<Idx->NV;i++){
			b=Idx->It.t4.Value+(f->size*i);
			B.data=ConvertData(b,f);
			B.RecNo=0;

	
			if (i==0){
				Base=Idx->It.t4.WadCount[0];
				Offset=0;
			}
			else {
				Base=Idx->It.t4.WadCount[i]-Idx->It.t4.WadCount[i-1]; /*Wad Count for current Value; */
				Offset=Idx->It.t4.WadCount[i-1]*2; /*Index into Count array */
			}

			for (j=0;j< Base*2;j+=2){	
				cumm2=Idx->It.t4.RecCount[Offset+j+1];


				for (k=0;k<cumm2;k++){
					a=q+((Idx->It.t4.RecCount[Offset+j]+k)*reclen);
					A.data=ConvertData(a,f);
					A.RecNo=0;
					if (comparitor((void *)&A,(void *)&B)){
						fprintf(stderr,"Mis-match: field:%s RecNo:%d I-val:%d J-val:%d!\n",
									f->name,Idx->It.t4.RecCount[Offset+j]+k,i,j);
					}
				}
			}
	 	}
	 }

 	} 
	Close_Index(mh);
}

I_Entry *
Expand(Index *Idx,FIELD *f)
{
	I_Entry *ptr;
	int i;
	int j;
	int k;
	char *b;
	int sum=0;
	int base=0;
	int Offset;
	DATA data;
	int cumm,cumm2;


	switch (Idx->Type) {

	case 1:
		ptr=(I_Entry *)calloc(Idx->NV,sizeof(I_Entry));
		for (i=0;i<Idx->NV;i++){
			ptr[i].RecNo=Idx->It.t1.RecNo[i];
			b=Idx->It.t1.Value+(f->size*i);	
			ptr[i].data=ConvertData(b,f);
		}
		break;

	case 2:
		ptr=(I_Entry *)calloc(Idx->NV,sizeof(I_Entry));
		for (i=0;i<Idx->NV;i++){
			ptr[i].RecNo=i;
			b=Idx->It.t1.Value+(f->size*i);
			ptr[i].data=ConvertData(b,f);
		}

		break;

	case 3:
		sum+=Idx->It.t3.Count[Idx->NV-1];
		ptr=(I_Entry *)calloc(sum,sizeof(I_Entry));
		for (i=0;i<Idx->NV;i++){
			b=Idx->It.t3.Value+(f->size*i);
			data=ConvertData(b,f);
			if (i==0){
				base=Idx->It.t3.Count[i];
				Offset=0;
			}

			else {
				base=Idx->It.t3.Count[i]-Idx->It.t3.Count[i-1];
				Offset=Idx->It.t3.Count[i-1];
			}
			for(j=0;j<base;j++){
				ptr[Offset+j].RecNo=Idx->It.t3.RecNo[Offset+j];
				ptr[Offset+j].data=data;
			}
		}
		break;

	case 4:
		cumm=0;
		for (i=1;i<(Idx->It.t4.WadCount[Idx->NV-1]*2);i+=2){
			sum+=Idx->It.t4.RecCount[i];
		}
		ptr=(I_Entry *)calloc(sum,sizeof(I_Entry));
		for (i=0;i<Idx->NV;i++){
			b=Idx->It.t4.Value+(f->size*i);
			data=ConvertData(b,f);
			if (i==0){
				base=Idx->It.t4.WadCount[i];
				Offset=0;
			}
			else {
				base=Idx->It.t4.WadCount[i]-Idx->It.t4.WadCount[i-1];
				Offset=Idx->It.t4.WadCount[i-1]*2;
			}
			for(j=0;j<base*2;j+=2){
				cumm2=Idx->It.t4.RecCount[Offset+j+1];

            for (k=0;k<cumm2;k++){
					
					ptr[cumm+k].RecNo=Idx->It.t4.RecCount[Offset+j]+k;
					ptr[cumm+k].data=data;
						
				}
				cumm+=cumm2;
			}
		}
	}

	return(ptr);
}
			


void
Driver2(char *filename,FIELD *f,int sub)
{
   Index *Idx;
	I_Entry *ptr;
   MHeader *mh;
   int i,j;
	int nrows;
   TABLE *table;
   int reclen;
   uchar *q;
   
   I_Entry  A,B;
   int Base=0;  

   uchar *a,*b;

	FuncPtr V;
      
   fprintf(stderr,"Performing Test 2\n");
  
   mh=(MHeader *)Open_Index(filename);
   Idx=LoadFieldIndex(f,sub,mh);
	ptr=Expand(Idx,f);
   table = f->label->table;
	nrows = table->buff->frag->fraghead->nrows;
   reclen = table->label->reclen;
 	V=select_cmp(f); 
   q = (uchar *)(table->buff->buf +
                 table->buff->frag->fraghead->offset +
                 f->start);

   if (f->dimension > 0){
       q = (uchar *)(q + f->size * sub);
   }
   
      qsort(ptr, nrows, (sizeof(I_Entry)),
           (int (*)(const void *, const void *))RecCmp); 


	for (j=0;j<nrows;j++){
		if (ptr[j].RecNo!=j){
			fprintf(stderr,"Missing Record! At: %d found %d\n",j,ptr[j].RecNo);
		}
		a=(q+(j*reclen));

      A.data=ConvertData(a,f);
      A.RecNo=0;
      B.data=ptr[j].data;
      B.RecNo=0;
      if (comparitor((void *)&A,(void *)&B)){
           fprintf(stderr,"We have a mis match!\n");
      }
	}
	free(ptr);
	Close_Index(mh);
}

#endif

/*
** Mallocs and returns the name of the default index file
** associated with the specified table fragment.
*/
static char *
FindIdxFileForTblFrag(
   const char *fname   /* Fragment name */
)
{
  char *idxname;
  char *p;
  char *sfx[] = {"idx", "IDX"}; /* Index suffixes to try */
  int   nsfx = sizeof(sfx)/sizeof(char *);
  int   i;
  int   trspc; /* trailing space after the dot in the frag file name */
  
  idxname = strdup(fname);
  if (idxname == NULL){ return NULL; }
  
  p = strrchr(idxname, '.');
  if (p){
    p++; /* skip over the '.' */
    trspc = strlen(p);
    
    for (i = 0; i < nsfx; i++){
      if (strlen(sfx[i]) > trspc){
        fprintf(stderr, "Warning! Suffix \"%s\" too long for index name generation.\n", sfx[i]);
      }
      else {
        sprintf(p, sfx[i]);
        if (access(idxname, R_OK) == 0){
          return idxname;
        }
      }
    }

    /* Search failed, deallocate index name and return NULL */
    free(idxname);
    idxname = NULL;
  }

  return idxname;
}



MHeader *
Open_Index_For_Fragment(char *fragname)
{
  MHeader *mh = NULL;
  char    *idxname;

  idxname = FindIdxFileForTblFrag(fragname);
  if (idxname != NULL){
    mh = Open_Index(idxname);
  }

  return mh;
}


MHeader *
Open_Index(char *filename)
{
	struct stat		sbuf;
	MHeader	*mh;	
	int	fd;
	

	mh=(MHeader *)calloc(1,sizeof(MHeader));

/*
	if ((fd=open(filename,O_RDONLY,0777))==-1){
		fprintf(stderr,"Can't find file %s\n",filename);
		return(NULL);
	}
*/

	fd=stat(filename,&sbuf);

	if (fd)
		return(NULL);

	mh->size=sbuf.st_size;
	mh->filename=strdup(filename);

/*
	mh->map=mmap(NULL,mh->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd,0);
*/

	mh->map=(uchar *)MemMapFile(NULL,mh->size, PROT_READ | PROT_WRITE, 
							 MAP_PRIVATE, filename, O_RDONLY,0);

	if (mh->map==NULL)
		return(NULL);

	Load_MH_Map(mh);

	return(mh);
}


void
Close_Index(MHeader *m)
{
	MHeader *mh=m;

	/*munmap(mh->map,mh->size);*/

	MemUnMapFile(mh->map,mh->size);

	Destroy_MH(mh);
}

void 
Destroy_MH_Index(MH_Index *mih)
{
	int i;
	if (mih==NULL)
		return;

	for (i=0;i<mih->NumFields;i++){
		free(mih->field[i]);
	}
    free(mih->sub);
	free(mih->offset);
	free(mih->f_len);

	free(mih);
}

void
Destroy_MH(MHeader *mh)
{

	free(mh->filename);
	free(mh->f_len);
	free(mh->field);
    free(mh->sub);
	free(mh->offset);
	free(mh);

}
	

int
Load_MH_Map(MHeader *mh)
{

	uchar *buf;
	int	ptr=0;
	int 	i;
	int nf;



	buf=mh->map+((long *)mh->map)[0];

	memcpy(&mh->NumFields,buf,sizeof(int)); /*Get the Number of fields*/;
	ptr+=sizeof(int);
	nf=mh->NumFields;


	mh->f_len=(int **)calloc(nf,sizeof(int *));
	mh->field=(char **)calloc(nf,sizeof(char *));
    mh->sub=(int **)calloc(nf,sizeof(int *));
	mh->offset=(int **)calloc(nf,sizeof(int *));

	for (i=0;i<(mh->NumFields);i++){
		mh->f_len[i]=(int *)(buf+ptr);
		ptr+=sizeof(int);
		mh->field[i]=(char *)(buf+ptr);
		ptr+=*(mh->f_len[i]);
        mh->sub[i]=(int *)(buf+ptr);
        ptr+=sizeof(int);
		mh->offset[i]=(int *)(buf+ptr);
		ptr+=sizeof(int);
	}
}


int
cmp_int(int *a, int *b)
{

   return ((*a < *b) ? -1 : ((*a > *b) ? 1 : 0));
} 

int
cmp_uint(uint *a, uint *b)
{

   return ((*a < *b) ? -1 : ((*a > *b) ? 1 : 0));
} 

int
cmp_real(double *a, double *b)
{

   return ((*a < *b) ? -1 : ((*a > *b) ? 1 : 0));
} 

int cmp_string(char *a, char *b)
{
	int r;
	r=strncmp(a,b,Comparitor_String_Length);
	if (r < 0)
		return(-1);
	if (r > 0)
		return(1);
	else
		return(0);
}



FuncPtr
select_cmp(FIELD *f)
{
	FuncPtr	V;
	V=(FuncPtr)comparitor;

   switch(f->iformat) {
      case INT:
         Comparitor_Path=1;
			break;
      case UINT:
         Comparitor_Path=2;
			break;
      case REAL:
         Comparitor_Path=3;
			break;
		case STRING:
         Comparitor_Path=4;
			Comparitor_String_Length=f->size;
			break;

      default:
         return(NULL);
   }

	return(V);
} 

int
Write_Field_Index(FIELD *f, int sub, int n, I_Entry *ptr, int type, int fd)
{
	int i,j;
	TABLE *table;
   uchar *q;
	int reclen;
	int index;
	int bytes=0;
	int rem=0;
	char pad[ALIGN]={0};
	int tmp=0;
	int cumm;

   table = f->label->table;

   reclen = table->label->reclen;


   q = (uchar *)(table->buff->buf +
                 table->buff->frag->fraghead->offset +
                 f->start);

   if (f->dimension > 0){
       q = (uchar *)(q + f->size * sub);
   }
	
/*
** After writting the Values, a padding of upto 
** 7 bytes is added to 8-byte align the data. This 
** is also done after RecNo so that the next index 
** will be start aligned
*/

	switch(type) {
			

	case 1:
		/*First Write out Values*/
		for (i=0;i<n;i++){
			write(fd,(q+ptr[i].RecNo*reclen),f->size);
		}
		rem=n*f->size;
		rem=((ALIGN-(rem % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes=rem+n*f->size;

		/*Then Write out RecNos*/
		for (i=0;i<n;i++){
			write(fd,&ptr[i].RecNo,sizeof(int));
		}
		rem=n*sizeof(int);
		rem=((ALIGN-(rem % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem+n*sizeof(int);

		
		break;

	case 2:
		/*Just Write out Values*/
		for (i=0;i<n;i++){
			write(fd,(q+ptr[i].RecNo*reclen),f->size);
		}
		bytes=n*f->size;
		rem=((ALIGN-(bytes % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem;

		break;
	case 3:
		/*First Write out Values */
		for(i=0;i<n;i++){
			if (index_type_array[i]){
				write(fd,(q+ptr[i].RecNo*reclen),f->size);
				bytes+=f->size;
			}
		}
		rem=((ALIGN-(bytes % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem;

		/*Then Write out cummlative Counts (one-to-one on Values)*/
		cumm=0;
		for(i=0;i<n;i++){
			if (index_type_array[i]){
				cumm+=index_type_array[i];
				write(fd,&cumm,sizeof(int));
				tmp+=sizeof(int);
			}
		}
		/*Now we Write out the RecNos*/
		for (i=0;i<n;i++){
			write(fd,&ptr[i].RecNo,sizeof(int));
		}
		tmp+=n*sizeof(int);
		rem=((ALIGN-(tmp % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem+tmp;

		break;

	case 4:
		/*First Write out Values */
		{
		int tmp1=0;
		int tmp2=0;
		int cumm2=0;
		int cnt=0;

		for(i=0;i<n;i++){
			if (index_type_array[i]){
				write(fd,(q+ptr[i].RecNo*reclen),f->size);
				bytes+=f->size;
			}
		}
		rem=((ALIGN-(bytes % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem;

		/*Now write out Wad counts */
		cumm=0;
		for (i=0;i<n;i++){
			if (index_type_array[i]){
				cumm+=wad_array[tmp1];
				write(fd,&cumm,sizeof(int));
				tmp2+=sizeof(int);
				tmp1++;
			}
		}
		/*Now write out counts for all wad groups*/
		for(i=0;i<cumm;i++){
			if (i==0)
				cumm2=0;
			else
				cumm2+=count_array[i-1];

			write(fd,&ptr[cumm2].RecNo,sizeof(int));
			write(fd,&count_array[i],sizeof(int));
			tmp2+=(2*sizeof(int));
		}

		rem=((ALIGN-(tmp2 % ALIGN))%ALIGN);
		write(fd,pad,rem);
		bytes+=rem+tmp2;
		}
	}
	return(bytes);
}

int 
comparitor(void *A, void *B)
{
	I_Entry *a,*b;
	FuncPtr	*V;
	int R;

	/*Extract Temporary Entry*/
	a=(I_Entry *)A;
	b=(I_Entry *)B;

	/*Call comparison function based on Value of Comparitor_Path:
		Set when called select_cmp();
	*/

   switch(Comparitor_Path) {
   	case 1: R=cmp_int(&a->data.i,&b->data.i);
				break;
      case 2: R=cmp_uint(&a->data.ui,&b->data.ui);
				break;
      case 3: R=cmp_real(&a->data.r,&b->data.r);
				break;
      case 4: R=cmp_string(a->data.str,b->data.str);
   }

	/*If data is equal, then sort by Record Number*/
	if (R)
		return(R);

	else
		return((a->RecNo < b->RecNo) ? -1 : ((a->RecNo > b->RecNo) ? 1 : 0));
}

int
make_index(
    FIELD *f, /* field to be indexed */
    int sub   /* zero-based subscript: f[sub] if "f" is an array */
    )
{ 
    TABLE *table;
    int i, j, n, nrows, nbytes, reclen;
    uchar *q;
	I_Entry *ptr;
	char *p;
    RHND rhnd;
    
    nbytes = f->size;
   
    table = f->label->table;
  
    table->buff = NewTblBuff(table);
  
    while (table->buff != NULL) {
        nrows = table->buff->frag->fraghead->nrows;
        reclen = table->label->reclen;
        q = (uchar *)(table->buff->buf +
                      table->buff->frag->fraghead->offset +
                      f->start);

        if (f->dimension > 0){ /* array field */
            q = (uchar *)(q + sub * f->size);
        }

        ptr = (I_Entry *)calloc(nrows,sizeof(I_Entry));
        
		/* j is our RecNo */
        for (j = 0 ; j < nrows ; j++) {
        	ptr[j].data=ConvertData((q + j*reclen),f);
			ptr[j].RecNo=j;
        }

	  #ifdef _WINDOWS 
	  /* Some Microsoft Visual C compilers need the "__cdecl" */
      qsort(ptr, nrows, (sizeof(I_Entry)), 
			  (int (__cdecl *)(const void *, const void *))select_cmp(f));
	  #else
      qsort(ptr, nrows, (sizeof(I_Entry)), 
			  (int (*)(const void *, const void *))select_cmp(f));
	  #endif /* _WINDOWS */

	
      Write_Index(f, sub, nrows, ptr);
		free(ptr);
#if DEBUG
/*TESTING PURPOSES ONLY*/
   p = strdup(ListElement(char *, table->files, table->buff->fileidx));
   strcpy(p + strlen(p) - 3, "idx");
	Driver1(p,f,sub);
	Driver2(p,f,sub);
/*TESTING PURPOSES ONLY*/
#endif


      fprintf(stderr, "%s\n", ((char **)table->files->ptr)[table->buff->fileidx]);

      rhnd = RefillTblBuff(table->buff);
   	if (RPTR(rhnd) ==NULL)
			break;
	}


   return 1;

}


void 
Make_Index(char *fields_str, LIST *tables)
{
    char buf[1024];
    char *q, *p;
    OSTRUCT *o;
    FIELD *f;
    int st, ed, i;
    int subscript;
    LIST *output = new_list();
    char *lb = NULL, *rb = NULL;
    
    if (fields_str == NULL)
        return;

    strcpy(buf, fields_str);
    q = buf;
    while ((p = strtok(q, " \t\n")) != NULL) {
        q = NULL;
        if ((f = FindField(p, tables)) == NULL) {
            fprintf(stderr, "Unable to find specified field: %s\n", p);
            continue;
        }

        if (f->vardata != NULL){ /* variable data field */
            fprintf(stderr, "Variable data fields cannot be indexed: %s\n", p);
            continue;
        }
        
        subscript = 0; /* array subscript */
        if (f->dimension > 0){
            if ((lb = strchr(p, '[')) && (rb = strchr(p, ']')) && rb > lb){
                subscript = atoi(lb+1) - 1;

                if (subscript < 0 || subscript >= f->dimension){
                    fprintf(stderr, "Invalid subscript encountered: %s\n", p);
                    continue;
                }
            }
            else {
                fprintf(stderr, "Cannot make index of an array: %s\n", p);
                fprintf(stderr, "Please specify an index.\n");
                continue;
            }
        }
        
        make_index(f, subscript);
    }
    
	if (index_type_array!=NULL)
		free(index_type_array);
    
} 

int
check_index_type(I_Entry *ptr, int num, FIELD *f)
{
	int i;
	int Type_2_Flag=1;
	int Type_1_Flag=1;
	int Type_3_Flag=0;
	int last_hit=0;
	int *count;
	int r;
	int NV=0;
	int byte_3;
	int byte_4;
	int wad_idx=0;
	int wad_tot=0;

	count=(int *)calloc(num,sizeof(int));
	count[0]++; /*First entry is automatically counted*/
	NV++;


	for (i=1;i<num;i++){

		if (ptr[i].RecNo!=(ptr[i-1].RecNo+1))
			Type_2_Flag=0;	
		
		switch (f->iformat) {

		case STRING:
			r=cmp_string(ptr[i].data.str,ptr[i-1].data.str);
			break;
		case INT:
			r=cmp_int(&ptr[i].data.i,&ptr[i-1].data.i);
			break;
		case UINT:
			r=cmp_uint(&ptr[i].data.ui,&ptr[i-1].data.ui);
			break;
		case REAL:
			r=cmp_real(&ptr[i].data.r,&ptr[i-1].data.r);
		}
		
		if(!(r)){
				count[last_hit]++;
				Type_2_Flag=0;
				Type_1_Flag=0;
				Type_3_Flag=1;
		}	
		else {
				count[i]++;
				last_hit=i;
				NV++;
		}

		

	}

	if (index_type_array != NULL)
		free(index_type_array);

	index_type_array=count;

	if (Type_2_Flag)
		return(2);

	else if (Type_1_Flag)
		return(1);

/*We determin if it's type 3 or type 4*/

	wad_idx=-1;/*First value will trigger an increment...setting this to zero*/
	if (wad_array!=NULL)
		free(wad_array);
	wad_array = (int *) calloc(NV,sizeof(int));
	for (i=0;i<num;i++){
		if(count[i]){ /*Value change (which by defualt means a cluster change) */
			wad_idx++;
			wad_array[wad_idx]++;
			wad_tot++;
		}
		else if (ptr[i-1].RecNo!=(ptr[i].RecNo-1)){ /*Cluster change*/
				wad_array[wad_idx]++;
				wad_tot++;
		}
			
	}

	byte_3=NV*2*sizeof(int)+sizeof(int)*(num+2);
	byte_4=NV*2*sizeof(int)+wad_tot*2*sizeof(int);

	if (byte_3 <= byte_4) {
		free(wad_array);
		wad_array=NULL;	
		return(3);
	}

	if (count_array!=NULL)
		free(count_array);

	count_array=(int *)calloc(wad_tot,sizeof(int));	
	wad_idx=-1;
	for(i=0;i<num;i++){
		if(!(count[i])){
			if (ptr[i-1].RecNo==(ptr[i].RecNo-1))
				count_array[wad_idx]++;
			else{
				wad_idx++;
				count_array[wad_idx]++;
			}
		}
		else {
			wad_idx++;
			count_array[wad_idx]++;
		}
	}

	return(4);

}
	

void
Write_Index(FIELD *f, int sub, int n ,void *ptr)
{
   struct stat sbuf;
   TABLE *t = f->label->table;
   char buf[256], *p;
   int fd, i, rc,j;
	MH_Index	*mih=NULL;
	int type;
	long mh_offset=0;

  
   p = strdup(ListElement(char *, t->files, t->buff->fileidx));
   strcpy(p + strlen(p) - 3, "idx");

	/*Determin Index Type*/ 
	type=check_index_type(ptr,n,f);


 	/* Check for exisiting index files and
	** update INDEX data structure if it exists or install new one 
	** with this index info
	*/
   rc = stat(p, &sbuf);
   if (rc == 0)  {
		fd=open(p,O_RDWR,0777);
      mih = LoadMasterIndex(p);
      if (has_index(mih,f,sub)) {
			/*Replace current index with new index writeout INDEX*/
			Replace_Index(mih,n,ptr,f,sub,type,fd);
      }
		else {
			/*Add new index to current index writeout only new portion*/
			Add_Index(mih,n,ptr,f,sub,type,fd);
		}
   }
	else { /*Build current index (empty) and add new index*/
			fd=open(p,O_RDWR | O_CREAT,0777);
			New_Index(n,ptr,f,sub,type,fd);
	}

	close(fd);

	Destroy_MH_Index(mih);	
}

void
Build_Index_Header(H_Index *ih, int type, FIELD *f, int n)
{

	int i;
	int wc=0;

	ih->type=type;

	switch (type) {
	case 1:
		ih->NV=n;
		ih->Cptr=0;
		ih->Rptr=(n*f->size)+((ALIGN-((n*f->size) % ALIGN))%ALIGN);
		break;
	case 2:
		ih->NV=n;
		ih->Cptr=0;
		ih->Rptr=0;
		break;
	case 3:
		ih->NV=0;
		for (i=0;i<n;i++){
			if(index_type_array[i])
				ih->NV++;
		}
		ih->Cptr=(ih->NV*f->size)+((ALIGN-((ih->NV*f->size) % ALIGN))%ALIGN);
		ih->Rptr=ih->Cptr+(ih->NV*sizeof(int));
		break;
	
	case 4:
		ih->NV=0;
      for (i=0;i<n;i++){
         if(index_type_array[i])
            ih->NV++;
      }
		ih->Cptr=(ih->NV*f->size)+((ALIGN-((ih->NV*f->size) % ALIGN))%ALIGN);
		ih->Rptr=ih->Cptr+ih->NV*sizeof(int);
	}
}

void
Add_Index(MH_Index *mih,int n, void *ptr,FIELD *f, int sub, int type, int fd) 
{
	int i;
	int nf;
	long end_jump;
	int num;
	H_Index 	ih;
	char pad[ALIGN]={0};

	nf=mih->NumFields;	

	/*Update Master Header*/

	mih->f_len=(int *)realloc(mih->f_len,sizeof(int)*(nf+1));
	mih->field=(char **)realloc(mih->field,sizeof(char *)*(nf+1));
    mih->sub=(int *)realloc(mih->sub,sizeof(int)*(nf+1));
	mih->offset=(int *)realloc(mih->offset,sizeof(int)*(nf+1));

	mih->f_len[nf]=strlen(f->name)+((ALIGN-(strlen(f->name) % ALIGN)) % ALIGN);
	mih->field[nf]=(char *)calloc(mih->f_len[nf]+1,sizeof(char));
	strcpy(mih->field[nf],f->name);
	memcpy(mih->field[nf]+strlen(f->name),pad,((ALIGN-(strlen(f->name) % ALIGN)) % ALIGN));
    mih->sub[nf]=sub;

	mih->offset[nf]=mih->Offset;
	mih->NumFields++;



	Build_Index_Header(&ih,type,f,n);

	/*Read in Jmp and move to position for new index*/
	lseek(fd,0,SEEK_SET);
	read(fd,&end_jump,sizeof(long));
	lseek(fd,end_jump,SEEK_SET);

	write(fd,&ih,sizeof(H_Index));
	num=Write_Field_Index(f,sub,n,(I_Entry *)ptr,type,fd);

	/*write out master header now*/

	Write_Master_Index(fd,mih);

	/*Now update jump_val*/
	lseek(fd,0,SEEK_SET);
	end_jump+=sizeof(H_Index)+num; /*Header + Data*/
	write(fd,&end_jump,sizeof(long));

}
void
Write_Master_Index(int fd, MH_Index *mih)
{
	int i;

	write(fd,&mih->NumFields,sizeof(int));

	for(i=0;i<mih->NumFields;i++){	
		write(fd,&mih->f_len[i],sizeof(int));
		write(fd,mih->field[i],mih->f_len[i]);
        write(fd,&mih->sub[i],sizeof(int));
		write(fd,&mih->offset[i],sizeof(int));
	}
}


void 
New_Index(int n, void *ptr,FIELD *f, int sub, int type, int fd)
{
	int i;
	long end_jump=0;
	int num;
	MH_Index mih;
	H_Index	ih;
	char pad[ALIGN]={0};
	char tmp[1024];


	/*Build Master Index*/
	mih.NumFields=1;

	mih.f_len=(int *)calloc(1,sizeof(int));
	mih.f_len[0]=strlen(f->name)+((ALIGN-(strlen(f->name) % ALIGN)) % ALIGN);
	mih.field=(char **)calloc(1,sizeof(char *));
	mih.field[0]=(char *)calloc(mih.f_len[0]+1,sizeof(char));
	strcpy(mih.field[0],f->name);
	memcpy(mih.field[0]+strlen(f->name),pad,((ALIGN-(strlen(f->name) % ALIGN)) % ALIGN));
    mih.sub=(int *)calloc(1,sizeof(int));
    mih.sub[0]=sub;
	mih.offset=(int *)calloc(1,sizeof(int)); 
	mih.offset[0]=ALIGN;


	/*Build and write Index header*/

	Build_Index_Header(&ih,type,f,n);

	/*Write out index values*/
	write(fd,&end_jump,sizeof(long)); /*Dummy value; used as placeholder*/
	end_jump=0xFFFF;
	write(fd,&end_jump,sizeof(long)); /*Dummy value; used to extend placeholder*/

	write(fd,&ih,sizeof(H_Index));
	num=Write_Field_Index(f,sub,n,(I_Entry *)ptr,type,fd);

	/*write out master header now*/
	Write_Master_Index(fd,&mih);

	/*Now update jump_val*/
	lseek(fd,0,SEEK_SET);
	end_jump=ALIGN+sizeof(H_Index)+num; /*jump_vec(ALIGN-byte)+header+data*/
	write(fd,&end_jump,sizeof(long));

}

void 
Replace_Index(MH_Index *mih,int n, void *ptr,FIELD *f,int sub,int type, int fd)
{
	int i,j;
	int modif;
	int num;
	int end_jmp;
	int num_index;
	int start_index;
	H_Index ih;
	int *chunk;
	uchar **ChunkBuf;
    int found;

	/*Search Master Header for index position; we'll skip over everything before it,
		but we'll have to read in everything after it. However, we can read 
		the indecies which follow as just huge chunks since we don't need to 
		process them*/

    found = 0;
	for (i=0;i<mih->NumFields;i++){
		if (!(strncmp(f->name,mih->field[i],mih->f_len[i]))){
            if (f->dimension > 0){
                if (sub == mih->sub[i]){
                    found = 1;
                }
            }
            else {
                found = 1;
            }

            if (found){
                num_index=mih->NumFields-i;
                start_index=i;
                break;
            }
		}
	}	

	/*Set up storage to read in the our replaced index and all the indices that follow */

	ChunkBuf=(uchar **)calloc(num_index,sizeof(uchar *));
	chunk=(int *)calloc(num_index,sizeof(int));

	/* Need to collect the offset of the master header, this will give us the
	**	final piece of size bounding formation for the last index
	*/	

	lseek(fd,0,SEEK_SET);
	read(fd,&end_jmp,sizeof(long));

	/*Now read each index, including it's header as just a big chunk*/
	for (i=0;i<num_index-1;i++){
		lseek(fd,mih->offset[i+start_index],SEEK_SET);
		chunk[i]=mih->offset[i+start_index+1]-mih->offset[i+start_index];
		ChunkBuf[i]=(uchar *)calloc(chunk[i],sizeof(uchar));
		read(fd,ChunkBuf[i],chunk[i]);
	}
	/* Now the last index in the bunch
	** i should now = num_index-1 
	*/
	chunk[i]=end_jmp-mih->offset[i+start_index];
	ChunkBuf[i]=(uchar *)calloc(chunk[i],sizeof(uchar));
	read(fd,ChunkBuf[i],chunk[i]);

	lseek(fd,mih->offset[start_index],SEEK_SET);

	/*Now write out our new header and index*/
	Build_Index_Header(&ih,type,f,n);
	write(fd,&ih,sizeof(H_Index));
	num=Write_Field_Index(f,sub,n,ptr,type,fd);

	modif=(num+sizeof(H_Index))-chunk[0]; /*difference in size*/
	chunk[0]+=modif;								/*Update size of new chunk*/

	/*update the Master Header*/
	for (i=(start_index+1);i<mih->NumFields;i++){
		mih->offset[i]+=modif;
	}

	/*Write back all the chunks (but the first one!)*/
	modif=0;
	for (i=1;i<num_index;i++){
		write(fd,ChunkBuf[i],chunk[i]);
		modif+=chunk[i];
	}	
	modif+=chunk[0];

	/*Write out the Master header*/

	Write_Master_Index(fd,mih);

	/*Update the jump vector at the start*/
	end_jmp=mih->offset[start_index]+modif;
	lseek(fd,0,SEEK_SET);
	write(fd,&end_jmp,sizeof(long));

	for (i=0;i<num_index;i++){
		free(ChunkBuf[i]);
	}
	free(ChunkBuf);
	free(chunk);
}

int
has_index(MH_Index *mih, FIELD *f, int sub)
{
   int i;
	for (i=0;i<mih->NumFields;i++){
		if (!(strncmp(f->name,mih->field[i],mih->f_len[i]))){
            if (f->dimension > 0){
                if (sub == mih->sub[i]){
                    return(mih->offset[i]);
                }
            }
            else {
                return(mih->offset[i]);
            }
        }
	}
   return(0);
} 


/**
 ** Loads the master catalog of what indices exist in an index
 ** file.
 **/
MH_Index *
LoadMasterIndex(char *filename)
{
	long offset;
	int i;
	int fd;
	MH_Index *mih;

	if ((fd=open(filename,O_RDONLY,0777))==-1)
		return(NULL);

	mih=(MH_Index *)calloc(1,sizeof(MH_Index));

	/*Get Master header offset at end of file (yea, I know it's supposed to be a header)*/

	read(fd,&offset,sizeof(long));
	mih->Offset=offset;

	lseek(fd,offset,SEEK_SET);

	read(fd,&mih->NumFields,sizeof(int));

	mih->f_len=(int *)calloc(mih->NumFields,sizeof(int));
	mih->field=(char **)calloc(mih->NumFields,sizeof(char *));
    mih->sub=(int *)calloc(mih->NumFields,sizeof(int)); /* field subscript */
	mih->offset=(int *)calloc(mih->NumFields,sizeof(int));

	for (i=0;i<mih->NumFields;i++){
		read(fd,&mih->f_len[i],sizeof(int));
		mih->field[i]=(char *)calloc(mih->f_len[i],sizeof(char));
		read(fd,mih->field[i],mih->f_len[i]);
        read(fd,&mih->sub[i],sizeof(int));
		read(fd,&mih->offset[i],sizeof(int));
	}
	close(fd);
	return (mih);
}

/**
 ** Loads the index for a particular field from the master catalog.
 **/
Index *LoadFieldIndex(
    FIELD *f,  /* field to be searched */
    int sub,   /* zero-based subscript into the field "f" (for array fields) */
    MHeader *m /* already populated MHeader */
    )
{
	int	f_len;
	int 	i;
	int	type;
	int	NV;
	int	ptr=0;
	uchar *buf;


	Index	*Idx;
	H_Index ih;

	MHeader *mh=m;

    int found;
    

	Idx=(Index *)calloc(1,sizeof(Index));

	/*Search the Master Header of Handle for Field name*/

    found = 0;
	for (i=0;i<(mh->NumFields);i++){
		if (!(strncmp(f->name,mh->field[i],*(mh->f_len[i])))) { /*Found it!*/
            if (f->dimension > 0){
                if (sub == *(mh->sub[i])){ /* same subscript: for array field */
                    found = 1;
                }
            }
            else {
                found = 1;
            }

            if (found){
                ptr=*(mh->offset[i]);
                buf=mh->map;
                break;
            }
		}
	}

	if (i==mh->NumFields) { /*Bad news, didn't/couldn't find it*/
/*		fprintf(stderr,"Cant find field %s in Index Map of Handle %d\n",f->name,Handle);*/
		return(NULL);
	}

	memcpy(&ih,(buf+ptr),sizeof(H_Index));
	ptr+=sizeof(H_Index);

	Idx->Type=ih.type;
	Idx->NV=ih.NV;
/*	Idx->Rptr=ih.Rptr; */
/*	Idx->Cptr=ih.Cptr; */

	switch (Idx->Type) {

	case 1:
			Idx->It.t1.Value=(buf+ptr);
			Idx->It.t1.RecNo=(int *)((buf+ptr)+(ih.Rptr));
			break;

	case 2:
			Idx->It.t2.Value=(buf+ptr);
			break;

	case 3:
			Idx->It.t3.Value=(buf+ptr);
			Idx->It.t3.Count=(int *)((buf+ptr)+(ih.Cptr));
			Idx->It.t3.RecNo=(int *)((buf+ptr)+(ih.Rptr));
			break;

	case 4:
		Idx->It.t4.Value=(buf+ptr);
		Idx->It.t4.WadCount=(int *)((buf+ptr)+(ih.Cptr));
		Idx->It.t4.RecCount=(int *)((buf+ptr)+(ih.Rptr));
		break;

	}

	return(Idx);
}



