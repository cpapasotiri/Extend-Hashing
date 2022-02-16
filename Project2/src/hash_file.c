#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include "sht_file.h"
#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20
#pragma once

#define CALL_BF(call){           \
    BF_ErrorCode code = call;   \
    if(code != BF_OK){          \
        BF_PrintError(code);    \
        return HP_ERROR;        \
    }                           \
}                               \

void RecordWrite(Record* record){
    printf("Record with: id->%d, name->%s, surname->%s, city->%s\n",record->id, record->name, record->surname, record->city);
}

int flag=0; //global variable gia to update

int* IndexCreate(int noBuckets){
    int* index = NULL;
    index = malloc(noBuckets*sizeof(int));
    for(int i = 0; i < noBuckets; i++){
        index[i] = i+2;
    }
    return index;
}

int hashFunction(int id, int depth){
    int temp=id;
    int j=0;
    int binary[32];
    for(int i=0; i<32; i++){
      binary[i]=0;
    }
  while(temp>0){
    binary[j++]=temp%2;
    temp=temp/2;
  }
  if(depth>j){
    j=depth;
    //printf("Something went wrong\n");
  }
  temp=0;
  j--;
  for(int i=1; i<=depth; i++){
    temp=temp+binary[j--]*(int)pow(2,depth-i);
  }
 // printf("num=%d  depth=%d    hash=%d\n",sum,depth,temp);
  return temp;
}

static hashInfo HashInfo[MAX_OPEN_FILES];
 int count;

HT_ErrorCode HT_Init(){
    int count = 0;
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        HashInfo[i].filedesc = -1;
    }
    return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  //insert code here
  int filedesc, noBuckets = 0;
  BF_Block *block;
  BF_Block_Init(&block);
  
  if(BF_CreateFile(filename) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  if(BF_OpenFile(filename, &filedesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  noBuckets = (int)pow(2.0, depth);
  for(int i = 0; i < noBuckets+2; i++){
    if(BF_AllocateBlock(filedesc, block) != BF_OK){
      BF_PrintError(BF_ERROR);
      return HT_ERROR;
    }
      if(i>=1){
        char* data = BF_Block_GetData(block);
        memcpy(data, &depth, sizeof(int));   //to local depth olwn ton data blocks
        memset(data+sizeof(int),0,sizeof(int));  //grafoyme ton arithmo ton eggrafon kathe bucket
        BF_Block_SetDirty(block);
        BF_UnpinBlock(block);
      }
  }

  if(BF_GetBlock(filedesc, 0, block) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  char* data = BF_Block_GetData(block);
  memcpy(data , &depth,sizeof(int));   //apothikevoume to global_depth sto proto block 
  BF_Block_SetDirty(block);

  if(BF_UnpinBlock(block) != BF_OK){
    BF_PrintError(BF_UnpinBlock(block));
    return BF_ERROR;
  }

  if(BF_GetBlock(filedesc, 1, block) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }
  
  //apothikevoume ton pinaka me ta index sto deftero block
  int *array=IndexCreate(noBuckets);
  data=BF_Block_GetData(block);
  memcpy(data,array,noBuckets*sizeof(int));
  free(array);
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);

  if(BF_CloseFile(filedesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }
  BF_Block_Destroy(&block);
  printf("Succesfully created Hash Table File \"%s\" with %d buckets.\n", filename, noBuckets);
  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  //insert code here
  BF_Block *block;
  BF_Block_Init(&block);

  if((BF_OpenFile(fileName, indexDesc)) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  if(count == MAX_OPEN_FILES){
    for(int i = 0; i < count; i++){
      if(HashInfo[count].filedesc == -1){
        HashInfo[count].filedesc = *indexDesc;
        HashInfo[count].filename = fileName;
        *indexDesc = count;
        count++;
      }
    }
  }
  else if(count < MAX_OPEN_FILES){
    HashInfo[count].filedesc = *indexDesc;
    HashInfo[count].filename = fileName;
    *indexDesc = count;
    count++;
  }
  else{
    printf("There is no more space\n");
  }

  printf("Succesfully opened index with id:%d\n",*indexDesc);
  BF_Block_Destroy(&block);      //prepei
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
  //insert code here
  int desc = indexDesc;
  indexDesc = HashInfo[indexDesc].filedesc;
  char* name = HashInfo[indexDesc].filename;
  if(BF_CloseFile(indexDesc) != BF_OK){
    BF_PrintError(BF_CloseFile(indexDesc));
    return HT_ERROR;
  }

  int i;
  HashInfo[desc].filedesc = -1;
  //replace the last one 
  count--;
  printf("Succesfully Closed file with filename:%s and fileid:%d\n",name, indexDesc);
  
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record, int* typleID,UpdateRecordArray* updateArray) {
 
  int k=HashInfo[indexDesc].filedesc;
  //diavazoume to global depth apo to block 0 tou arxeiou
  BF_Block* block;
  BF_Block_Init(&block);
  BF_GetBlock(k,0,block);
  char* data;
  data=BF_Block_GetData(block);
  int global_depth;
  memcpy(&global_depth,data,sizeof(int));
  if(global_depth>6){
    //printf("No more insertions can be made\n");
    return HT_OK;
  }
 // printf("global_depth=%d\n",global_depth);
  //kaloume th hash_function
  int hashValue=hashFunction(record.id,global_depth);
  //printf("HashValue=%d\n",hashValue);
  BF_GetBlock(k,1,block); //pairnoume to evretirio
  data=BF_Block_GetData(block);
  int* array=data;
 // printf("rererer %d\n",*(array+hashValue));
  BF_GetBlock(k,*(array+hashValue),block);
  int p=*(array+hashValue);
  data=BF_Block_GetData(block);
  int noRecords;
  memcpy(&noRecords,data+sizeof(int),sizeof(int));
  //printf("noRecords=%d\n",noRecords);
 
  if(noRecords<8){         //kathe block xoraei 8 eggrafes
    memcpy(data+2*sizeof(int)+noRecords*sizeof(Record),&record,sizeof(Record));
    noRecords++;
    memcpy(data+sizeof(int),&noRecords,sizeof(int));
    *typleID=8*(*(array+hashValue)-2)+noRecords;
    //printf("INSERTED\n");
  }
 
  else{
    flag=1;
    //printf("case 2\n");
    int local_depth;
    memcpy(&local_depth,data,sizeof(int));
    if(global_depth==local_depth){
     // printf("1\n");
      //printf("1\n");
     // printf("rererrerer\n");
      global_depth++;
      //printf("%d\n",global_depth);
      BF_Block* block2;
      BF_Block_Init(&block2);
      BF_GetBlock(k,0,block2);
      char* data2=BF_Block_GetData(block2);
      memcpy(data2,&global_depth,sizeof(int));
      //diplasiazoume to megethos toy evretiriou
      int w=(int)pow(2,global_depth);
     // printf("w=%d\n",w);
      int *newArray=malloc(w*sizeof(int));
      int count=0;
      int i=0;
      while (count<w){
        newArray[count]=array[i];
        newArray[count+1]=array[i];
        count=count+2;
        i++;
      }
      //kanoume allocate ena kainourio block
      BF_Block* block3;
      BF_Block_Init(&block3);
      BF_AllocateBlock(k,block3);
      char* data3=BF_Block_GetData(block3);
      int new_depth;
      new_depth=local_depth+1;
      memcpy(data3,&new_depth,sizeof(int));  //to local_depth tou kainouriou bucket 
      memset(data3+sizeof(int),0,sizeof(int));    // to kainourio bucket den exei akoma eggrafes ara noRecords=0
      BF_Block_SetDirty(block3);
      BF_UnpinBlock(block3);
      int num_of_Blocks;
      BF_GetBlockCounter(k,&num_of_Blocks);
      int l;
      l=global_depth-local_depth;
      int y=(int)pow(2,l); //ipologizo to 2^l
      //xero oti 2^l theseis toy pinaka deinoun ston idio kado 
      count=0;
      int ptr=0;
      //isomoirazo ta velakia se dio isa iposinola
      while(count<w){
        if(newArray[count]==*(array+hashValue)){
          ptr=count;
          break;
        }
        count++;
      }
      count=0;
      while(count<(y/2)){
        newArray[ptr]=num_of_Blocks-1;
        ptr++;
        count++;
      }
      BF_GetBlock(k,1,block2);
      data2=BF_Block_GetData(block2);
      memcpy(data2,newArray,w*sizeof(int));
      BF_Block_SetDirty(block2);
      BF_UnpinBlock(block2);
      
    
    //apothikevo tis metaavlites toy gematou kado se enan pinaka gia na tis xanaperaso
    Record *records=malloc(8*sizeof(Record));
    for(int i=0; i<8; i++){
      memcpy(&records[i],data+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
    }
    
    memcpy(data,&new_depth,sizeof(int));  //to local_depth toy afxanete kata 1 afoy pleon dixnoun oi mises theseis sto kado
    memset(data+sizeof(int),0,sizeof(int)); //midenizo tis egrafes tou bucket
    memset(data+2*sizeof(int),0,8*sizeof(Record));
    
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    
   

    for(int i=0; i<8; i++){
      strcpy(updateArray[i].city,records[i].city);
      strcpy(updateArray[i].surname,records[i].surname);
      updateArray[i].oldTupleId=8*(p-2)+i+1;    
      HT_InsertEntry(indexDesc,records[i],typleID,updateArray);  //xanapername oles tis eggrafes
      updateArray[i].newTupleId=*typleID;
    }
    
    free(records);
    HT_InsertEntry(indexDesc,record,typleID,updateArray);
   // return HT_ERROR;
  BF_Block_SetDirty(block2);
  BF_UnpinBlock(block2);
  BF_Block_Destroy(&block2);
  BF_Block_SetDirty(block3);
  BF_UnpinBlock(block3);
  BF_Block_Destroy(&block3);
  }
  

  
  else{    //periptosi local_depth<global_depth
  //kanoume allocate ena kainoyrio block
  //printf("2\n");
  BF_Block* block2;
  BF_Block_Init(&block2);
  BF_AllocateBlock(k,block2);
  char* data2=BF_Block_GetData(block2);
  int new_depth=local_depth+1;
  memcpy(data2,&new_depth,sizeof(int)); //to depth tou kainourio bucket einai local_depth+1
  memset(data2+sizeof(int),0,sizeof(int));  // to kainourio bucket arxika den periexei eggrafes 
  memcpy(data,&new_depth,sizeof(int)); //to local depth toy gematou bucket afxanete kata 1
  
  
  //apothikevo tis eggrafes tou gematou bucket se ena pinaka gia na tis ksanakano insert
  Record *records=malloc(8*sizeof(Record));
  for(int i=0; i<8; i++){
    memcpy(&records[i],data+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
  }
  memset(data+sizeof(int),0,sizeof(int));  //midenizoume ton arithmo eggrafon toy gematou bucket afou prokeitai na tis xanaperasoume oles 
  memset(data+2*sizeof(int),0,8*sizeof(Record));

  //pairnoume to evretirio 
  BF_GetBlock(k,1,block2);
  data2=BF_Block_GetData(block2);
  // prepei na isomoirasoume ta velakia 
  int w=(int)pow(2,global_depth);
 // int *newArray=malloc(w*sizeof(int));
  int l=global_depth-local_depth; // xeroume oti 2^l theseis tou pinaka edeixan ston gemato kado 
  l=(int)pow(2,l);
  count=0;
  int ptr;
  int number;
  BF_GetBlockCounter(k,&number);
  while(count<w){
    if(array[count]==*(array+hashValue)){
      ptr=count;
      break;
    }
    count++;
  }
  count=0;
  while(count<l/2){
    array[ptr]=number-1;
    ptr++;
    count++;
  }
  memcpy(data2,array,w*sizeof(int));
  for(int i=0; i<8; i++){
      
      strcpy(updateArray[i].city,records[i].city);
      strcpy(updateArray[i].surname,records[i].surname);
      updateArray[i].oldTupleId=8*(p-2)+i+1;
      HT_InsertEntry(indexDesc,records[i],typleID,updateArray);  //xanapername oles tis eggrafes
      updateArray[i].newTupleId=*typleID;
  }
  
  HT_InsertEntry(indexDesc,record,typleID,updateArray);
 // printf("Succesfully inserted record at the secondary index\n");
  BF_Block_SetDirty(block2);
  BF_UnpinBlock(block2);
  BF_Block_Destroy(&block2);
}

//tipono to evretirio gia epalithefsi
/*BF_Block *block4;
BF_Block_Init(&block4);
BF_GetBlock(k,1,block4);
  int* a;
  data=BF_Block_GetData(block4);
  a=data;
  //printf("%d\n",(int)pow(2,global));
  for(int i=0; i<(int)pow(2,global_depth); i++){
    printf("%d\n",*(a+i));
  }*/
} 
BF_Block_SetDirty(block);
BF_UnpinBlock(block);
BF_Block_Destroy(&block);
return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  indexDesc = HashInfo[indexDesc].filedesc;
  int noBlocks, noRecords = 0;
  BF_Block *block;
  BF_Block_Init(&block);
  Record* record;
  char* data;
  BF_GetBlockCounter(indexDesc, &noBlocks);
  record = malloc(sizeof(Record));
  if(id == NULL){
    for(int i = 2; i < noBlocks; ++i){
      if(BF_GetBlock(indexDesc, i, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      if((data=BF_Block_GetData(block)) == NULL){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
     
       memcpy(&noRecords, data+sizeof(int), sizeof(int));
      printf("Block=%d\n",i);
      for(int j = 0; j < noRecords; j++){
        memcpy(record, data + 2*sizeof(int) + j*sizeof(Record), sizeof(Record));
        RecordWrite(record);
      }
      BF_UnpinBlock(block);
    }
  }
  else{
    for(int i = 2; i < noBlocks; ++i){
      if(BF_GetBlock(indexDesc, i, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      if((data=BF_Block_GetData(block)) == NULL){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      
      memcpy(&noRecords, data+sizeof(int), sizeof(int));
      
      for(int j = 0; j < noRecords; j++){
        memcpy(record, data + 2*sizeof(int) + j*sizeof(Record), sizeof(Record));
        if(record->id == *id){
          printf("Succesfully found a record with id:%d\n",record->id);
          RecordWrite(record);
        }
      }
      BF_UnpinBlock(block);
    }
  }
  free(record);
  BF_Block_Destroy(&block);
  //printf("Succesfully printed all entries\n");
  return HT_OK;
}

HT_ErrorCode HashStatistics(char* filename){
  int indexDesc;
  int flag=0;
  for(int i=0; i<MAX_OPEN_FILES; i++){
    if(HashInfo[i].filedesc!=-1){
       if(HashInfo[i].filename==filename){        
        indexDesc=i;
        flag=1;         //vrikame to arxeio me onoma filename
        break;
      }
    }
  }
  if(flag==0){
    printf("No such file\n");
    return HT_ERROR;
  }
  int fileDesk=HashInfo[indexDesc].filedesc;
  BF_Block *block;
  BF_Block_Init(&block);
  if(BF_GetBlock(fileDesk,0, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
  char* data=BF_Block_GetData(block);
  int global_depth;
  memcpy(&global_depth,data,sizeof(int));
  BF_UnpinBlock(block);
  int noBlocks;
  BF_GetBlockCounter(fileDesk,&noBlocks);
  int sum=0;
  int min=9;
  int max=-1;
  int noRecords;
  for(int i=2; i<noBlocks; i++){
    if(BF_GetBlock(fileDesk,i, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      data=BF_Block_GetData(block);
      memcpy(&noRecords,data,sizeof(int));
      BF_UnpinBlock(block);
      sum=sum+noRecords;
      if(noRecords>max){
        max=noRecords;
      }
      if(noRecords<min){
        min=noRecords;
      }
      BF_UnpinBlock(block);
  }
  BF_Block_Destroy(&block);
  sum=sum/(noBlocks-2);
  printf("Number of blocks=%d  max=%d and min=%d and average=%d\n",noBlocks,max,min,sum);
  return HT_OK;
}