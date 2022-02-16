#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

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

int* IndexCreate(int noBuckets){
    int* index = NULL;
    index = malloc(noBuckets*sizeof(int));
    for(int i = 0; i < noBuckets; i++){
        index[i] = i+2;
    }
    return index;
}

int hashFunction(int id, int depth){
    return (((1 << depth) - 1) & (id >> ((10-depth) - 0)));
}

static hashInfo HashInfo[MAX_OPEN_FILES];
static int count;

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

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  
  indexDesc = HashInfo[indexDesc].filedesc;

  BF_Block* block;
  BF_Block_Init(&block);
  BF_GetBlock(indexDesc,0,block);
  char* data=BF_Block_GetData(block);
  int global_depth;
  memcpy(&global_depth,data,sizeof(int));     //pernoume to global depth apo to proto block toy arxeioy  
  BF_UnpinBlock(block);
  int hashValue=hashFunction(record.id,global_depth);
  BF_GetBlock(indexDesc,1,block);
  data=BF_Block_GetData(block);
  int *array=data;
  BF_UnpinBlock(block);
  BF_GetBlock(indexDesc,*(array+hashValue),block);
  data=BF_Block_GetData(block);
  int noRecords;
  int k=0;
  memcpy(&k,data,sizeof(int));
  memcpy(&noRecords,data+(sizeof(int)),sizeof(int));
  if(noRecords!=8){
    memcpy(data+2*sizeof(int)+noRecords*sizeof(Record),&record,sizeof(Record));
    noRecords++;
    memcpy(data+sizeof(int),&noRecords,sizeof(int));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
  }
  else{
    int local_depth=0;
    memcpy(&local_depth,data,sizeof(int));
    
    if(local_depth==global_depth){
      if(global_depth>=7){       //de mporoume na epekteinoume allo to hash table giati tha kseperasei to megethos enos block
        BF_Block_Destroy(&block);
        return HT_OK;
      }
      
      BF_Block* block2;
      BF_Block_Init(&block2);
      if(BF_AllocateBlock(indexDesc, block2) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      data=BF_Block_GetData(block2);
     
      memcpy(data,&global_depth,sizeof(int));
      int a=0;
      memset(data+sizeof(int),0,sizeof(int));
      BF_Block_SetDirty(block2);
      BF_UnpinBlock(block2);
       global_depth++;
      BF_GetBlock(indexDesc,0,block2);
      data=BF_Block_GetData(block2);
      memcpy(data,&global_depth,sizeof(int));
      BF_Block_SetDirty(block2);
      BF_UnpinBlock(block2);
      
      int n=(int)(pow(2,global_depth)+0.5);
      
      int* newArray=malloc(n*sizeof(int));
      int i=0;
      int j=0;
      int w=0;
      while(i<n/2){
        newArray[j]=array[i];
        newArray[j+1]=array[i];
        
        if(i==hashValue){
          BF_GetBlockCounter(indexDesc,&w);
          newArray[i]=w-1;
          
        }
        i++;  
        j=j+2;
      }
      BF_GetBlock(indexDesc,*(array+hashValue),block);
      data=BF_Block_GetData(block);
      Record* records;
    
      
      records=malloc(noRecords*(sizeof(Record)));
     
      for(int i=0; i<noRecords; i++){
        memcpy(&records[i],data+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
        //RecordWrite(&records[i]);
        memset(data+2*sizeof(int)+i*sizeof(Record),0,sizeof(Record));
      }
      

      memset(data+sizeof(int),0,sizeof(int));   //0 eggrafes 

    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    BF_GetBlock(indexDesc,1,block2);
    data=BF_Block_GetData(block2);
    memcpy(data,newArray,n*sizeof(int));
    free(newArray);
    BF_Block_SetDirty(block2);
    BF_UnpinBlock(block2);
    
    for(int i=0; i<noRecords; i++){
        HT_InsertEntry(indexDesc,records[i]);
    }
      HT_InsertEntry(indexDesc,record);
      
    free(records);
    BF_Block_Destroy(&block2);
    }
    else{
      local_depth=local_depth+1;
      Record* records;
     
      records=malloc(noRecords*sizeof(Record));
      for(int i=0; i<noRecords; i++){
        memcpy(&records[i],data+2*sizeof(int)+i*sizeof(Record),sizeof(Record));
        memset(data+2*sizeof(int)+i*sizeof(Record),0,sizeof(Record));
      }
      
      memcpy(data,&local_depth,sizeof(int));
      memset(data+sizeof(int),0,sizeof(int));
      BF_Block_SetDirty(block);
      BF_UnpinBlock(block);
      BF_Block* block3;
      BF_Block_Init(&block3);
      BF_AllocateBlock(indexDesc,block3);
      data=BF_Block_GetData(block3);
      memcpy(data,&global_depth,sizeof(int));
      memset(data+sizeof(int),0,sizeof(int));
      BF_Block_SetDirty(block3);
      BF_UnpinBlock(block3);
      BF_GetBlock(indexDesc,1,block3);
      data=BF_Block_GetData(block3);
      int number;
      BF_GetBlockCounter(indexDesc,&number);
     
      array[hashValue]=number-1;
      int n=(int)(pow(2,global_depth)+0.5);
      
      memcpy(data,array,n*sizeof(int));
      BF_Block_SetDirty(block3);
      BF_UnpinBlock(block3);
      for(int i=0; i<noRecords; i++){
        
        HT_InsertEntry(indexDesc,records[i]);
        
      }
        HT_InsertEntry(indexDesc,record);
        BF_Block_Destroy(&block3);
        free(records);

    }
  }
    


  
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
  printf("Succesfully printed all entries\n");
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