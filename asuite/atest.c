/* atest.c: simple test program for linux-sna.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <linux/cpic.h>

#define SYM_DEST_NAME	(unsigned char *)"AFTP    "
#define SEND_THIS	(unsigned char *)"Hello, Jay"

int main(void)
{
	unsigned char	conversation_ID[CM_CID_SIZE];
	unsigned char	*data_buffer = SEND_THIS;
	CM_INT32	send_length = (CM_INT32)strlen(SEND_THIS);
	CM_RETURN_CODE	cpic_return_code;
	CM_REQUEST_TO_SEND_RECEIVED rts_received;

	memset(conversation_ID, 0, CM_CID_SIZE);
	printf("conversation_ID=%02X%02X%02X%02X%02X%02X%02X%02X\n",
                conversation_ID[0], conversation_ID[1],
                conversation_ID[2], conversation_ID[3],
                conversation_ID[4], conversation_ID[5],
                conversation_ID[6], conversation_ID[7]);
	
	printf("cminit.. ");
	cminit(conversation_ID, SYM_DEST_NAME, &cpic_return_code);
	if (cpic_return_code != CM_OK)
		printf("bad (%ld).\n", cpic_return_code);
	else
		printf("good!\n");

	printf("conversation_ID=%02X%02X%02X%02X%02X%02X%02X%02X\n",
		conversation_ID[0], conversation_ID[1],
		conversation_ID[2], conversation_ID[3],
		conversation_ID[4], conversation_ID[5],
		conversation_ID[6], conversation_ID[7]);
	
	printf("cmallc.. ");
	cmallc(conversation_ID, &cpic_return_code);
	if (cpic_return_code != CM_OK)
                printf("bad (%ld).\n", cpic_return_code);
        else
                printf("good!\n");

//	cmsend(conversation_ID, data_buffer, &send_length,
//		&rts_received, &cpic_return_code);
//	cmdeal(conversation_ID, &cpic_return_code);
	return 0;
}
