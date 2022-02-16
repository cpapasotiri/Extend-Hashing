#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define GLOBAL_DEPT 2 // you can change it if you want
#define FILE_NAME "data.db"
#define FILE_NAME1 "data1.db"
#define FILE_NAME2 "data2.db"

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
  "Los Angeles",
  "Amsterdam",
  "London",
  "New York",
  "Tokyo",
  "Hong Kong",
  "Munich",
  "Miami"
};

#define CALL_OR_DIE(call)   \
{                           \
  HT_ErrorCode code = call; \
  if (code != HT_OK) {      \
    printf("Error\n");      \
    exit(code);             \
  }                         \
}

int main() {
  BF_Init(MRU);
  
  CALL_OR_DIE(HT_Init());
  
  int indexDesc;
  CALL_OR_DIE(HT_CreateIndex(FILE_NAME, GLOBAL_DEPT));
  CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc)); 
  int idn = indexDesc;

  int indexDesc1;
  CALL_OR_DIE(HT_CreateIndex(FILE_NAME1, GLOBAL_DEPT));
  CALL_OR_DIE(HT_OpenIndex(FILE_NAME1, &indexDesc1)); 
  int idn1 = indexDesc1;

  Record record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  int counter=0;
  srand(time(NULL));
  while(counter<250){ 
    // create a record
     record.id = rand()%RECORDS_NUM;
     r = rand() % 12;
     memcpy(record.name, names[r], strlen(names[r]) + 1);
     r = rand() % 12;
     memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
     r = rand() % 10;
     memcpy(record.city, cities[r], strlen(cities[r]) + 1);
     CALL_OR_DIE(HT_InsertEntry(indexDesc, record));
     counter++;
   }
  counter = 0;
  while(counter<40){ 
    // create a record
     record.id = rand()%RECORDS_NUM;
     r = rand() % 12;
     memcpy(record.name, names[r], strlen(names[r]) + 1);
     r = rand() % 12;
     memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
     r = rand() % 10;
     memcpy(record.city, cities[r], strlen(cities[r]) + 1);
     CALL_OR_DIE(HT_InsertEntry(indexDesc1, record));
     counter++;
   }
  printf("Run PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
  CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, &id));
  CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));
  CALL_OR_DIE(HT_PrintAllEntries(indexDesc1, NULL));
  CALL_OR_DIE(HashStatistics(FILE_NAME));
  CALL_OR_DIE(HashStatistics(FILE_NAME1));
  CALL_OR_DIE(HT_CloseFile(idn));
  CALL_OR_DIE(HT_CloseFile(idn1));
  
  CALL_OR_DIE(HT_CreateIndex(FILE_NAME2, GLOBAL_DEPT));
  for(int i = 0; i < 30; i++){
    printf("We are opening the file for the %d-th time.\n",i);
    int indexDesc;
    CALL_OR_DIE(HT_OpenIndex(FILE_NAME2, &indexDesc)); 
    int idn = indexDesc;
    CALL_OR_DIE(HT_CloseFile(idn));
  }

  BF_Close();
}
