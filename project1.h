typedef struct {
  unsigned cmd;
  char *arg;
} command;

#define nullTerminatedCmd  (1)
#define givenLengthCmd  (2)
#define badIntCmd  (3)
#define goodIntCmd  (4)
#define byteAtATimeCmd  (5)
#define kByteAtATimeCmd  (6)
#define noMoreCommands (0)

#define BUFSIZE (1000)

/* This is the list of commands to be run by the client to demonstrate your program */
static command commands[] = {
  {nullTerminatedCmd, "Sent as a null-terminated string"},
  {givenLengthCmd, "Sent as an unterminated string"},
  {badIntCmd, "20160919"},
  {goodIntCmd, "20160919"},
  {byteAtATimeCmd, "500000"},
  {kByteAtATimeCmd, "500000"},
  {noMoreCommands, ""}
};

/* 
 * These command names are to be prefixed to responses by the server; the array is
 * indexed by the #define'd constants above
 */
static char *commandNames[] = {
  "No More Commands",
  "Null Terminated",
  "Given Length",
  "Bad Int",
  "Good Int",
  "Byte At A Time",
    "KByte At A Time"
};

/* The maximum argument string length is 128 bytes */
/*
 * Client behavior:
 *   For noMoreCommands: 
 *       don't send anything; exit the command-reading loop and 
 *       close the socket.
 *   For nullTerminatedCmd:
 *       Send the string given as command.arg along with a null character as terminator
 *       (i.e. just like the string is conventionally represented in C). 
 *   For givenLengthCmd:
 *       Send the string's length as a 16 bit number in network byte order followed by the
 *       characters of the string; do not include a null character.
 *   For badIntCmd:
 *       Convert command.arg to an int and send the 4 bytes without applying htonl() to the value.
 *       This is the incorrect way to go! Note that the number you get back from the server won't
 *       be what was sent.
 *   For goodIntCmd:
 *       Convert command.arg to an int and send the 4 bytes resulting from applying htonl() to it.
 *   For byteAtATimeCmd and kByteAtATimeCmd:
 *       Convert command.arg to an int; send the int (after apply htonl) and then send
 *       that many bytes of alternating 1000-byte blocks of 0 bytes and 1 bytes.
 *       ByteAtATime - use 1-byte sends and receives
 *       KByteAtATime - use 1000-byte sends and receives (except for the last)
 *   After sending the message associated with a command, recv the response
 *   that the server produces for that command and print it on stdout followed by a \n
 */

/*
 * Server behavior:
 *   The server never receives noMoreCommands.
 *
 *   For nullTerminatedCmd, givenLengthCmd, badIntCmd, goodIntCmd:
 *   reply with a 16 bit string length followed by a string containing 
 *   the name of the command (i.e. commandNames[cmdByte]), 
 *   a colon, a space, and the received value. Example:
 *      16bit: htons(11)
 *      11 bytes: GoodInt: 37
 *   Note that the terminator for the string sent by nullTerminatedCmd
 *   is *not* considered part of the string and should not be echoed.
 *
 *   The server behavior for BadInt and GoodInt is identical -- it applies
 *   ntohl to the received bytes.
 *   
 *   For byteAtATimeCmd and kByteAtATimeCmd reply with the name of the command and the
 *   total number of recv() operations that the server performed in carrying it out,
 *   formatted as an ASCII string and counting the first recv of the data bytes as 1. 
 *   Use the same format as before (16 bit network byte order integer, followed by the
 *   bytes of the string with no terminator character).
 *
 *   In addition to its other behavior, the server is to write all the bytes that
 *   it receives into a file -- it doesn't matter where as long as you can find it later.
 *   Make sure to write all and only the bytes the server receives into the file.
 *
 *   The server should close the client connection when it recv's 0 bytes, which indicates that the 
 *   client has closed its end of the connection. Print on the console (printf) the total number 
 *   of bytes received on the current connection. Then continue by calling accept() for a new
 *   connection.
 */
