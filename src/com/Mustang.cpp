/*
 * PLUG - software to operate Fender Mustang amplifier
 *        Linux replacement for Fender FUSE software
 *
 * Copyright (C) 2017-2020  offa
 * Copyright (C) 2010-2016  piorekf <piorek@piorekf.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "com/Mustang.h"
#include "com/PacketSerializer.h"
#include "com/CommunicationException.h"
#include "com/Packet.h"
#include <algorithm>

namespace plug::com
{
    SignalChain decode_data(const std::array<PacketRawType, 7>& data)
    {
        const auto name = decodeNameFromData(fromRawData<NamePayload>(data[0]));
        const auto amp = decodeAmpFromData(fromRawData<AmpPayload>(data[1]), fromRawData<AmpPayload>(data[6]));
        const auto effects = decodeEffectsFromData({{fromRawData<EffectPayload>(data[2]), fromRawData<EffectPayload>(data[3]),
                                                     fromRawData<EffectPayload>(data[4]), fromRawData<EffectPayload>(data[5])}});

        return SignalChain{name, amp, effects};
    }

    std::vector<std::uint8_t> receivePacket(Connection& conn)
    {
        return conn.receive(packetRawTypeSize);
    }


    void sendCommand(Connection& conn, const PacketRawType& packet)
    {
        conn.send(packet);

        receivePacket(conn);
    }


    void sendApplyCommand(Connection& conn)
    {
        sendCommand(conn, serializeApplyCommand().getBytes());
    }

    std::array<PacketRawType, 7> loadBankData(Connection& conn, std::uint8_t slot)
    {
        std::array<PacketRawType, 7> data{{}};
    //printf("loadBankData: \n");

        const auto loadCommand = serializeLoadSlotCommand(slot);
        auto n = conn.send(loadCommand.getBytes());

        for (std::size_t i = 0; n != 0; ++i)
        {
            //printf("i: %d ",static_cast<int>(i));

            const auto recvData = receivePacket(conn);
            n = recvData.size();
            //printf("n: %d ",static_cast<int>(n));

            if (i < 7)
            {
                std::copy(recvData.cbegin(), recvData.cend(), data[i].begin());
            }
        }
        return data;
    }


    Mustang::Mustang(std::shared_ptr<Connection> connection)
        : conn(connection),
	stopTuner(false)
    {
    }

    InitalData Mustang::start_amp()
    {
        if (conn->isOpen() == false)
        {
            throw CommunicationException{"Device not connected"};
        }

        initializeAmp();

        return loadData();
    }

    void Mustang::stop_amp()
    {
        conn->close();
    }

    void Mustang::set_effect(fx_pedal_settings value)
    {
        const auto clearEffectPacket = serializeClearEffectSettings();
        sendCommand(*conn, clearEffectPacket.getBytes());
        sendApplyCommand(*conn);
        printf("mustang::set_effect: \n");
        if (value.effect_num != effects::EMPTY)
        {
            const auto settingsPacket = serializeEffectSettings(value);
            sendCommand(*conn, settingsPacket.getBytes());
            sendApplyCommand(*conn);
        }
    }

    void Mustang::set_amplifier(amp_settings value)
    {
        const auto settingsPacket = serializeAmpSettings(value);
        sendCommand(*conn, settingsPacket.getBytes());
        sendApplyCommand(*conn);

        const auto settingsGainPacket = serializeAmpSettingsUsbGain(value);
        sendCommand(*conn, settingsGainPacket.getBytes());
        sendApplyCommand(*conn);
    }

    void Mustang::save_on_amp(std::string_view name, std::uint8_t slot)
    {
        printf("mustang::saveonamp:\n");
        const auto data = serializeName(slot, name).getBytes();
        sendCommand(*conn, data);
        loadBankData(*conn, slot);
    }

    SignalChain Mustang::load_memory_bank(std::uint8_t slot)
    {
        return decode_data(loadBankData(*conn, slot));
    }

    void Mustang::save_effects(std::uint8_t slot, std::string_view name, const std::vector<fx_pedal_settings>& effects)
    {
        printf("mustang::save_effects:\n");
        const auto saveNamePacket = serializeSaveEffectName(slot, name, effects);
        sendCommand(*conn, saveNamePacket.getBytes());

        const auto packets = serializeSaveEffectPacket(slot, effects);
        std::for_each(packets.cbegin(), packets.cend(), [this](const auto& p) { sendCommand(*conn, p.getBytes()); });

        sendCommand(*conn, serializeApplyCommand(effects[0]).getBytes());
    }

    InitalData Mustang::loadData()
    {
        std::vector<std::array<std::uint8_t, 64>> recieved_data;

        const auto loadCommand = serializeLoadCommand();
        auto recieved = conn->send(loadCommand.getBytes());

        while (recieved != 0)
        {
            const auto recvData = receivePacket(*conn);
            recieved = recvData.size();
            PacketRawType p{};
            std::copy(recvData.cbegin(), recvData.cend(), p.begin());
            recieved_data.push_back(p);
        }

        const std::size_t max_to_receive = (recieved_data.size() > 143 ? 200 : 48);
        std::vector<Packet<NamePayload>> presetListData;
        presetListData.reserve(max_to_receive);
        std::transform(recieved_data.cbegin(), std::next(recieved_data.cbegin(), max_to_receive), std::back_inserter(presetListData), [](const auto& p) {
            Packet<NamePayload> packet{};
            packet.fromBytes(p);
            return packet;
        });
        auto presetNames = decodePresetListFromData(presetListData);

        std::array<PacketRawType, 7> presetData{{}};
        std::copy(std::next(recieved_data.cbegin(), max_to_receive), std::next(recieved_data.cbegin(), max_to_receive + 7), presetData.begin());

        return {decode_data(presetData), presetNames};
    }

    void Mustang::initializeAmp()
    {
	sendTunerCommand(false);
        const auto packets = serializeInitCommand();
        std::for_each(packets.cbegin(), packets.cend(), [this](const auto& p) { sendCommand(*conn, p.getBytes()); });
    }
    
    void Mustang::sendTunerCommand(bool tuner_on)
    {
printf("tuner: %d\n",tuner_on);

	stopTuner=!tuner_on;

        sendCommand(*conn, serializeTunerCommand(tuner_on).getBytes() );
        auto recvData = receivePacket(*conn);
        auto recieved = recvData.size();
        if (tuner_on)  {
            while ((recieved != 0) && (stopTuner==false))
            {
                recvData = receivePacket(*conn);
                recieved = recvData.size();
            }
        }
       //sendCommand(*conn, clearEffectPacket.getBytes());
    }

}
