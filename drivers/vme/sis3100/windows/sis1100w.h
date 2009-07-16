/*

  SIS1100 Windows Driver Wrapper for PLX PCI API V6.00

  */

// Der folgende ifdef-Block zeigt die Standardlösung zur Erstellung von Makros, die das Exportieren 
// aus einer DLL vereinfachen. Alle Dateien in dieser DLL wurden mit dem in der Befehlszeile definierten
// Symbol SIS1100DRV_EXPORTS kompiliert. Dieses Symbol sollte für kein Projekt definiert werden, das
// diese DLL verwendet. Auf diese Weise betrachtet jedes andere Projekt, dessen Quellcodedateien diese Datei 
// einbeziehen, SIS1100DRV_API-Funktionen als aus einer DLL importiert, während diese DLL mit diesem 
// Makro definierte Symbole als exportiert betrachtet.
#ifdef SIS1100DRV_EXPORTS
#define SIS1100DRV_API __declspec(dllexport)
#else
#define SIS1100DRV_API __declspec(dllimport)
#endif

#include "PlxApi.h"



typedef struct _VIRTUAL_ADDRESSES
{
    U32 Va0;
    U32 Va1;
    U32 Va2;
    U32 Va3;
    U32 Va4;
    U32 Va5;
    U32 VaRom;
} VIRTUAL_ADDRESSES, *PVIRTUAL_ADDRESSES;


struct SIS1100_Device_Struct{
	VIRTUAL_ADDRESSES   plx_Va;
	PLX_DEVICE_OBJECT devObj;
//	unsigned int   *plx_Va2;
};



/* init stuff */
SIS1100DRV_API int sis1100w_Find_No_Of_sis1100(U32 *);

SIS1100DRV_API int sis1100w_Get_PciSlot_Information(U32, U32 *, U32 *);
SIS1100DRV_API int sis1100w_Get_PciSlot_Information_Advance(U32, U32 *, U32 *, U8 *);





SIS1100DRV_API int sis1100w_Get_Handle_And_Open(U32 , struct SIS1100_Device_Struct *);
SIS1100DRV_API int sis1100w_Close(struct SIS1100_Device_Struct *);

SIS1100DRV_API int sis1100w_Init(struct SIS1100_Device_Struct *, int);
SIS1100DRV_API int sis1100w_Init_sis3100(struct SIS1100_Device_Struct *, int);
SIS1100DRV_API int sis1100w_Get_IdentVersion(struct SIS1100_Device_Struct *, U32 *);

/* register access 1100 */
SIS1100DRV_API int sis1100w_sis1100_Control_Read(struct SIS1100_Device_Struct *, U32, U32 *);
SIS1100DRV_API int sis1100w_sis1100_Control_Write(struct SIS1100_Device_Struct *, U32, U32);

/* register access 310x */
SIS1100DRV_API int sis1100w_sis3100_Control_Read(struct SIS1100_Device_Struct *, U32, U32 *);
SIS1100DRV_API int sis1100w_sis3100_Control_Write(struct SIS1100_Device_Struct *, U32, U32);

/* vme stuff */
SIS1100DRV_API int sis1100w_VmeSysreset(struct SIS1100_Device_Struct *);
SIS1100DRV_API int sis1100w_Vme_Single_Read(struct SIS1100_Device_Struct *, U32, U32, U32, U32 *);
SIS1100DRV_API int sis1100w_Vme_Single_Write(struct SIS1100_Device_Struct *, U32, U32, U32, U32);
SIS1100DRV_API int sis1100w_Vme_Dma_Read(struct SIS1100_Device_Struct *, U32, U32, U32, U32, U32 *, U32, U32 *);
SIS1100DRV_API int sis1100w_Vme_Dma_Write(struct SIS1100_Device_Struct *, U32, U32, U32, U32, U32 *, U32, U32 *);

SIS1100DRV_API int sis1100w_SdramSharc_Single_Read(struct SIS1100_Device_Struct *, U32, U32 *);
SIS1100DRV_API int sis1100w_SdramSharc_Single_Write(struct SIS1100_Device_Struct *, U32, U32);

SIS1100DRV_API int sis1100w_SdramSharc_Dma_Read(struct SIS1100_Device_Struct *, U32, U32 *, U32, U32 *);
SIS1100DRV_API int sis1100w_SdramSharc_Dma_Write(struct SIS1100_Device_Struct *, U32, U32 *, U32, U32 *);


SIS1100DRV_API int sis1100w_version(U32 * )  ;












