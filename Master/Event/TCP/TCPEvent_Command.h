#pragma once

#include "Network/NetClient.h"

void TCPEvent_Command(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);