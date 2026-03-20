#pragma once

#ifndef SANDFORGE_MULTIPLAYER_SESSION_H_
#define SANDFORGE_MULTIPLAYER_SESSION_H_

#include "SandforgePacketBuffer.h"
#include "World/SandforgeWorld.h"
#include "Core/SandforgeTypes.h"

enum class SandforgeMultiplayerMode
{
    None,
    Host,
    Client
};

enum class SandforgeMultiplayerConnectionState
{
    Idle,
    Hosting,
    Connecting,
    Connected,
    Failed
};

struct SandforgeLobbyState
{
    SandforgeMultiplayerMode mode = SandforgeMultiplayerMode::None;
    SandforgeMultiplayerConnectionState connectionState = SandforgeMultiplayerConnectionState::Idle;
    uint32_t localPlayerId = 0;
    string localPlayerName = "Player";
    string remotePlayerName = "Remote";
    SandforgeStartResourcePreset resourcePreset = SandforgeStartResourcePreset::Standard;
    SandforgeStartWorkerPreset workerPreset = SandforgeStartWorkerPreset::Standard;
    bool remoteConnected = false;
    bool localReady = false;
    bool remoteReady = false;
    bool matchStartReceived = false;
    string statusText = "Open a multiplayer session.";
};

enum class SandforgeNetCommandType
{
    Produce,
    AssignWorker,
    MoveUnit,
    Build,
    CancelProduction
};

struct SandforgeNetCommand
{
    SandforgeNetCommandType type = SandforgeNetCommandType::Produce;
    uint32_t entityId = 0;
    uint16_t nodeIndex = 0;
    SandforgeBuildingType buildingType = SandforgeBuildingType::HQ;
    SandforgeUnitType unitType = SandforgeUnitType::Worker;
    SandforgeVec2 position{};
};

class SandforgeMultiplayerSession
{
public:
    SandforgeMultiplayerSession();
    ~SandforgeMultiplayerSession();

    bool host(uint16_t port);
    bool join(const string& address, uint16_t port);
    void disconnect();
    void update();
    void setLocalPlayerName(const string& playerName);
    void setLobbyMatchSettings(SandforgeStartResourcePreset resourcePreset, SandforgeStartWorkerPreset workerPreset);
    void setLocalReady(bool ready);
    bool tryStartMatch();
    bool consumeMatchStart();
    bool isAuthoritativeHost() const;
    void sendWorldSnapshot(const SandforgeWorldSnapshot& snapshot);
    bool consumeLatestSnapshot(SandforgeWorldSnapshot& outSnapshot);
    void sendProduceCommand(uint32_t buildingId, SandforgeBuildingType buildingType, SandforgeUnitType unitType);
    void sendAssignWorkerCommand(uint32_t workerId, uint16_t nodeIndex);
    void sendMoveUnitCommand(uint32_t unitId, const SandforgeVec2& position);
    void sendBuildCommand(SandforgeBuildingType buildingType, uint16_t nodeIndex, const SandforgeVec2& position);
    void sendCancelProductionCommand(uint32_t buildingId, SandforgeBuildingType buildingType);
    vector<SandforgeNetCommand> consumePendingCommands();
    const SandforgeLobbyState& getLobbyState() const;

private:
    bool ensureWinsock();
    void closeSocket(SOCKET& socketHandle);
    bool makeSocketNonBlocking(SOCKET socketHandle) const;
    bool sendPacket(const vector<uint8_t>& packet);
    void sendJoinRequest();
    void sendJoinAccept(uint32_t playerId);
    void sendReadyPacket(bool ready);
    void sendMatchStartPacket();
    void sendLobbySettingsPacket();
    void sendPong(uint32_t sequence);
    void processAccept();
    void processIncomingPackets();
    void processPacket(const SandforgePacketHeader& header, const uint8_t* payload, size_t payloadSize);
    void setFailure(const string& message);

private:
    bool _winsockReady = false;
    SOCKET _listenSocket = INVALID_SOCKET;
    SOCKET _connectionSocket = INVALID_SOCKET;
    vector<uint8_t> _recvBuffer;
    uint32_t _nextSequence = 1;
    SandforgeLobbyState _lobbyState;
    SandforgeWorldSnapshot _latestSnapshot;
    bool _hasLatestSnapshot = false;
    vector<SandforgeNetCommand> _pendingCommands;
};

#endif // !SANDFORGE_MULTIPLAYER_SESSION_H_
