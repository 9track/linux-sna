#include <stdio.h>
#include <stdlib.h>
#include <linux/cpic.h>

int main(void)
{
	unsigned char	conversation_ID[CM_CID_SIZE];
	unsigned char	data_buffer[100+1];
	CM_INT32	requested_length = (CM_INT32)sizeof(data_buffer)-1;
	CM_INT32	received_length = 0;
	CM_RETURN_CODE	cpic_return_code;
	CM_DATA_RECEIVED_TYPE		data_received;
	CM_STATUS_RECEIVED		status_received;
	CM_REQUEST_TO_SEND_RECEIVED	rts_received;

	cmaccp(conversation_ID, &cpic_return_code);
	printf("Got called and did cmaccp\n");

//	cmrcv(conversation_ID, data_buffer, &requested_length,
//		&data_received, &received_length, &status_received,
//		&rts_received, &cpic_return_code);
//	data_buffer[received_length] = '\0';
//	printf("%s\n", data_buffer);
//	printf("Press a key to end the program...");
//	getchar();

	return (0);
}
