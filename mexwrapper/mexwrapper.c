#include "mex.h"
#include "msc.h"
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
	bool scanFlag = false;

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
	
	char scanCmd[256] = " ";

	char* ipStringPtr = ipString;
	char* rwCmdPtr = rwCmd;
	char* nodeAddrPtr = nodeAddr;
	char* rwAddrPtr = rwAddr;
	char* wValPtr = wVal;
	char* rwCmd_rwAddr_wVal_Ptr = rwCmd_rwAddr_wVal;
	char* spacePtr = space;

	char* scanCmdPtr = scanCmd;
	
	if(nrhs == 3 || nrhs == 4 || nrhs == 5 || nrhs == 2)
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
							input_buf = (char *) mxCalloc(buflen + 1, sizeof(char));
							status = mxGetString(prhs[count], input_buf, buflen + 1);

							if(count == 0)
							{
								ipStringPtr = input_buf;
							}
							if(count == 2)
							{
								rwCmdPtr = input_buf;	
								
								//check for right number of inputs
								if( strcmp(rwCmdPtr,"w") == 0)
								{
									if(nrhs == 5)
									{
										writeFlag = true;
									}
									else
									{
										validInput = false;
									}
								}
								else if(strcmp(rwCmdPtr, "r") == 0)
								{
									if(nrhs == 4)
									{
										writeFlag = false;
									}
									else
									{
										validInput = false;
									}
								}
								else if(strcmp(rwCmdPtr, "i") == 0)
								{
									if(nrhs == 3)
									{
										writeFlag = false;
									}
									else
									{
										validInput = false;
									}
								}
								else
								{
									validInput = false;
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
						//check if command is to scan node
						if(count == 1)
						{
							if(nrhs == 2)
							{
								printf("entered here!\n");

								//get length of the char array
								buflen = mxGetN(prhs[count]);
								if(buflen > 0)
								{
									itoa(buflen,tempBufPtr,10);
									input_buf = (char *) mxCalloc(buflen + 1, sizeof(char));
									status = mxGetString(prhs[count], input_buf, buflen + 1);
																
									scanCmdPtr = input_buf;	

									if(strcmp(scanCmdPtr, "sc") == 0)
									{
										scanFlag = true;
									}
								}	
							}
						}
						else
						{
							mexErrMsgTxt("Numerical type expected.");
							validInput = false;
						}
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
		printf("Function Input/Output:\n");
		printf("read node info  -> input: [ip:string, node_addr:int, 'i']\n");
		printf("read reg value  -> input: [ip:string, node_addr:int, 'r', read_addr:int]\n");
		printf("write reg value -> input: [ip:string, node_addr:int, 'w', write_addr:int, write_val:int], output: [reg_name, reg_val] \n");
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
		
		if(scanFlag == false)
		{
			StringPtr[0] = myString1;
			StringPtr[1] = myString2;
			StringPtr[2] = ipStringPtr;
			StringPtr[3] = myString4;
			StringPtr[4] = nodeAddrPtr;
			StringPtr[5] = myString6;
			StringPtr[6] = rwCmd_rwAddr_wVal_Ptr;
		}
		else
		{
			StringPtr[0] = myString1;
			StringPtr[1] = myString2;
			StringPtr[2] = ipStringPtr;
			StringPtr[3] = "-c";
			StringPtr[4] = "sc";
			StringPtr[5] = " ";
			StringPtr[6] = " ";
		}

		//check if it's usb mode
		if(strcmp(ipStringPtr, "usb") == 0)
		{
			StringPtr[1] = "";
			StringPtr[2] = "";
		}

		//printf("concatenate test: ");
		//printf(rwCmd_rwAddr_wVal_Ptr);
		//printf("\n");

		//for(count = 0; count < STRINGNUM; count++)
		//{
		//	printf("--------------\n");
		//	printf( StringPtr[count]);
		//	printf("\n");
		//}

		mainmsc(STRINGNUM,StringPtr,plhs, writeFlag, scanFlag);
	}

}
