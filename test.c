

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void list_2_json(char* L){
  int i = 0;

  while(L[i] != '\0'){

    if (L[i] == '{' || L[i] == '}'){
      L[i] = '\"';
    }

    i ++;
  }
}

int main(){

  char* str = malloc(sizeof(char) * 30);
  strcpy(str, "[{a}, {b}, {c}]\0");

  printf("str: %s\n", str);
  list_2_json(str);
  printf("result: %s\n", str);

  return 1;
}
