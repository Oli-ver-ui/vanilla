#ifndef _PROTO_H
#define _PROTO_H

#include "header.h"

LABEL *LoadLabel(char *);
DATASET * LoadDataset(DATASET *dataset, char *fname);
DATASET *FakeDataset(LIST *files_list);
int FakeTable(DATASET *dataset, LIST *files, char *name);
FIELD *FindField(char *name, LIST * labels);
FIELD *FindFieldInLabel(char *name, LABEL * l);
FRAGHEAD *LoadFragHead(char *fname, TABLE *table);
void FreeFragHead(FRAGHEAD *f);
void FreeFragment(FRAGMENT *f);

DATA ConvertASCIItoData(char *ascii, int i);
DATA ConvertField(char *ptr, FIELD *f);
LIST *LoadFilenames(char *path, char *prefix);

DATA ConvertData(PTR ptr, FIELD *f);
DATA ConvertFieldData(PTR ptr, FIELD *f);
DATA ConvertVarData(PTR ptr, VARDATA *v);
int EquivalentData(DATA d1, DATA d2, FIELD *f);
int CompareData(DATA d1, DATA d2, FIELD *f);
DATA *maxFieldVal(SLICE * s, int dim, TABLE **tbl, DATA *maxValue);

void Make_Index(char *fields_str, LIST *tables);
int ConvertSelect(DATASET * d, char *sel_str);
void search(int deep, int maxdepth, SLICE ** slice, TABLE ** tbl, int tcount);
void output_rec(OSTRUCT **o, int n);
void SortFiles(LIST *list);
RHND RefillTblBuff(TBLBUFF *b);

FRAGMENT *AllocFragment();
FRAGIDX *NewFragIdx();
void FreeFragIdx(FRAGIDX *fragidx);
FRAGIDX *LoadFragIdxAndMarkUsedSelects(char *fragname, LIST *selects, char *tblname, int nrows);
void SelectResetIdxPPFlag(SELECT **s, int ns);

/* TBLBUFF *NewTblBuff(TABLE * t, size_t reccount, size_t overcount); */
TBLBUFF *NewTblBuff(TABLE *t);
RHND GetFirstRec(TABLE * t);
RHND GetEndOfFrag(TBLBUFF *b);
RHND GetNextRec(RHND rhnd);
RHND find_jump(TABLE * t, FIELD * f, DATA d, RHND beg, RHND end, int deep);
RHND find_until(TABLE * t, FIELD * f, RHND beg, RHND end);
RHND find_select(TABLE * t, RHND beg, RHND end);

int sequence_keys(SEQ * keyseq, TABLE ** tables, int num_tables);
SLICE ** init_slices(TABLE ** tables, int tcount, SEQ keyseq);

LIST * ConvertOutput(char *output_str, LIST *tables);
char * find_file(char *fname);
short ConvertVaxVarByteCount(PTR raw, VARDATA *vdata);

FIELD * FindFakeField(char *name, LIST *tables);
double ConvertAndScaleData(PTR raw, FIELD * field);

PTR   GiveMeVarPtr(PTR raw, TABLE *table, int offset);

#ifdef LOGGING

void invocation_log_on_exit(int exit_status);
void invocation_log_start(int ac, char **av);
void invocation_log_dataset(char *dataset);
void invocation_log_files(char *file);

#define exit invocation_log_on_exit
#endif /* LOGGING */

#endif /* _PROTO_H */
