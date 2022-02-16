#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "sht_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

#pragma once

static shashInfo SHashInfo[MAX_OPEN_FILES];

//na kratame kai ena kleidi pros to protevon eyretirio
int count1;

HT_ErrorCode SHT_Init(){
    int count1 = 0;
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        SHashInfo[i].filedesc = -1;
    }
    return HT_OK;
}

HT_ErrorCode SHT_CreateSecondaryIndex(const char *sfileName, char *attrName, int attrLength, int depth,char *fileName ) {
  //insert code here
  int filedesc, noBuckets = 0;
  BF_Block *block;
  BF_Block_Init(&block);

  if(BF_CreateFile(sfileName) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  if(BF_OpenFile(sfileName, &filedesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  noBuckets = (int)pow(2.0, depth);
  for(int i = 0; i < noBuckets+2; i++){
    if(BF_AllocateBlock(filedesc, block) != BF_OK){
      BF_PrintError(BF_ERROR);
      return HT_ERROR;
    }
      if(i>1){                       //nomizo thelei > kai oxi >=

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
  
  memcpy(data,&depth,sizeof(int));
  int size=strlen(attrName)+1;
  memcpy(data+sizeof(int),&size,sizeof(int));
  memcpy(data+2*sizeof(int),attrName,size*sizeof(char));
 
  BF_Block_SetDirty(block);
  if(BF_UnpinBlock(block) != BF_OK){
    BF_PrintError(BF_UnpinBlock(block));
    return BF_ERROR;
  }

  if(BF_GetBlock(filedesc, 1, block) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  int* array = IndexCreate(noBuckets);
  data = BF_Block_GetData(block);
  memcpy(data, array, noBuckets*sizeof(int));
  free(array);
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);

    if(BF_CloseFile(filedesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }
  BF_Block_Destroy(&block);

  printf("Succesfully created Secondary Hash Table File \"%s\" with %d buckets.\n", fileName, noBuckets);

  return HT_OK;
}

HT_ErrorCode SHT_OpenSecondaryIndex(const char *sfileName, int *indexDesc ) {
  //insert code here
  BF_Block *block;
  BF_Block_Init(&block);

  if(BF_OpenFile(sfileName,indexDesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  if(count1 == MAX_OPEN_FILES){
    for(int i = 0; i < count1; i++){
      if(SHashInfo[count1].filedesc == -1){
        SHashInfo[count1].filedesc = *indexDesc;
        SHashInfo[count1].filename = sfileName;
        *indexDesc = count1;
        count1++;
      }
    }
  }
  else if(count1 < MAX_OPEN_FILES){
    SHashInfo[count1].filedesc = *indexDesc;
    SHashInfo[count1].filename = sfileName;
    *indexDesc = count1;
    count1++;
  }
  else{
    printf("There is no more space\n");
  }

  printf("Succesfully opened Secondary index with id:%d\n",*indexDesc);
  BF_Block_Destroy(&block);      //prepei
  return HT_OK;

  return HT_OK;
}

HT_ErrorCode SHT_CloseSecondaryIndex(int indexDesc) {
  //insert code here
  int desc = indexDesc;
  indexDesc = SHashInfo[indexDesc].filedesc;
  char* name = SHashInfo[indexDesc].filename;
  if(BF_CloseFile(indexDesc) != BF_OK){
    BF_PrintError(BF_ERROR);
    return HT_ERROR;
  }

  SHashInfo[desc].filedesc = -1;
  count1--;
  printf("Succesfully closed the secondary index\n");
  return HT_OK;
}

HT_ErrorCode SHT_SecondaryInsertEntry (int indexDesc,SecondaryRecord record  ) {
  
  //insert code here
  //printf("Insert entry at secondary index\n");
  //printf("desc=%d\n",indexDesc);
  int k=SHashInfo[indexDesc].filedesc;
  //printf("k=%d,\n");
  //diavazoume to global depth apo to block 0 tou arxeiou
  BF_Block* block;
  BF_Block_Init(&block);
  BF_GetBlock(k,0,block);
  char* data;
  data=BF_Block_GetData(block);
  int global_depth;
  memcpy(&global_depth,data,sizeof(int));
  //printf("global_depth=%d\n",global_depth);
  if(global_depth>6){
    printf("No more insertions can be made(secondary)\n");
    return HT_ERROR;
  }
 // printf("global_depth=%d\n",global_depth);
  //kaloume th hash_function
  int hashValue=hash_Function(record,global_depth);
  //printf("HashValue=%d\n",hashValue);
  BF_GetBlock(k,1,block); //pairnoume to evretirio
  data=BF_Block_GetData(block);
  int* array=data;
 // printf("rererer %d\n",*(array+hashValue));
  BF_GetBlock(k,*(array+hashValue),block);
  data=BF_Block_GetData(block);
  int noRecords;
  memcpy(&noRecords,data+sizeof(int),sizeof(int));
 // printf("noRecords=%d\n",noRecords);
 
  if(noRecords<18){         //kathe block xoraei 18 eggrafes
    memcpy(data+2*sizeof(int)+noRecords*sizeof(SecondaryRecord),&record,sizeof(SecondaryRecord));
    noRecords++;
    memcpy(data+sizeof(int),&noRecords,sizeof(int));
    //printf("INSERTED\n");
  }
 
  else{
   
    int local_depth;
    memcpy(&local_depth,data,sizeof(int));
    if(global_depth==local_depth){
      
     // printf("rererrerer\n");
      global_depth++;
      BF_Block* block2;
      BF_Block_Init(&block2);
      BF_GetBlock(k,0,block2);
      char* data2=BF_Block_GetData(block2);
      memcpy(data2,&global_depth,sizeof(int));
      //diplasiazoume to megethos toy evretiriou
      int w=(int)pow(2,global_depth);
     // printf("w=%d\n",w);
      int *newArray=malloc(w*sizeof(int));
      int count1=0;
      int i=0;
      while (count1<w){
        newArray[count1]=array[i];
        newArray[count1+1]=array[i];
        count1=count1+2;
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
      count1=0;
      int ptr=0;
      //isomoirazo ta velakia se dio isa iposinola
      while(count1<w){
        if(newArray[count1]==*(array+hashValue)){
          ptr=count1;
          break;
        }
        count1++;
      }
      count1=0;
      while(count1<(y/2)){
        newArray[ptr]=num_of_Blocks-1;
        ptr++;
        count1++;
      }
      BF_GetBlock(k,1,block2);
      data2=BF_Block_GetData(block2);
      memcpy(data2,newArray,w*sizeof(int));
      
    
    //apothikevo tis metaavlites toy gematou kado se enan pinaka gia na tis xanaperaso
    SecondaryRecord *records=malloc(18*sizeof(SecondaryRecord));
    for(int i=0; i<18; i++){
      memcpy(&records[i],data+2*sizeof(int)+i*sizeof(SecondaryRecord),sizeof(SecondaryRecord));
    }
    
    memcpy(data,&new_depth,sizeof(int));  //to local_depth toy afxanete kata 1 afoy pleon dixnoun oi mises theseis sto kado
    memset(data+sizeof(int),0,sizeof(int)); //midenizo tis egrafes tou bucket
    memset(data+2*sizeof(int),0,18*sizeof(SecondaryRecord));
    
   
    
   

    for(int i=0; i<18; i++){
      SHT_SecondaryInsertEntry(indexDesc,records[i]);  //xanapername oles tis eggrafes
    }
    
    free(records);
    SHT_SecondaryInsertEntry(indexDesc,record);
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
  
  BF_Block* block2;
  BF_Block_Init(&block2);
  BF_AllocateBlock(k,block2);
  char* data2=BF_Block_GetData(block2);
  int new_depth=local_depth+1;
  memcpy(data2,&new_depth,sizeof(int)); //to depth tou kainourio bucket einai local_depth+1
  memset(data2+sizeof(int),0,sizeof(int));  // to kainourio bucket arxika den periexei eggrafes 
  memcpy(data,&new_depth,sizeof(int)); //to local depth toy gematou bucket afxanete kata 1
  
  
  //apothikevo tis eggrafes tou gematou bucket se ena pinaka gia na tis ksanakano insert
  SecondaryRecord *records=malloc(18*sizeof(SecondaryRecord));
  for(int i=0; i<18; i++){
    memcpy(&records[i],data+2*sizeof(int)+i*sizeof(SecondaryRecord),sizeof(SecondaryRecord));
  }
  memset(data+sizeof(int),0,sizeof(int));  //midenizoume ton arithmo eggrafon toy gematou bucket afou prokeitai na tis xanaperasoume oles 
  memset(data+2*sizeof(int),0,18*sizeof(SecondaryRecord));
  
  //pairnoume to evretirio 
  BF_GetBlock(k,1,block2);
  data2=BF_Block_GetData(block2);
  // prepei na isomoirasoume ta velakia 
  int w=(int)pow(2,global_depth);
 // int *newArray=malloc(w*sizeof(int));
  int l=global_depth-local_depth; // xeroume oti 2^l theseis tou pinaka edeixan ston gemato kado 
  l=(int)pow(2,l);
  count1=0;
  int ptr;
  int number;
  BF_GetBlockCounter(k,&number);
  while(count1<w){
    if(array[count1]==*(array+hashValue)){
      ptr=count1;
      break;
    }
    count1++;
  }
  count1=0;
  while(count1<l/2){
    array[ptr]=number-1;
    ptr++;
    count1++;
  }
  memcpy(data2,array,w*sizeof(int));
  for(int i=0; i<18; i++){
    SHT_SecondaryInsertEntry(indexDesc,records[i]);
  }
  
  SHT_SecondaryInsertEntry(indexDesc,record);
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

extern int flag;

HT_ErrorCode SHT_SecondaryUpdateEntry (int indexDesc, UpdateRecordArray *updateArray ) {
  if(flag==1){
  for(int i=0; i<8; i++){
    //  printf("surname=%s    city=%s      old=%d      new=%d\n",updateArray[i].surname,updateArray[i].city,updateArray[i].oldTupleId,updateArray[i].newTupleId);
    }

  int k=SHashInfo[indexDesc].filedesc;
  //diavazoume to global_depth
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(k,0,block);  
  int global_depth;
  char *data=BF_Block_GetData(block);
  memcpy(&global_depth,data,sizeof(int));
 // printf("global_depth=%d\n",global_depth);
  //prepei na doume an to pedio kleidi eina to city h to surname
  int key_size;    //an to key_size einai 5 exoume city allios surname
  memcpy(&key_size,data+sizeof(int),sizeof(int));
  
  //pairnoume to evretirio
  BF_GetBlock(k,1,block);
  data=BF_Block_GetData(block);
  int *array;
  array=data;
  int w=(int)pow(2,global_depth);
  for(int i=0; i<w; i++){
 //   printf("%d\n",array[i]);
  }
  SecondaryRecord record,record2;
  int hashValue;
  BF_Block *block2;
  BF_Block_Init(&block2);
  char *data2;
  int noRecords;
  for(int i=0; i<8; i++){
    record.tupleId=updateArray[i].newTupleId;
    if(key_size==5){
    //  printf("To pedio kleidi einai to city\n");
      strcpy(record.index_key,updateArray[i].city);
    }
    else{
      //printf("To pedio kleidi einai to surname\n");
      strcpy(record.index_key,updateArray[i].surname);
    }
    hashValue=hash_Function(record,global_depth);
  //  printf("hashValue=%d\n",hashValue);
    BF_GetBlock(k,*(array+hashValue),block2);
    data2=BF_Block_GetData(block2);
    memcpy(&noRecords,data2+sizeof(int),sizeof(int));
  //  printf("noRecords=%d\n",noRecords);
    for(int j=0; j<noRecords; j++){
      memcpy(&record2,data2+2*sizeof(int)+j*(sizeof(SecondaryRecord)),sizeof(SecondaryRecord));
   //   printf("city=%s       id=%d      old=%d       arrayCity=%s\n",record2.index_key,record2.tupleId,updateArray[i].oldTupleId,updateArray[i].city);
      if(record2.tupleId==updateArray[i].oldTupleId){
        record2.tupleId=updateArray[i].newTupleId;
        memcpy(data2+2*sizeof(int)+j*sizeof(SecondaryRecord),&record2,sizeof(SecondaryRecord));
       // printf("Updated\n");
        BF_Block_SetDirty(block2);
        break;

      }
      BF_UnpinBlock(block2);

    }
    
  }
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&block2);
  flag=0;
  }
  
return HT_OK;
}

HT_ErrorCode SHT_PrintAllEntries(int sindexDesc, char *indexkey ) {
  //insert code here
 
  sindexDesc = SHashInfo[sindexDesc].filedesc;
  int noBlocks, noRecords = 0;
  BF_Block *block;
  BF_Block_Init(&block);
  BF_GetBlock(sindexDesc,0,block);
  SecondaryRecord* record;
  char* data=BF_Block_GetData(block);
  int global;
  memcpy(&global,data,sizeof(int));
  //printf("global_depth=%d\n",global);
  //na tiposo to evretirio gia epalithefsi
  BF_GetBlock(sindexDesc,1,block);
  int* a;
  data=BF_Block_GetData(block);
  a=data;
  //printf("%d\n",(int)pow(2,global));
  for(int i=0; i<(int)pow(2,global); i++){
  //  printf("%d\n",*(a+i));
  }
  int local;
  BF_GetBlockCounter(sindexDesc, &noBlocks);
  //printf("noBlocks=%d\n",noBlocks);
  record = malloc(sizeof(SecondaryRecord));
  if(indexkey == NULL){
    
    for(int i = 2; i < noBlocks; i++){
      if(BF_GetBlock(sindexDesc, i, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      if((data = BF_Block_GetData(block)) == NULL){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      
      memcpy(&local,data,sizeof(int));
      memcpy(&noRecords, data+sizeof(int), sizeof(int));
      //printf("local_depth= %d noRecords=%d\n",local,noRecords);
      for(int j = 0; j < noRecords; j++){
        memcpy(record, data + 2*sizeof(int) + j*sizeof(SecondaryRecord), sizeof(SecondaryRecord));
        printf("hash=%d city=%s typleID=%d\n",hash_Function(*record,local),record->index_key,record->tupleId);
      }
      BF_UnpinBlock(block);
    }
  }
  else{
      printf("Secondary Records with index_key=%s\n",indexkey);
  /*  for(int i = 2; i < noBlocks; ++i){
      if(BF_GetBlock(sindexDesc, i, block) != BF_OK){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      if((data = BF_Block_GetData(block)) == NULL){
        BF_PrintError(BF_ERROR);
        return HT_ERROR;
      }
      memcpy(&noRecords, data+sizeof(int), sizeof(int));
      for(int j = 0; j < noRecords; j++){
        if(strcmp(record->index_key, indexkey) == 0){
          printf("here\n");
          memcpy(record, data + 2*sizeof(int) + j*sizeof(SecondaryRecord), sizeof(SecondaryRecord));
        }
      }
      BF_UnpinBlock(block);
    }
    free(record);
    BF_Block_Destroy(&block);
    */
    //nomizo einai kalytero an to kanoume etsi , de xreiazetai na psaxnoume ola ta block 
    SecondaryRecord record;
    record.tupleId=0;
    strcpy(record.index_key,indexkey);
    int hashValue=hash_Function(record,global);
    BF_GetBlock(sindexDesc,*(a+hashValue),block);
    data=BF_Block_GetData(block);
    int noRecords;
    memcpy(&noRecords,data+sizeof(int),sizeof(int));
    for(int i=0; i<noRecords; i++){
      memcpy(&record,data+2*sizeof(int)+i*sizeof(SecondaryRecord),sizeof(SecondaryRecord));
      //printf("%s\n",record.index_key);
      if(strcmp(record.index_key,indexkey)==0){
        printf("indexkey=%s        tupleId=%d\n",record.index_key,record.tupleId);
      }
    }
    return HT_OK;
  }
  printf("Succesfully printed all secondary index entries\n");
  return HT_OK;
}

HT_ErrorCode SHT_HashStatistics(char *filename ) {
  
  int indexDesc;
  int flag=0;
  for(int i=0; i<MAX_OPEN_FILES; i++){
    if(SHashInfo[i].filedesc!=-1){
       if(SHashInfo[i].filename==filename){     
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
  int fileDesk=SHashInfo[indexDesc].filedesc;
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
      memcpy(&noRecords,data+sizeof(int),sizeof(int));
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
  printf("In the secondary index: number of blocks=%d  max=%d and min=%d and average=%d\n",noBlocks,max,min,sum);
  return HT_OK;
}

int hash_Function(SecondaryRecord record,int depth){

  int count1=0;
  int sum=0;
  while (record.index_key[count1]){
    sum=sum+ (int)record.index_key[count1++];
    
  }
  
 
  int binary[32];
  for(int i=0; i<32; i++){
    binary[i]=0;
  }
  int temp=sum;
  int j=0;
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

HT_ErrorCode SHT_InnerJoin(int sindexDesc1, int sindexDesc2, char *index_key){
  
  sindexDesc1 = SHashInfo[sindexDesc1].filedesc;
  int noBlocks1, noRecords1 = 0;
  BF_Block *block1;
  BF_Block_Init(&block1);
  BF_GetBlock(sindexDesc1,0,block1);
  SecondaryRecord* record1;
  char* data1=BF_Block_GetData(block1);
  int global1;
  memcpy(&global1,data1,sizeof(int));
  //printf("global_depth=%d\n",global1);
  //na tiposo to evretirio gia epalithefsi
  BF_GetBlock(sindexDesc1,1,block1);
  int* a1;
  data1=BF_Block_GetData(block1);
  a1=data1;
  //printf("%d\n",(int)pow(2,global1));
  for(int i=0; i<(int)pow(2,global1); i++){
  //  printf("%d\n",*(a1+i));
  }
  int local1;
  BF_GetBlockCounter(sindexDesc1, &noBlocks1);
  //printf("noBlocks=%d\n",noBlocks1);
  record1 = malloc(sizeof(SecondaryRecord));

  sindexDesc2 = SHashInfo[sindexDesc2].filedesc;
  int noBlocks2, noRecords2 = 0;
  BF_Block *block2;
  BF_Block_Init(&block2);
  BF_GetBlock(sindexDesc2,0,block2);
  SecondaryRecord* record2;
  char* data2=BF_Block_GetData(block2);
  int global2;
  memcpy(&global2,data2,sizeof(int));
  //printf("global_depth=%d\n",global2);
  //na tiposo to evretirio gia epalithefsi
  BF_GetBlock(sindexDesc2,1,block2);
  int* a2;
  data2=BF_Block_GetData(block2);
  a2=data2;
  //printf("%d\n",(int)pow(2,global2));
  for(int i=0; i<(int)pow(2,global2); i++){
  //  printf("%d\n",*(a2+i));
  }
  int local2;
  BF_GetBlockCounter(sindexDesc2, &noBlocks2);
  //printf("noBlocks2=%d\n",noBlocks2);
  record2 = malloc(sizeof(SecondaryRecord));
// 


  // κατι σαν το printAllEntries όμως μόνο για κάθε ζεύξη που χει ταίρι
  if(index_key == NULL){  
    printf("Return all joins\n");
    
    
  }
  else{     // κατι σαν το printAllEntries ομως σε συγκεκριμένο μπλοκ που υπαρχει ταίρι για index_key
    printf("index_key is: %s\n", index_key);
    SecondaryRecord record1;
    record1.tupleId=0;
    strcpy(record1.index_key,index_key);
    printf("record %s\n", record1.index_key);
    int hashValue1=hash_Function(record1,global1);
    BF_GetBlock(sindexDesc1,*(a1+hashValue1),block1);
    data1=BF_Block_GetData(block1);
    int noRecords1;
    memcpy(&noRecords1,data1+sizeof(int),sizeof(int));

    SecondaryRecord record2;
    record2.tupleId=0;
    strcpy(record2.index_key,index_key);
    int hashValue2=hash_Function(record2,global2);
    BF_GetBlock(sindexDesc2,*(a2+hashValue2),block2);
    data2=BF_Block_GetData(block2);
    int noRecords2;
    memcpy(&noRecords2,data2+sizeof(int),sizeof(int));

    printf("global1=%d, global2=%d\n", global1, global2);
    printf("h1=%d, h2=%d\n", hashValue1, hashValue2);
    printf("1=%d, 2=%d\n", noRecords1, noRecords2);
    for(int i=0; i<noRecords1; i++){
      memcpy(&record1,data1+2*sizeof(int)+i*sizeof(SecondaryRecord),sizeof(SecondaryRecord));
      for(int j=0; j<noRecords2; j++){
        printf("INSIDE\n");
        memcpy(&record2,data1+2*sizeof(int)+j*sizeof(SecondaryRecord),sizeof(SecondaryRecord));
        if(strcmp(record1.index_key,index_key)==0 && strcmp(record2.index_key,index_key)==0){
          // print την ζευξη των δυο 
          printf("index_key=%s        tupleId=%d\n",record1.index_key,record1.tupleId);
        }
      }
    }    
  }


  return HT_OK;
}