#include <stdio.h>
#include "midi.h"

int main(){
	FILE * file;
	guint32 val = 0;
  int bytes_read = 0;

	file = fopen("vlv", "r");
  
	printf("%d\n", G_BYTE_ORDER);
	VLV_read(file, &val, &bytes_read);
	printf("\n%X\n", val);
  printf("bytes read: %d\n", bytes_read);
	
	return 0;
}
