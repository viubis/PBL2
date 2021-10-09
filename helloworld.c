#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <string.h>
#include <mongoc/mongoc.h>


int main(){
	char c = '1';
	int a = c - '0';
	printf("%d", a);	
    return 0;
}

