#define MAP_DEFAULT "37"
#define BLINK (1<<6)

tnXhost *tnX_host_new(tnXstream *This);
void setXCharMap(tnXhost *This, const char *name); 
void sendReadMDT(tnXstream *This, tnXbuff * buff);
void appendBlock2Ebcdic(tnXbuff *buff, unsigned char *str, 
			int len, tnXchar_map *map);
void writeToDisplay(tnXhost *This);
void setBufferAddr(tnXhost *This, int row, int col);
void repeat2Addr(tnXhost *This, int row, int col, unsigned char uc);
void setAttribute(tnXhost *This, unsigned short attr);
int readMDTfields_1(client_t *client, tnXhost *This, int sendRMF);
int readMDTfields_2(client_t *client, tnXhost *This, int sendRMF);
int processFlags(tnXhost *This, unsigned char flags, unsigned char *buf);
int cancelInvite(tnXstream *This);
char *getErrMsg(short ecode);
void sendWriteErrorCode(tnXhost *This, char *msg, unsigned char opcode);
char *processErr(unsigned char *buf);
void clearScreen(tnXhost * This);
int processSRQ(tnXstream *This);
void hiliteString(tnXbuff *buff, char *str, tnXchar_map *map);
void flushTN5250(tnXhost *This);
tnXrec *saveScreen(tnXstream *This);
void restoreScreen(tnXstream *This, tnXbuff *buff);
int SendTestScreen(client_t *client);
void tnX_host_destroy(tnXhost *This);
