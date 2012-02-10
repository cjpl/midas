#include "mex.h"
#include "msc.h"
#include "data.h"
#include <string.h>

#define STRINGNUM 7

void mexFunction(int nlhs, mxArray *plhs[ ],int nrhs, const mxArray *prhs[ ]) 
{
	char* StringPtr[STRINGNUM];

	int m,n;
	int count;
	bool validInput = true;
	int buflen;
	char *input_buf;
	int status;
	bool writeFlag = false;

	double* nodeAddrDouble;
	double* rwAddrDouble;
	double* writeValDouble;

	int nodeAddrInt;
	int rwAddrInt;
	long signed int writeValInt;
	
	char tempBuffer[256];
	char* tempBufPtr = tempBuffer;

	char myString1[] = "asfasdfasf";
	char myString2[] = "-d";
	char ipString[256] = "172.23.1.153";
	char myString4[256] = "-a";
	char nodeAddr[256] = "0";
	char myString6[256] = "-c";
	char rwCmd[256] = "r";
	char rwAddr[256] = "10";
	char wVal[256] = "3";
	char space[] = " ";
	char rwCmd_rwAddr_wVal[512];
	
	char* ipStringPtr = ipString;
	char* rwCmdPtr = rwCmd;
	char* nodeAddrPtr = nodeAddr;
	char* rwAddrPtr = rwAddr;
	char* wValPtr = wVal;
	char* rwCmd_rwAddr_wVal_Ptr = rwCmd_rwAddr_wVal;
	char* spacePtr = space;
	
	if(nrhs == 4 || nrhs == 5)
	{
		for(count = 0; count < nrhs; count++)
		{
			/* Find the dimensions of each input data */
			m = mxGetM(prhs[count]);
			n = mxGetN(prhs[count]);

			//number of column should equal to 1
			if(m != 1)
			{
				validInput = false;
			}
			else
			{
				//char data input parsing
				if(count == 0 || count == 2)
				{
					//check char exists
					if(!mxIsChar(prhs[count]))
					{
						mexErrMsgTxt("Char array expected.");
						validInput = false;
					}
					else
					{
						//get length of the char array
						buflen = mxGetN(prhs[count]);
						if(buflen > 0)
						{
							//printf("string length\n");
							itoa(buflen,tempBufPtr,10);
							//printf(tempBufPtr);
							//printf("\n");
							input_buf = mxCalloc(buflen + 1, sizeof(char));
							status = mxGetString(prhs[count], input_buf, buflen + 1);

							if(count == 0)
							{
								ipStringPtr = input_buf;
							}
							if(count == 2)
							{
								rwCmdPtr = input_buf;	
								if(rwCmdPtr == "w")
								{
									writeFlag = true;
								}
							}
							input_buf = NULL;
						}
						else
						{
							printf("empty input\n");
							validInput = false;
						}
					}
				}
				else
				{
					//check numerical value exists
					if(!mxIsNumeric(prhs[count]))
					{
						mexErrMsgTxt("Numerical type expected.");
						validInput = false;
					}
					else
					{
						if(count == 1)
						{
							nodeAddrDouble = mxGetPr(prhs[count]);
							nodeAddrInt = (int)(*nodeAddrDouble);
							
							itoa(nodeAddrInt,nodeAddrPtr,10);
							//printf("node address: ");
							//printf(nodeAddrPtr);
							//printf("\n");
						}
						else if(count == 3)
						{
							rwAddrDouble = mxGetPr(prhs[count]);
							rwAddrInt = (int)(*rwAddrDouble);
							
							itoa(rwAddrInt,rwAddrPtr,10);
							//printf("r/w address: ");
							//printf(rwAddrPtr);
							//printf("\n");
						}
						else if(count == 4)
						{
							writeValDouble = mxGetPr(prhs[count]);
							writeValInt = *writeValDouble;

							itoa(writeValInt,wValPtr,10);

							//printf("write value: ");
							//printf(wValPtr);
							//printf("\n");
						}
					}
				}
			}
		}
	}
	else
	{
		validInput = false;
	}
	
	if(validInput == false)
	{
		printf("input: [ip:string, node_addr:int, r/w:string, read_addr/write_addr:int, (write_val):int]\n");
	}
	else
	{
	
		strcpy(rwCmd_rwAddr_wVal_Ptr, rwCmdPtr); 
		strcat(rwCmd_rwAddr_wVal_Ptr, spacePtr); 
		strcat(rwCmd_rwAddr_wVal_Ptr, rwAddrPtr);	
		
		if(nrhs == 5)
		{
			strcat(rwCmd_rwAddr_wVal_Ptr, spacePtr);
			strcat(rwCmd_rwAddr_wVal_Ptr, wValPtr); 
		}
		
		StringPtr[0] = myString1;
		StringPtr[1] = myString2;
		StringPtr[2] = ipStringPtr;
		StringPtr[3] = myString4;
		StringPtr[4] = nodeAddrPtr;
		StringPtr[5] = myString6;
		StringPtr[6] = rwCmd_rwAddr_wVal_Ptr;

		//printf("concatenate test: ");
		//printf(rwCmd_rwAddr_wVal_Ptr);
		//printf("\n");

		//for(count = 0; count < STRINGNUM; count++)
		//{
		//	printf("--------------\n");
		//	printf( StringPtr[count]);
		//	printf("\n");
		//}

		mainmsc(STRINGNUM,StringPtr,plhs, writeFlag);
	}

}
