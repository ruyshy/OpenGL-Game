#pragma once

#ifndef SANDFORGE_PACKET_TYPES_H_
#define SANDFORGE_PACKET_TYPES_H_

#include "../pch.h"

enum class SandforgePacketType : uint16_t
{
    C2S_JoinRequest = 1,
    S2C_JoinAccept = 2,
    S2C_MatchStart = 3,
    C2S_Ready = 4,
    C2S_Ping = 5,
    S2C_Pong = 6,
    S2C_WorldSnapshot = 7,
    C2S_ProduceCommand = 8,
    C2S_AssignWorkerCommand = 9,
    C2S_MoveUnitCommand = 10,
    C2S_BuildCommand = 11,
    C2S_CancelProduction = 12,
    S2C_LobbySettings = 13
};

struct SandforgePacketHeader
{
    uint16_t size = 0;
    uint16_t type = 0;
    uint32_t sequence = 0;
};

#endif // !SANDFORGE_PACKET_TYPES_H_
