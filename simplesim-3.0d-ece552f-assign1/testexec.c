#include <stdio.h>
int main (int argc, char *argv[ ])
{

/*void swap(int array[][3])
{
        int i = 0; //number of rows
        int j = 0; //number of columns
	int size =  sizeof(array) / sizeof (int);
        
  
        for (i = size-1; i >= 0; i--)
        {
          for (j = 0; j < 3; j++)
          {
            array[i+1][j] = array[i][j];
          }     
        }
}*/

__asm__ (
	   "add $7, $9, $3;"
	   "add $3, $7, $11;"   
 );

/*int c[2][3]={{1,3,0}, {-1,5,9}};
swap(c);
printf("c is %d",c[1][0]);*/
}
