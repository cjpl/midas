// CC-USB ccusb.h

#ifdef __cplusplus
extern "C" {
#endif

int   ccusb_init();
void* ccusb_getCrateHandle(int crate);
int   ccusb_flush(void*handle);
int   ccusb_readReg(void*handle,int ireg);
int   ccusb_writeReg(void*handle,int ireg,int value);
int   ccusb_reset(void*handle);
int   ccusb_status(void*handle);
int   ccusb_naf(void*handle,int n,int a,int f,int d24,int data);

#ifdef __cplusplus
}
#endif
// end
