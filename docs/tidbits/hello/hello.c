#include <stdio.h>
#include <string.h>
#include <linux/cpic.h>

#define SYM_DEST_NAME	(unsigned char *)"HELLOD  "
#define SEND_THIS	(unsigned char *)"Hello, Jay"

int main(void)
{
	unsigned char	conversation_ID[CM_CID_SIZE];
	unsigned char	*data_buffer = SEND_THIS;
	CM_INT32	send_length = (CM_INT32)strlen(SEND_THIS);
	CM_RETURN_CODE	cpic_return_code;
	CM_REQUEST_TO_SEND_RECEIVED rts_received;

	cminit(conversation_ID, SYM_DEST_NAME, &cpic_return_code);
	if(cpic_return_code != CM_OK)
		printf("Bad cminit (%d)\n", cpic_return_code);
	else
		printf("Good cminit\n");

	cmallc(conversation_ID, &cpic_return_code);
	if(cpic_return_code != CM_OK)
                printf("Bad cmallc (%d)\n", cpic_return_code);
        else
                printf("Good cmallc\n");
	sleep(5);

	cmsend(conversation_ID, data_buffer, &send_length,
		&rts_received, &cpic_return_code);
//	cmdeal(conversation_ID, &cpic_return_code);

	return (0);
}
