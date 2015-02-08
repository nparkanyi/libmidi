#include <stdio.h>
#include "midi.h"

int main(){
	FILE * file;
	guint32 val = 0;

	file = fopen("vlv", "r");
  
	printf("%d\n", G_BYTE_ORDER);
	VLV_read(file, &val);
	printf("\n%X\n", val);
	
	return 0;
}
