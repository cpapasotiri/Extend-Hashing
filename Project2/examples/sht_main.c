#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
//#include "hash_file.h"
#include "sht_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPT 2 // you can change it if you want
#define FILE_NAME "data.db"
#define FILE_NAME1 "data1.db"

const char* names[] = {
  "Yannis",
  "Christofos",
  "Sofia",
  "Marianna",
  "Vagelis",
  "Maria",
  "Iosif",
  "Dionisis",
  "Konstantina",
  "Theofilos",
  "Giorgos",
  "Dimitris"
};

const char* surnames[] = {
  "Ioannidis",
  "Svingos",
  "Karvounari",
  "Rezkalla",
  "Nikolopoulos",
  "Berreta",
  "Koronis",
  "Gaitanis",
  "Oikonomou",
  "Mailis",
  "Michas",
  "Halatsis"
};

const char* cities[] = {
  "Athens",
  "San Francisco",
  "Keratsini",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Madrid",
  "Los Angeles"
};

#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

int main() {

  BF_Init(LRU);
  
  CALL_OR_DIE(HT_Init());
  CALL_OR_DIE(SHT_Init());

  UpdateRecordArray *array=malloc(8*sizeof(UpdateRecordArray));
  for(int i=0; i<8; i++){                    //xreiazete afti i arxikopoiisi
    strcpy(array[i].city,"");
    array[i].newTupleId=0;
    array[i].oldTupleId=0;
    strcpy(array[i].surname,"");
  }
  //array=malloc(8*sizeof(int));
  int indexDesc, sindexDesc;
  //1)dimioyrgoume to protevon
  CALL_OR_DIE(HT_CreateIndex(FILE_NAME, GLOBAL_DEPT));
  CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
  //2)dimiourgoume to defterevon 
  CALL_OR_DIE(SHT_CreateSecondaryIndex(FILE_NAME1, "city", 20, GLOBAL_DEPT, FILE_NAME));
  CALL_OR_DIE(SHT_OpenSecondaryIndex(FILE_NAME1, &sindexDesc)); 
  
  Record record;
  srand(12569874);
  int r;
  int a=0;
  
  SecondaryRecord record2;
  //3)kanoume eisagogi eggrafes sto protevon kai sto defterevon me indexKey to city
  for (int id = 0; id < 28; id++) {
    // create a record
    record.id = id;
    r = (rand()%10);
    memcpy(record.name, names[r], (strlen(names[r]) + 1)*sizeof(char));
    r = (rand()%9);
    memcpy(record.surname, surnames[r], (strlen(surnames[r]) + 1)*sizeof(char));
    r = (rand()%11);
    //printf("r=%d\n",r);
    memcpy(record.city, cities[r], (strlen(cities[r]) + 1)*sizeof(char));

    CALL_OR_DIE(HT_InsertEntry(indexDesc, record,&a, array));
    //printf("id=%d\n",id);
   
    strcpy(record2.index_key,record.city);
    //printf("eeeeeeeeeeeeeeeeeee %s\n",array[0].city);
    record2.tupleId=a;
    SHT_SecondaryUpdateEntry(sindexDesc,array);
    CALL_OR_DIE(SHT_SecondaryInsertEntry(sindexDesc, record2));
  //  CALL_OR_DIE(HT_PrintAllEntries(indexDesc, id));
   // CALL_OR_DIE(SHT_PrintAllEntries(sindexDesc, NULL));
  }
    //SHT_SecondaryUpdateEntry(sindexDesc,array);
    //CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
    //CALL_OR_DIE(SHT_PrintAllEntries(sindexDesc, NULL));
  //4)
    int k=5;
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc,&k));
    CALL_OR_DIE(SHT_PrintAllEntries(sindexDesc,"Athens"));
    CALL_OR_DIE(HashStatistics(FILE_NAME));
    CALL_OR_DIE(SHT_HashStatistics(FILE_NAME1));

  //6)pairname alles 25 eggrafes
  int counter=0;
  while(counter<15){
    record.id=rand()%400;
   
    r = (rand()%10);
    memcpy(record.name, names[r], (strlen(names[r]) + 1)*sizeof(char));
    r = (rand()%9);
    memcpy(record.surname, surnames[r], (strlen(surnames[r]) + 1)*sizeof(char));
    r = (rand()%11);
    //printf("r=%d\n",r);
    memcpy(record.city, cities[r], (strlen(cities[r]) + 1)*sizeof(char));

    CALL_OR_DIE(HT_InsertEntry(indexDesc, record,&a, array));
    record2.tupleId=a;
    SHT_SecondaryUpdateEntry(sindexDesc,array);
    CALL_OR_DIE(SHT_SecondaryInsertEntry(sindexDesc, record2));
    
    counter++;
  }
  k=145;
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc,&k));
    CALL_OR_DIE(SHT_PrintAllEntries(sindexDesc,"Munich"));
    CALL_OR_DIE(HashStatistics(FILE_NAME));
    CALL_OR_DIE(SHT_HashStatistics(FILE_NAME1));
    CALL_OR_DIE(SHT_CloseSecondaryIndex(sindexDesc));
    CALL_OR_DIE(HT_CloseFile(indexDesc));
  
  
    free(array);   
}
