#include<stdio.h>
#include<stdlib.h>

int main(void)
{
	int a,b;
	char *data;
	int respondNum;

	printf("%s%c%c\n",
           "Content-Type:text/html;charset=iso-8859-1",13,10);
    printf("<TITLE>Multiplication results</TITLE>\n");
    printf("<H3>Multiplication results</H3>\n");
	printf("<P>This is a respond:\n</P>");
	data=getenv("QUERY_STRING");
	if(data==NULL)
	{
		printf("<P>Error,data is null</P>");
	}
	else
	{
		respondNum=sscanf(data,"a=%d&b=%d",&a,&b);
		if(respondNum>0){
			printf("<P>Ans:%d \n</P>",a+b);
		}else{
			printf("<P>invalid Data</P>");
		}
	}

	return 0;
}