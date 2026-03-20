#include "pch.h"
#include "SandforgeMultiplayerSession.h"

namespace
{
    constexpr uint16_t kDefaultLobbyPort = 42042;
    constexpr size_t kJoinNameLength = 32;

    void writeUnitSnapshot(SandforgePacketWriter& writer, const SandforgeUnit& unit)
    {
        writer.writeUInt32(unit.id);
        writer.writeUInt32(unit.ownerId);
        writer.writeUInt8(static_cast<uint8_t>(unit.unitType));
        writer.writeUInt8(static_cast<uint8_t>(unit.state));
        writer.writeUInt8(unit.alive ? 1 : 0);
        writer.writeFloat(unit.position.x);
        writer.writeFloat(unit.position.y);
        writer.writeFloat(unit.moveTarget.x);
        writer.writeFloat(unit.moveTarget.y);
        writer.writeFloat(unit.hp);
        writer.writeFloat(unit.maxHp);
        writer.writeUInt32(unit.targetId);
        writer.writeUInt32(unit.captureNodeId);
        writer.writeFloat(unit.gatherProgress);
        writer.writeUInt32(static_cast<uint32_t>(unit.carriedAmount));
        writer.writeUInt8(static_cast<uint8_t>(unit.carriedResourceType));
    }

    bool readUnitSnapshot(SandforgePacketReader& reader, SandforgeUnit& unit)
    {
        uint8_t unitType = 0;
        uint8_t state = 0;
        uint8_t alive = 0;
        uint32_t carriedAmount = 0;
        uint8_t resourceType = 0;
        return reader.readUInt32(unit.id) &&
            reader.readUInt32(unit.ownerId) &&
            reader.readUInt8(unitType) &&
            reader.readUInt8(state) &&
            reader.readUInt8(alive) &&
            reader.readFloat(unit.position.x) &&
            reader.readFloat(unit.position.y) &&
            reader.readFloat(unit.moveTarget.x) &&
            reader.readFloat(unit.moveTarget.y) &&
            reader.readFloat(unit.hp) &&
            reader.readFloat(unit.maxHp) &&
            reader.readUInt32(unit.targetId) &&
            reader.readUInt32(unit.captureNodeId) &&
            reader.readFloat(unit.gatherProgress) &&
            reader.readUInt32(carriedAmount) &&
            reader.readUInt8(resourceType) &&
            ((unit.unitType = static_cast<SandforgeUnitType>(unitType)), true) &&
            ((unit.state = static_cast<SandforgeUnitState>(state)), true) &&
            ((unit.alive = alive != 0), true) &&
            ((unit.carriedAmount = static_cast<int>(carriedAmount)), true) &&
            ((unit.carriedResourceType = static_cast<SandforgeResourceType>(resourceType)), true);
    }

    void writeProductionItem(SandforgePacketWriter& writer, const SandforgeProductionItem& item)
    {
        writer.writeUInt8(static_cast<uint8_t>(item.unitType));
        writer.writeFloat(item.totalTime);
        writer.writeFloat(item.remainingTime);
        writer.writeUInt8(item.repeat ? 1 : 0);
    }

    bool readProductionItem(SandforgePacketReader& reader, SandforgeProductionItem& item)
    {
        uint8_t unitType = 0;
        uint8_t repeat = 0;
        return reader.readUInt8(unitType) &&
            reader.readFloat(item.totalTime) &&
            reader.readFloat(item.remainingTime) &&
            reader.readUInt8(repeat) &&
            ((item.unitType = static_cast<SandforgeUnitType>(unitType)), true) &&
            ((item.repeat = repeat != 0), true);
    }

    void writeBuildingSnapshot(SandforgePacketWriter& writer, const SandforgeBuilding& building)
    {
        writer.writeUInt32(building.id);
        writer.writeUInt32(building.ownerId);
        writer.writeUInt8(static_cast<uint8_t>(building.buildingType));
        writer.writeUInt8(building.alive ? 1 : 0);
        writer.writeFloat(building.position.x);
        writer.writeFloat(building.position.y);
        writer.writeFloat(building.hp);
        writer.writeFloat(building.maxHp);
        writer.writeUInt8(building.underConstruction ? 1 : 0);
        writer.writeUInt16(static_cast<uint16_t>(building.productionQueue.size()));
        for (const SandforgeProductionItem& item : building.productionQueue)
        {
            writeProductionItem(writer, item);
        }
    }

    bool readBuildingSnapshot(SandforgePacketReader& reader, SandforgeBuilding& building)
    {
        uint8_t buildingType = 0;
        uint8_t alive = 0;
        uint8_t underConstruction = 0;
        uint16_t queueCount = 0;
        if (!(reader.readUInt32(building.id) &&
            reader.readUInt32(building.ownerId) &&
            reader.readUInt8(buildingType) &&
            reader.readUInt8(alive) &&
            reader.readFloat(building.position.x) &&
            reader.readFloat(building.position.y) &&
            reader.readFloat(building.hp) &&
            reader.readFloat(building.maxHp) &&
            reader.readUInt8(underConstruction) &&
            reader.readUInt16(queueCount)))
        {
            return false;
        }

        building.buildingType = static_cast<SandforgeBuildingType>(buildingType);
        building.alive = alive != 0;
        building.underConstruction = underConstruction != 0;
        building.productionQueue.clear();
        for (uint16_t index = 0; index < queueCount; ++index)
        {
            SandforgeProductionItem item;
            if (!readProductionItem(reader, item))
            {
                return false;
            }

            building.productionQueue.push_back(item);
        }

        return true;
    }

    void writeNodeSnapshot(SandforgePacketWriter& writer, const SandforgeResourceNode& node)
    {
        writer.writeUInt32(node.id);
        writer.writeUInt32(node.ownerId);
        writer.writeUInt8(static_cast<uint8_t>(node.resourceType));
        writer.writeUInt8(node.alive ? 1 : 0);
        writer.writeFloat(node.position.x);
        writer.writeFloat(node.position.y);
        writer.writeFloat(node.captureProgress);
        writer.writeFloat(node.captureRequiredTime);
        writer.writeUInt32(node.capturingWorkerId);
        writer.writeUInt32(node.harvestingWorkerId);
    }

    bool readNodeSnapshot(SandforgePacketReader& reader, SandforgeResourceNode& node)
    {
        uint8_t resourceType = 0;
        uint8_t alive = 0;
        return reader.readUInt32(node.id) &&
            reader.readUInt32(node.ownerId) &&
            reader.readUInt8(resourceType) &&
            reader.readUInt8(alive) &&
            reader.readFloat(node.position.x) &&
            reader.readFloat(node.position.y) &&
            reader.readFloat(node.captureProgress) &&
            reader.readFloat(node.captureRequiredTime) &&
            reader.readUInt32(node.capturingWorkerId) &&
            reader.readUInt32(node.harvestingWorkerId) &&
            ((node.resourceType = static_cast<SandforgeResourceType>(resourceType)), true) &&
            ((node.alive = alive != 0), true);
    }
}

SandforgeMultiplayerSession::SandforgeMultiplayerSession()
{
}

SandforgeMultiplayerSession::~SandforgeMultiplayerSession()
{
    disconnect();
}

bool SandforgeMultiplayerSession::host(uint16_t port)
{
    const string localName = _lobbyState.localPlayerName.empty() ? "Player" : _lobbyState.localPlayerName;
    disconnect();
    if (!ensureWinsock())
    {
        setFailure("Winsock initialization failed.");
        return false;
    }

    _listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSocket == INVALID_SOCKET)
    {
        setFailure("Failed to create host socket.");
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port == 0 ? kDefaultLobbyPort : port);

    if (::bind(_listenSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        setFailure("Failed to bind multiplayer host port.");
        return false;
    }

    if (::listen(_listenSocket, 1) == SOCKET_ERROR)
    {
        setFailure("Failed to listen for multiplayer client.");
        return false;
    }

    makeSocketNonBlocking(_listenSocket);
    _lobbyState.mode = SandforgeMultiplayerMode::Host;
    _lobbyState.connectionState = SandforgeMultiplayerConnectionState::Hosting;
    _lobbyState.localPlayerId = 1;
    _lobbyState.localPlayerName = localName;
    _lobbyState.remotePlayerName = "Remote";
    _lobbyState.statusText = "Hosting on port " + to_string(port == 0 ? kDefaultLobbyPort : port) + ". Waiting for another player...";
    return true;
}

bool SandforgeMultiplayerSession::join(const string& addressText, uint16_t port)
{
    const string localName = _lobbyState.localPlayerName.empty() ? "Player" : _lobbyState.localPlayerName;
    disconnect();
    if (!ensureWinsock())
    {
        setFailure("Winsock initialization failed.");
        return false;
    }

    _connectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_connectionSocket == INVALID_SOCKET)
    {
        setFailure("Failed to create client socket.");
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port == 0 ? kDefaultLobbyPort : port);
    if (inet_pton(AF_INET, addressText.c_str(), &address.sin_addr) != 1)
    {
        setFailure("Invalid host address.");
        return false;
    }

    _lobbyState.mode = SandforgeMultiplayerMode::Client;
    _lobbyState.connectionState = SandforgeMultiplayerConnectionState::Connecting;
    _lobbyState.localPlayerName = localName;
    _lobbyState.remotePlayerName = "Host";
    if (::connect(_connectionSocket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
    {
        const int errorCode = WSAGetLastError();
        if (errorCode != WSAEISCONN)
        {
            setFailure("Could not connect to host.");
            return false;
        }
    }

    makeSocketNonBlocking(_connectionSocket);
    _lobbyState.connectionState = SandforgeMultiplayerConnectionState::Connected;
    _lobbyState.remoteConnected = true;
    _lobbyState.statusText = "Connected to host. Sending join request...";
    sendJoinRequest();
    return true;
}

void SandforgeMultiplayerSession::disconnect()
{
    closeSocket(_listenSocket);
    closeSocket(_connectionSocket);
    _recvBuffer.clear();
    _nextSequence = 1;
    _lobbyState = {};
}

void SandforgeMultiplayerSession::update()
{
    if (_lobbyState.mode == SandforgeMultiplayerMode::Host)
    {
        processAccept();
    }

    processIncomingPackets();
}

void SandforgeMultiplayerSession::setLocalPlayerName(const string& playerName)
{
    if (playerName.empty())
    {
        _lobbyState.localPlayerName = "Player";
        return;
    }

    _lobbyState.localPlayerName = playerName.substr(0, kJoinNameLength);
}

void SandforgeMultiplayerSession::setLobbyMatchSettings(SandforgeStartResourcePreset resourcePreset, SandforgeStartWorkerPreset workerPreset)
{
    _lobbyState.resourcePreset = resourcePreset;
    _lobbyState.workerPreset = workerPreset;

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && _lobbyState.remoteConnected)
    {
        sendLobbySettingsPacket();
    }
}

void SandforgeMultiplayerSession::setLocalReady(bool ready)
{
    _lobbyState.localReady = ready;
    if (_lobbyState.mode == SandforgeMultiplayerMode::Client && _connectionSocket != INVALID_SOCKET)
    {
        sendReadyPacket(ready);
        _lobbyState.statusText = ready ? "Ready sent. Waiting for host..." : "Ready canceled.";
    }
    else if (_lobbyState.mode == SandforgeMultiplayerMode::Host)
    {
        _lobbyState.statusText = ready ? "You are ready. Waiting for remote player..." : "Ready canceled.";
    }
}

bool SandforgeMultiplayerSession::tryStartMatch()
{
    if (_lobbyState.mode != SandforgeMultiplayerMode::Host)
    {
        _lobbyState.statusText = "Only the host can start the match.";
        return false;
    }
    if (!_lobbyState.remoteConnected)
    {
        _lobbyState.statusText = "A remote player must join before starting.";
        return false;
    }
    if (!_lobbyState.localReady || !_lobbyState.remoteReady)
    {
        _lobbyState.statusText = "Both players must be ready before the match can start.";
        return false;
    }

    sendMatchStartPacket();
    _lobbyState.matchStartReceived = true;
    _lobbyState.statusText = "Host started the match.";
    return true;
}

bool SandforgeMultiplayerSession::consumeMatchStart()
{
    if (!_lobbyState.matchStartReceived)
    {
        return false;
    }

    _lobbyState.matchStartReceived = false;
    return true;
}

bool SandforgeMultiplayerSession::isAuthoritativeHost() const
{
    return _lobbyState.mode == SandforgeMultiplayerMode::Host;
}

void SandforgeMultiplayerSession::sendWorldSnapshot(const SandforgeWorldSnapshot& snapshot)
{
    if (_connectionSocket == INVALID_SOCKET || _lobbyState.mode != SandforgeMultiplayerMode::Host)
    {
        return;
    }

    SandforgePacketWriter writer(SandforgePacketType::S2C_WorldSnapshot, _nextSequence++);
    writer.writeFloat(static_cast<float>(snapshot.elapsedTime));
    writer.writeUInt8(snapshot.matchResult.gameOver ? 1 : 0);
    writer.writeUInt32(snapshot.matchResult.winnerPlayerId);
    writer.writeUInt8(static_cast<uint8_t>(snapshot.matchResult.reason));
    for (const SandforgePlayerState& player : snapshot.players)
    {
        writer.writeUInt32(player.playerId);
        writer.writeUInt32(static_cast<uint32_t>(player.metal));
        writer.writeUInt32(static_cast<uint32_t>(player.energy));
        writer.writeUInt32(static_cast<uint32_t>(player.controlledNodeCount));
        writer.writeUInt8(player.defeated ? 1 : 0);
    }

    writer.writeUInt32(snapshot.nextEntityId);
    writer.writeUInt16(static_cast<uint16_t>(snapshot.units.size()));
    writer.writeUInt16(static_cast<uint16_t>(snapshot.buildings.size()));
    writer.writeUInt16(static_cast<uint16_t>(snapshot.nodes.size()));
    writer.writeFixedString(snapshot.statusText, 96);

    for (const SandforgeUnit& unit : snapshot.units)
    {
        writeUnitSnapshot(writer, unit);
    }
    for (const SandforgeBuilding& building : snapshot.buildings)
    {
        writeBuildingSnapshot(writer, building);
    }
    for (const SandforgeResourceNode& node : snapshot.nodes)
    {
        writeNodeSnapshot(writer, node);
    }

    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendProduceCommand(uint32_t buildingId, SandforgeBuildingType buildingType, SandforgeUnitType unitType)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_ProduceCommand, _nextSequence++);
    writer.writeUInt32(buildingId);
    writer.writeUInt8(static_cast<uint8_t>(buildingType));
    writer.writeUInt8(static_cast<uint8_t>(unitType));
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendAssignWorkerCommand(uint32_t workerId, uint16_t nodeIndex)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_AssignWorkerCommand, _nextSequence++);
    writer.writeUInt32(workerId);
    writer.writeUInt16(nodeIndex);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendMoveUnitCommand(uint32_t unitId, const SandforgeVec2& position)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_MoveUnitCommand, _nextSequence++);
    writer.writeUInt32(unitId);
    writer.writeFloat(position.x);
    writer.writeFloat(position.y);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendBuildCommand(SandforgeBuildingType buildingType, uint16_t nodeIndex, const SandforgeVec2& position)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_BuildCommand, _nextSequence++);
    writer.writeUInt8(static_cast<uint8_t>(buildingType));
    writer.writeUInt16(nodeIndex);
    writer.writeFloat(position.x);
    writer.writeFloat(position.y);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendCancelProductionCommand(uint32_t buildingId, SandforgeBuildingType buildingType)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_CancelProduction, _nextSequence++);
    writer.writeUInt32(buildingId);
    writer.writeUInt8(static_cast<uint8_t>(buildingType));
    sendPacket(writer.buffer());
}

bool SandforgeMultiplayerSession::consumeLatestSnapshot(SandforgeWorldSnapshot& outSnapshot)
{
    if (!_hasLatestSnapshot)
    {
        return false;
    }

    outSnapshot = _latestSnapshot;
    _hasLatestSnapshot = false;
    return true;
}

vector<SandforgeNetCommand> SandforgeMultiplayerSession::consumePendingCommands()
{
    vector<SandforgeNetCommand> commands = std::move(_pendingCommands);
    _pendingCommands.clear();
    return commands;
}

const SandforgeLobbyState& SandforgeMultiplayerSession::getLobbyState() const
{
    return _lobbyState;
}

bool SandforgeMultiplayerSession::ensureWinsock()
{
    if (_winsockReady)
    {
        return true;
    }

    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        return false;
    }

    _winsockReady = true;
    return true;
}

void SandforgeMultiplayerSession::closeSocket(SOCKET& socketHandle)
{
    if (socketHandle != INVALID_SOCKET)
    {
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
    }
}

bool SandforgeMultiplayerSession::makeSocketNonBlocking(SOCKET socketHandle) const
{
    u_long nonBlocking = 1;
    return ioctlsocket(socketHandle, FIONBIO, &nonBlocking) == 0;
}

bool SandforgeMultiplayerSession::sendPacket(const vector<uint8_t>& packet)
{
    if (_connectionSocket == INVALID_SOCKET)
    {
        return false;
    }

    const int sendResult = ::send(_connectionSocket, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0);
    return sendResult == static_cast<int>(packet.size());
}

void SandforgeMultiplayerSession::sendJoinRequest()
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_JoinRequest, _nextSequence++);
    writer.writeFixedString(_lobbyState.localPlayerName.empty() ? "Player" : _lobbyState.localPlayerName, kJoinNameLength);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendJoinAccept(uint32_t playerId)
{
    SandforgePacketWriter writer(SandforgePacketType::S2C_JoinAccept, _nextSequence++);
    writer.writeUInt32(playerId);
    writer.writeFixedString(_lobbyState.localPlayerName.empty() ? "Host" : _lobbyState.localPlayerName, kJoinNameLength);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendReadyPacket(bool ready)
{
    SandforgePacketWriter writer(SandforgePacketType::C2S_Ready, _nextSequence++);
    writer.writeUInt8(ready ? 1 : 0);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendMatchStartPacket()
{
    SandforgePacketWriter writer(SandforgePacketType::S2C_MatchStart, _nextSequence++);
    writer.writeUInt32(1);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendLobbySettingsPacket()
{
    SandforgePacketWriter writer(SandforgePacketType::S2C_LobbySettings, _nextSequence++);
    writer.writeUInt8(static_cast<uint8_t>(_lobbyState.resourcePreset));
    writer.writeUInt8(static_cast<uint8_t>(_lobbyState.workerPreset));
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::sendPong(uint32_t sequence)
{
    SandforgePacketWriter writer(SandforgePacketType::S2C_Pong, sequence);
    sendPacket(writer.buffer());
}

void SandforgeMultiplayerSession::processAccept()
{
    if (_listenSocket == INVALID_SOCKET || _connectionSocket != INVALID_SOCKET)
    {
        return;
    }

    sockaddr_in remoteAddress{};
    int addressSize = sizeof(remoteAddress);
    SOCKET acceptedSocket = ::accept(_listenSocket, reinterpret_cast<sockaddr*>(&remoteAddress), &addressSize);
    if (acceptedSocket == INVALID_SOCKET)
    {
        const int errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK)
        {
            setFailure("Accept failed.");
        }
        return;
    }

    _connectionSocket = acceptedSocket;
    makeSocketNonBlocking(_connectionSocket);
    _lobbyState.connectionState = SandforgeMultiplayerConnectionState::Connected;
    _lobbyState.remoteConnected = true;
    _lobbyState.statusText = "Remote player connected. Waiting for join handshake...";
}

void SandforgeMultiplayerSession::processIncomingPackets()
{
    if (_connectionSocket == INVALID_SOCKET)
    {
        return;
    }

    array<uint8_t, 4096> recvChunk{};
    while (true)
    {
        const int recvBytes = ::recv(_connectionSocket, reinterpret_cast<char*>(recvChunk.data()), static_cast<int>(recvChunk.size()), 0);
        if (recvBytes <= 0)
        {
            const int errorCode = WSAGetLastError();
            if (recvBytes == 0)
            {
                setFailure("Remote player disconnected.");
            }
            else if (errorCode != WSAEWOULDBLOCK)
            {
                setFailure("Socket receive failed.");
            }
            break;
        }

        _recvBuffer.insert(_recvBuffer.end(), recvChunk.begin(), recvChunk.begin() + recvBytes);
    }

    while (_recvBuffer.size() >= sizeof(SandforgePacketHeader))
    {
        SandforgePacketHeader header{};
        memcpy(&header, _recvBuffer.data(), sizeof(header));
        if (header.size < sizeof(SandforgePacketHeader) || _recvBuffer.size() < header.size)
        {
            break;
        }

        const size_t payloadSize = header.size - sizeof(SandforgePacketHeader);
        const uint8_t* payload = _recvBuffer.data() + sizeof(SandforgePacketHeader);
        processPacket(header, payload, payloadSize);
        _recvBuffer.erase(_recvBuffer.begin(), _recvBuffer.begin() + header.size);
    }
}

void SandforgeMultiplayerSession::processPacket(const SandforgePacketHeader& header, const uint8_t* payload, size_t payloadSize)
{
    SandforgePacketReader reader(payload, payloadSize);
    const auto type = static_cast<SandforgePacketType>(header.type);

    if (type == SandforgePacketType::C2S_JoinRequest && _lobbyState.mode == SandforgeMultiplayerMode::Host)
    {
        string playerName;
        reader.readFixedString(playerName, kJoinNameLength);
        if (playerName.empty())
        {
            playerName = "Remote";
        }
        _lobbyState.remotePlayerName = playerName;
        _lobbyState.remoteConnected = true;
        _lobbyState.statusText = playerName.empty() ? "Player joined the lobby." : playerName + " joined the lobby.";
        sendJoinAccept(2);
        sendLobbySettingsPacket();
        return;
    }

    if (type == SandforgePacketType::S2C_JoinAccept && _lobbyState.mode == SandforgeMultiplayerMode::Client)
    {
        uint32_t playerId = 0;
        string hostName;
        reader.readUInt32(playerId);
        reader.readFixedString(hostName, kJoinNameLength);
        _lobbyState.localPlayerId = playerId;
        _lobbyState.remotePlayerName = hostName.empty() ? "Host" : hostName;
        _lobbyState.statusText = "Join accepted. Press Ready when you are set.";
        return;
    }

    if (type == SandforgePacketType::C2S_Ready && _lobbyState.mode == SandforgeMultiplayerMode::Host)
    {
        uint8_t ready = 0;
        reader.readUInt8(ready);
        _lobbyState.remoteReady = ready != 0;
        _lobbyState.statusText = _lobbyState.remoteReady ? "Remote player is ready." : "Remote player canceled ready.";
        return;
    }

    if (type == SandforgePacketType::S2C_LobbySettings)
    {
        uint8_t resourcePreset = 0;
        uint8_t workerPreset = 0;
        if (reader.readUInt8(resourcePreset) && reader.readUInt8(workerPreset))
        {
            _lobbyState.resourcePreset = static_cast<SandforgeStartResourcePreset>(resourcePreset);
            _lobbyState.workerPreset = static_cast<SandforgeStartWorkerPreset>(workerPreset);
            _lobbyState.statusText = "Host updated lobby settings.";
        }
        return;
    }

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && type == SandforgePacketType::C2S_ProduceCommand)
    {
        uint32_t buildingId = 0;
        uint8_t buildingType = 0;
        uint8_t unitType = 0;
        if (reader.readUInt32(buildingId) && reader.readUInt8(buildingType) && reader.readUInt8(unitType))
        {
            _pendingCommands.push_back({ SandforgeNetCommandType::Produce, buildingId, 0, static_cast<SandforgeBuildingType>(buildingType), static_cast<SandforgeUnitType>(unitType), {} });
        }
        return;
    }

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && type == SandforgePacketType::C2S_AssignWorkerCommand)
    {
        uint32_t workerId = 0;
        uint16_t nodeIndex = 0;
        if (reader.readUInt32(workerId) && reader.readUInt16(nodeIndex))
        {
            _pendingCommands.push_back({ SandforgeNetCommandType::AssignWorker, workerId, nodeIndex, SandforgeBuildingType::HQ, SandforgeUnitType::Worker, {} });
        }
        return;
    }

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && type == SandforgePacketType::C2S_MoveUnitCommand)
    {
        uint32_t unitId = 0;
        SandforgeVec2 position{};
        if (reader.readUInt32(unitId) && reader.readFloat(position.x) && reader.readFloat(position.y))
        {
            _pendingCommands.push_back({ SandforgeNetCommandType::MoveUnit, unitId, 0, SandforgeBuildingType::HQ, SandforgeUnitType::Worker, position });
        }
        return;
    }

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && type == SandforgePacketType::C2S_BuildCommand)
    {
        uint8_t buildingType = 0;
        uint16_t nodeIndex = 0;
        SandforgeVec2 position{};
        if (reader.readUInt8(buildingType) && reader.readUInt16(nodeIndex) && reader.readFloat(position.x) && reader.readFloat(position.y))
        {
            _pendingCommands.push_back({ SandforgeNetCommandType::Build, 0, nodeIndex, static_cast<SandforgeBuildingType>(buildingType), SandforgeUnitType::Worker, position });
        }
        return;
    }

    if (_lobbyState.mode == SandforgeMultiplayerMode::Host && type == SandforgePacketType::C2S_CancelProduction)
    {
        uint32_t buildingId = 0;
        uint8_t buildingType = 0;
        if (reader.readUInt32(buildingId) && reader.readUInt8(buildingType))
        {
            _pendingCommands.push_back({ SandforgeNetCommandType::CancelProduction, buildingId, 0, static_cast<SandforgeBuildingType>(buildingType), SandforgeUnitType::Worker, {} });
        }
        return;
    }

    if (type == SandforgePacketType::S2C_MatchStart)
    {
        uint32_t hostPlayerId = 0;
        reader.readUInt32(hostPlayerId);
        (void)hostPlayerId;
        _lobbyState.matchStartReceived = true;
        _lobbyState.statusText = "Match start packet received.";
        return;
    }

    if (type == SandforgePacketType::S2C_WorldSnapshot)
    {
        SandforgeWorldSnapshot snapshot;
        float elapsedTime = 0.0f;
        uint8_t gameOver = 0;
        uint32_t winnerPlayerId = 0;
        uint8_t reason = 0;
        uint16_t unitCount = 0;
        uint16_t buildingCount = 0;
        uint16_t nodeCount = 0;
        string statusText;

        if (!(reader.readFloat(elapsedTime) &&
            reader.readUInt8(gameOver) &&
            reader.readUInt32(winnerPlayerId) &&
            reader.readUInt8(reason)))
        {
            return;
        }

        snapshot.elapsedTime = elapsedTime;
        snapshot.matchResult.gameOver = gameOver != 0;
        snapshot.matchResult.winnerPlayerId = winnerPlayerId;
        snapshot.matchResult.reason = static_cast<SandforgeMatchEndReason>(reason);

        for (SandforgePlayerState& player : snapshot.players)
        {
            uint32_t metal = 0;
            uint32_t energy = 0;
            uint32_t controlledNodeCount = 0;
            uint8_t defeated = 0;
            if (!(reader.readUInt32(player.playerId) &&
                reader.readUInt32(metal) &&
                reader.readUInt32(energy) &&
                reader.readUInt32(controlledNodeCount) &&
                reader.readUInt8(defeated)))
            {
                return;
            }

            player.metal = static_cast<int>(metal);
            player.energy = static_cast<int>(energy);
            player.controlledNodeCount = static_cast<int>(controlledNodeCount);
            player.defeated = defeated != 0;
        }

        if (!(reader.readUInt32(snapshot.nextEntityId) &&
            reader.readUInt16(unitCount) &&
            reader.readUInt16(buildingCount) &&
            reader.readUInt16(nodeCount) &&
            reader.readFixedString(statusText, 96)))
        {
            return;
        }

        snapshot.statusText = statusText;
        snapshot.units.resize(unitCount);
        snapshot.buildings.resize(buildingCount);
        snapshot.nodes.resize(nodeCount);

        for (SandforgeUnit& unit : snapshot.units)
        {
            if (!readUnitSnapshot(reader, unit))
            {
                return;
            }
        }
        for (SandforgeBuilding& building : snapshot.buildings)
        {
            if (!readBuildingSnapshot(reader, building))
            {
                return;
            }
        }
        for (SandforgeResourceNode& node : snapshot.nodes)
        {
            if (!readNodeSnapshot(reader, node))
            {
                return;
            }
        }

        _latestSnapshot = snapshot;
        _hasLatestSnapshot = true;
        _lobbyState.statusText = "Receiving live world snapshots from host.";
        return;
    }

    if (type == SandforgePacketType::C2S_Ping && _lobbyState.mode == SandforgeMultiplayerMode::Host)
    {
        sendPong(header.sequence);
        return;
    }
}

void SandforgeMultiplayerSession::setFailure(const string& message)
{
    closeSocket(_listenSocket);
    closeSocket(_connectionSocket);
    _lobbyState.connectionState = SandforgeMultiplayerConnectionState::Failed;
    _lobbyState.statusText = message;
    _lobbyState.remoteConnected = false;
    _lobbyState.remoteReady = false;
}
