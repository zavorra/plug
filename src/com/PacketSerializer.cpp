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

#include "com/PacketSerializer.h"
#include "com/IdLookup.h"
#include "effects_enum.h"
#include <algorithm>

namespace plug::com
{
    namespace
    {
        template <class T, T upperBound>
        constexpr T clampToRange(T value)
        {
            return std::clamp(value, T{0}, upperBound);
        }

        constexpr std::uint8_t getFxKnob(const fx_pedal_settings& effect)
        {
            if ((effect.effect_num >= effects::SINE_CHORUS) && (effect.effect_num <= effects::PITCH_SHIFTER))
            {
                return 0x01;
            }
            return 0x02;
        }

        constexpr std::uint8_t getSlot(const fx_pedal_settings& effect)
        {
            if (effect.position == Position::effectsLoop)
            {
                constexpr std::uint8_t fxLoopOffset{4};
                return effect.fx_slot + fxLoopOffset;
            }
            return effect.fx_slot;
        }


        constexpr bool hasExtraKnob(effects e)
        {
            switch (e)
            {
                case effects::MONO_ECHO_FILTER:
                case effects::STEREO_ECHO_FILTER:
                case effects::TAPE_DELAY:
                case effects::STEREO_TAPE_DELAY:
                    return true;
                default:
                    return false;
            }
        }


        std::size_t getSaveEffectsRepeats(const std::vector<fx_pedal_settings>& effects)
        {
            const auto size = effects.size();

            if (size > 2)
            {
                return 1;
            }
            const auto effect = effects[0].effect_num;

            if ((effect >= effects::SINE_CHORUS) && (effect <= effects::PITCH_SHIFTER))
            {
                return 1;
            }
            return size;
        }

    }


    std::string decodeNameFromData(const Packet<NamePayload>& packet)
    {
        return packet.getPayload().getName();
    }

    amp_settings decodeAmpFromData(const Packet<AmpPayload>& packet, const Packet<AmpPayload>& packetUsbGain)
    {
        const auto payload = packet.getPayload();

        amp_settings settings{};
        settings.amp_num = lookupAmpById(payload.getModel());
        settings.gain = payload.getGain();
        settings.volume = payload.getVolume();
        settings.treble = payload.getTreble();
        settings.middle = payload.getMiddle();
        settings.bass = payload.getBass();
        settings.cabinet = lookupCabinetById(payload.getCabinet());
        settings.noise_gate = payload.getNoiseGate();
        settings.master_vol = payload.getMasterVolume();
        settings.gain2 = payload.getGain2();
        settings.presence = payload.getPresence();
        settings.threshold = payload.getThreshold();
        settings.depth = payload.getDepth();
        settings.bias = payload.getBias();
        settings.sag = payload.getSag();
        settings.brightness = payload.getBrightness();
        settings.usb_gain = packetUsbGain.getPayload().getUsbGain();
        return settings;
    }

    std::array<fx_pedal_settings, 4> decodeEffectsFromData(const std::array<Packet<EffectPayload>, 4>& packet)
    {
        std::array<fx_pedal_settings, 4> effects{{}};

        std::for_each(packet.cbegin(), packet.cend(), [&effects](const auto& p) {
            const auto payload = p.getPayload();
            const auto slot = payload.getSlot() % 4;
            effects[slot].fx_slot = slot;
            effects[slot].knob1 = payload.getKnob1();
            effects[slot].knob2 = payload.getKnob2();
            effects[slot].knob3 = payload.getKnob3();
            effects[slot].knob4 = payload.getKnob4();
            effects[slot].knob5 = payload.getKnob5();
            effects[slot].knob6 = payload.getKnob6();
            effects[slot].position = (payload.getSlot() > 0x03 ? Position::effectsLoop : Position::input);
            effects[slot].effect_num = lookupEffectById(payload.getModel());
        });

        return effects;
    }

    std::vector<std::string> decodePresetListFromData(const std::vector<Packet<NamePayload>>& packets)
    {
        const auto max_to_receive = std::min<std::size_t>(packets.size(), (packets.size() > 143 ? 200 : 48));
        std::vector<std::string> presetNames;
        presetNames.reserve(max_to_receive);

        for (std::size_t i = 0; i < max_to_receive; i += 2)
        {
            presetNames.push_back(packets[i].getPayload().getName());
        }

        return presetNames;
    }

    Packet<AmpPayload> serializeAmpSettings(const amp_settings& value)
    {
        Header header{};
        header.setStage(Stage::ready);
        header.setType(Type::data);
        header.setDSP(DSP::amp);
        header.setUnknown(0x00, 0x01, 0x01);

        AmpPayload payload{};
        payload.setVolume(value.volume);
        payload.setGain(value.gain);
        payload.setGain2(value.gain2);
        payload.setMasterVolume(value.master_vol);
        payload.setTreble(value.treble);
        payload.setMiddle(value.middle);
        payload.setBass(value.bass);
        payload.setPresence(value.presence);
        payload.setBias(value.bias);
        payload.setNoiseGate(clampToRange<std::uint8_t, 0x05>(value.noise_gate));
        payload.setCabinet(plug::value(value.cabinet));
        payload.setSag(clampToRange<std::uint8_t, 0x02>(value.sag));
        payload.setBrightness(value.brightness);
        payload.setUnknown(0x80, 0x80, 0x01);

        if (value.noise_gate == 0x05)
        {
            payload.setThreshold(clampToRange<uint8_t, 0x09>(value.threshold));
            payload.setDepth(value.depth);
        }
        else
        {
            payload.setDepth(0x80);
        }

        switch (value.amp_num)
        {
            case amps::FENDER_57_DELUXE:
                payload.setModel(0x67);
                payload.setUnknownAmpSpecific(0x01, 0x01, 0x01, 0x01, 0x53);
                break;

            case amps::FENDER_59_BASSMAN:
                payload.setModel(0x64);
                payload.setUnknownAmpSpecific(0x02, 0x02, 0x02, 0x02, 0x67);
                break;

            case amps::FENDER_57_CHAMP:
                payload.setModel(0x7c);
                payload.setUnknownAmpSpecific(0x0c, 0x0c, 0x0c, 0x0c, 0x00);
                break;

            case amps::FENDER_65_DELUXE_REVERB:
                payload.setModel(0x53);
                payload.setUnknownAmpSpecific(0x03, 0x03, 0x03, 0x03, 0x6a);
                payload.setUnknown(0x00, 0x00, 0x01);
                break;

            case amps::FENDER_65_PRINCETON:
                payload.setModel(0x6a);
                payload.setUnknownAmpSpecific(0x04, 0x04, 0x04, 0x04, 0x61);
                break;

            case amps::FENDER_65_TWIN_REVERB:
                payload.setModel(0x75);
                payload.setUnknownAmpSpecific(0x05, 0x05, 0x05, 0x05, 0x72);
                break;

            case amps::FENDER_SUPER_SONIC:
                payload.setModel(0x72);
                payload.setUnknownAmpSpecific(0x06, 0x06, 0x06, 0x06, 0x79);
                break;

            case amps::BRITISH_60S:
                payload.setModel(0x61);
                payload.setUnknownAmpSpecific(0x07, 0x07, 0x07, 0x07, 0x5e);
                break;

            case amps::BRITISH_70S:
                payload.setModel(0x79);
                payload.setUnknownAmpSpecific(0x0b, 0x0b, 0x0b, 0x0b, 0x7c);
                break;

            case amps::BRITISH_80S:
                payload.setModel(0x5e);
                payload.setUnknownAmpSpecific(0x09, 0x09, 0x09, 0x09, 0x5d);
                break;

            case amps::AMERICAN_90S:
                payload.setModel(0x5d);
                payload.setUnknownAmpSpecific(0x0a, 0x0a, 0x0a, 0x0a, 0x6d);
                break;

            case amps::METAL_2000:
                payload.setModel(0x6d);
                payload.setUnknownAmpSpecific(0x08, 0x08, 0x08, 0x08, 0x75);
                break;

            //V2 Only
            case amps::STUDIO_PREAMP:
                payload.setModel(0xf1);
                payload.setUnknownAmpSpecific(0x0d, 0x0d, 0x0d, 0x0d,0xf6);
                break;
            case amps::FENDER_57_TWIN:
                payload.setModel(0xf6);
                payload.setUnknownAmpSpecific(0x0e, 0x0e, 0x0e, 0x0e, 0xf9);
                break;
            case amps::SIXTIES_THRIFT:
                payload.setModel(0xf9);
                payload.setUnknownAmpSpecific(0x0f, 0x0f, 0x0f, 0x0f, 0xfc);
                break;
            case amps::BRITTISH_WATTS:
                payload.setModel(0xff);
                payload.setUnknownAmpSpecific(0x11, 0x11, 0x11, 0x11, 0x00);
                break;
            case amps::BRITTISH_COLOUR:
                payload.setModel(0xfc);
                payload.setUnknownAmpSpecific(0x10, 0x10, 0x10, 010, 0xff);
                break;
        }

        Packet<AmpPayload> packet{};
        packet.setHeader(header);
        packet.setPayload(payload);
        return packet;
    }

    Packet<AmpPayload> serializeAmpSettingsUsbGain(const amp_settings& value)
    {
        Header header{};
        header.setStage(Stage::ready);
        header.setType(Type::data);
        header.setDSP(DSP::usbGain);
        header.setUnknown(0x00, 0x01, 0x01);

        AmpPayload payload{};
        payload.setUsbGain(value.usb_gain);

        Packet<AmpPayload> packet{};
        packet.setHeader(header);
        packet.setPayload(payload);
        return packet;
    }

    Packet<NamePayload> serializeName(std::uint8_t slot, std::string_view name)
    {
        Header header{};
        header.setStage(Stage::ready);
        header.setType(Type::operation);
        header.setDSP(DSP::opSave);
        header.setSlot(slot);
        header.setUnknown(0x00, 0x01, 0x01);

        NamePayload payload{};
        payload.setName(name);

        Packet<NamePayload> packet{};
        packet.setHeader(header);
        packet.setPayload(payload);
        return packet;
    }

    Packet<EffectPayload> serializeEffectSettings(const fx_pedal_settings& value)
    {
        Header header{};
        header.setStage(Stage::ready);
        header.setType(Type::data);
        header.setUnknown(0x00, 0x01, 0x01);

        printf("serializeEffectSettings: slot: %d, model: %02x ",
                static_cast<int>(value.fx_slot)
                ,static_cast<int>(value.effect_num));
        EffectPayload payload{};
        payload.setSlot(getSlot(value));
        payload.setUnknown(0x00, 0x08, 0x01);
        payload.setKnob1(value.knob1);
        payload.setKnob2(value.knob2);
        payload.setKnob3(value.knob3);
        payload.setKnob4(value.knob4);
        payload.setKnob5(value.knob5);

        if (hasExtraKnob(value.effect_num) == true)
        {
            payload.setKnob6(value.knob6);
        }

        switch (value.effect_num)
        {
            case effects::OVERDRIVE:
                header.setDSP(DSP::effect0);
                payload.setModel(0x3c);
                break;

            case effects::WAH:
                header.setDSP(DSP::effect0);
                payload.setModel(0x49);
                payload.setUnknown(0x01, 0x08, 0x01);
                break;

            case effects::TOUCH_WAH:
                header.setDSP(DSP::effect0);
                payload.setModel(0x4a);
                payload.setUnknown(0x01, 0x08, 0x01);
                break;

            case effects::FUZZ:
                header.setDSP(DSP::effect0);
                payload.setModel(0x1a);
                break;

            case effects::FUZZ_TOUCH_WAH:
                header.setDSP(DSP::effect0);
                payload.setModel(0x1c);
                break;

            case effects::SIMPLE_COMP:
                header.setDSP(DSP::effect0);
                payload.setModel(0x88);
                payload.setKnob1(clampToRange<std::uint8_t, 0x03>(value.knob1));
                payload.setKnob2(0x00);
                payload.setKnob3(0x00);
                payload.setKnob4(0x00);
                payload.setKnob5(0x00);
                payload.setUnknown(0x08, 0x08, 0x01);
                break;

            case effects::COMPRESSOR:
                header.setDSP(DSP::effect0);
                payload.setModel(0x07);
                break;

	    case effects::RANGE_BOOST:
                header.setDSP(DSP::effect0);
                payload.setModel(0x0103);
                payload.setUnknown(0x00, 0x08, 0x01);
                break;

	    case effects::GREEN_BOX:
                header.setDSP(DSP::effect0);
                payload.setModel(0xba);
                payload.setUnknown(0x00, 0x08, 0x01);
                break;

	    case effects::ORANGE_BOX:
                header.setDSP(DSP::effect0);
                payload.setModel(0x0110);
                payload.setUnknown(0x00, 0x08, 0x01);
                break;

	    case effects::BLACK_BOX:
                header.setDSP(DSP::effect0);
                payload.setModel(0x0111);
                payload.setUnknown(0x00, 0x08, 0x01);
                break;

	    case effects::BIG_FUZZ:
                header.setDSP(DSP::effect0);
                payload.setModel(0x010f);
                payload.setUnknown(0x00, 0x08, 0x01);
                break;

            case effects::SINE_CHORUS:
                header.setDSP(DSP::effect1);
                payload.setModel(0x12);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::TRIANGLE_CHORUS:
                header.setDSP(DSP::effect1);
                payload.setModel(0x13);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::SINE_FLANGER:
                header.setDSP(DSP::effect1);
                payload.setModel(0x18);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::TRIANGLE_FLANGER:
                header.setDSP(DSP::effect1);
                payload.setModel(0x19);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::VIBRATONE:
                header.setDSP(DSP::effect1);
                payload.setModel(0x2d);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::VINTAGE_TREMOLO:
                header.setDSP(DSP::effect1);
                payload.setModel(0x40);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::SINE_TREMOLO:
                header.setDSP(DSP::effect1);
                payload.setModel(0x41);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::RING_MODULATOR:
                header.setDSP(DSP::effect1);
                payload.setModel(0x22);
                payload.setKnob4(clampToRange<std::uint8_t, 0x01>(value.knob4));
                payload.setUnknown(0x01, 0x08, 0x01);
                break;

            case effects::STEP_FILTER:
                header.setDSP(DSP::effect1);
                payload.setModel(0x29);
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::PHASER:
                header.setDSP(DSP::effect1);
                payload.setModel(0x4f);
                payload.setKnob5(clampToRange<std::uint8_t, 0x01>(value.knob5));
                payload.setUnknown(0x01, 0x01, 0x01);
                break;

            case effects::PITCH_SHIFTER:
                header.setDSP(DSP::effect1);
                payload.setModel(0x1f);
                payload.setUnknown(0x01, 0x08, 0x01);
                break;

            case effects::MONO_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x16);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::MONO_ECHO_FILTER:
                header.setDSP(DSP::effect2);
                payload.setModel(0x43);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::STEREO_ECHO_FILTER:
                header.setDSP(DSP::effect2);
                payload.setModel(0x48);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::MULTITAP_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x44);
                payload.setKnob5(clampToRange<std::uint8_t, 0x03>(value.knob5));
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::PING_PONG_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x45);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::DUCKING_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x15);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::REVERSE_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x46);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::TAPE_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x2b);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::STEREO_TAPE_DELAY:
                header.setDSP(DSP::effect2);
                payload.setModel(0x2a);
                payload.setUnknown(0x02, 0x01, 0x01);
                break;

            case effects::SMALL_HALL_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x24);
                break;

            case effects::LARGE_HALL_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x3a);
                break;

            case effects::SMALL_ROOM_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x26);
                break;

            case effects::LARGE_ROOM_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x3b);
                break;

            case effects::SMALL_PLATE_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x4e);
                break;

            case effects::LARGE_PLATE_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x4b);
                break;

            case effects::AMBIENT_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x4c);
                break;

            case effects::ARENA_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x4d);
                break;

            case effects::FENDER_63_SPRING_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x21);
                break;

            case effects::FENDER_65_SPRING_REVERB:
                header.setDSP(DSP::effect3);
                payload.setModel(0x0b);
                break;

            default:
                break;
        }
        printf(" DSP: %d, model: %02x\n"
                ,static_cast<int>(payload.getSlot())
                ,static_cast<int>(payload.getModel())
                );


        Packet<EffectPayload> packet{};
        packet.setHeader(header);
        packet.setPayload(payload);
        return packet;
    }

    Packet<EffectPayload> serializeClearEffectSettings()
    {
        Header h{};
        h.setStage(Stage::ready);
        h.setType(Type::data);
        h.setDSP(DSP::none);
        h.setUnknown(0x00, 0x01, 0x01);
        EffectPayload payload{};
        payload.setUnknown(0x00, 0x08, 0x01);

        Packet<EffectPayload> applyCommand{};
        applyCommand.setHeader(h);
        applyCommand.setPayload(payload);
        return applyCommand;
    }

    Packet<NamePayload> serializeSaveEffectName(std::uint8_t slot, std::string_view name, const std::vector<fx_pedal_settings>& effects)
    {
        const std::size_t repeat = getSaveEffectsRepeats(effects);

        for (std::size_t i = 0; i < repeat; ++i)
        {
            if (effects[i].effect_num < effects::SINE_CHORUS)
            {
                throw std::invalid_argument{"Invalid effect"};
            }
        }

        Header header{};
        header.setStage(Stage::ready);
        header.setType(Type::operation);
        header.setDSP(DSP::opSaveEffectName);
        header.setSlot(slot);
        header.setUnknown(getFxKnob(effects[0]), 0x01, 0x01);

        constexpr std::size_t nameLength{24};
        NamePayload payload{};
        payload.setName(name.substr(0, nameLength));

        Packet<NamePayload> packet{};
        packet.setHeader(header);
        packet.setPayload(payload);
        return packet;
    }

    std::vector<Packet<EffectPayload>> serializeSaveEffectPacket(std::uint8_t slot, const std::vector<fx_pedal_settings>& effects)
    {
        const auto fxKnob = getFxKnob(effects[0]);
        const std::size_t repeat = getSaveEffectsRepeats(effects);

        for (std::size_t i = 0; i < repeat; ++i)
        {
            if (effects[i].effect_num < effects::SINE_CHORUS)
            {
                throw std::invalid_argument{"Invalid effect"};
            }
        }

        std::vector<Packet<EffectPayload>> packets;

        for (std::size_t i = 0; i < repeat; ++i)
        {
            auto packet = serializeEffectSettings(effects[i]);
            auto header = packet.getHeader();
            header.setSlot(slot);
            header.setUnknown(fxKnob, 0x00, 0x01);
            packet.setHeader(header);
            packets.push_back(packet);
        }

        return packets;
    }

    Packet<EmptyPayload> serializeLoadSlotCommand(std::uint8_t slot)
    {
        Header h{};
        h.setStage(Stage::ready);
        h.setType(Type::operation);
        h.setDSP(DSP::opSelectMemBank);
        h.setSlot(slot);
        h.setUnknown(0x00, 0x01, 0x00);

        Packet<EmptyPayload> loadCommand{};
        loadCommand.setHeader(h);
        loadCommand.setPayload(EmptyPayload{});
        return loadCommand;
    }

    Packet<EmptyPayload> serializeLoadCommand()
    {
        Header h{};
        h.setStage(Stage::unknown);
        h.setType(Type::load);
        h.setDSP(DSP::none);
        Packet<EmptyPayload> loadCommand{};
        loadCommand.setHeader(h);
        loadCommand.setPayload(EmptyPayload{});
        return loadCommand;
    }

    Packet<EmptyPayload> serializeApplyCommand()
    {
        Header h{};
        h.setStage(Stage::ready);
        h.setType(Type::data);
        h.setDSP(DSP::none);
        Packet<EmptyPayload> applyCommand{};
        applyCommand.setHeader(h);
        applyCommand.setPayload(EmptyPayload{});
        return applyCommand;
    }

    Packet<EmptyPayload> serializeApplyCommand(fx_pedal_settings effect)
    {
        auto applyCommand = serializeApplyCommand();
        auto header = applyCommand.getHeader();
        header.setUnknown(getFxKnob(effect), 0x00, 0x00);
        applyCommand.setHeader(header);
        return applyCommand;
    }

    std::array<Packet<EmptyPayload>, 2> serializeInitCommand()
    {
        Header h0{};
        h0.setStage(Stage::init0);
        h0.setType(Type::init0);
        h0.setDSP(DSP::none);
        Packet<EmptyPayload> p0{};
        p0.setHeader(h0);
        p0.setPayload(EmptyPayload{});

        Header h1{};
        h1.setStage(Stage::init1);
        h1.setType(Type::init1);
        h1.setDSP(DSP::none);
        Packet<EmptyPayload> p1{};
        p1.setHeader(h1);
        p1.setPayload(EmptyPayload{});

        return {{p0, p1}};
    }
}
