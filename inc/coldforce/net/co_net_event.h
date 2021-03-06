#ifndef CO_NET_EVENT_H_INCLUDED
#define CO_NET_EVENT_H_INCLUDED

#include <coldforce/net/co_net.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net event
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_NET_EVENT_ID_TCP_ACCEPT_READY        0x7101
#define CO_NET_EVENT_ID_TCP_CONNECT_COMPLETE    0x7102
#define CO_NET_EVENT_ID_TCP_SEND_READY          0x7103
#define CO_NET_EVENT_ID_TCP_SEND_COMPLETE       0x7104
#define CO_NET_EVENT_ID_TCP_RECEIVE_READY       0x7105
#define CO_NET_EVENT_ID_TCP_CLOSE               0x7106
#define CO_NET_EVENT_ID_TCP_TRANSFER            0x7107

#define CO_NET_EVENT_ID_UDP_SEND_READY          0x7111
#define CO_NET_EVENT_ID_UDP_SEND_COMPLETE       0x7112
#define CO_NET_EVENT_ID_UDP_RECEIVE_READY       0x7113

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_EVENT_H_INCLUDED
