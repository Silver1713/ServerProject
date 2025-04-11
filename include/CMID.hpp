#ifndef CMID_HPP
#define CMID_HPP

/*!****************************************************************************
 * \brief
 * Enum for command ID
 ******************************************************************************/
enum struct CMID : unsigned char
{
	UNKNOWN = 0x0,
	REQ_QUIT = 0x1,
	REQ_DOWNLOAD = 0x2,
	RSP_DOWNLOAD = 0x3,
	REQ_LISTFILES = 0x4,
	RSP_LISTFILES = 0x5,
	CMD_TEST = 0x20,
	DOWNLOADERROR = 0x30
};


enum struct UDPCMID : unsigned char {
	PAYLOAD_UNKNOWN = 0x0,
	PAYLOAD_REC = 0x1,
	PAYLOAD_SEND = 0x2,
	PAYLOAD_ACK = 0x3,
};
#endif	