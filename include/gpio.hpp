#pragma once

#include "peripheral-utils.hpp"

#include <bitset>

#include <cstdint>

namespace Gpio {
    enum class Mode { Input, Output, Alternative, Analog };

    template <typename Peripheral, auto pin>
    constexpr void setMode(Mode mode) {
        auto ptr = &Peripheral::MODER::reg();
        auto offset = 2 * pin;
        auto mask = 3 << offset;
        *ptr = (*ptr & ~mask) |
               (mask & (static_cast<std::uint32_t>(mode) << offset));
    }

    template <typename Peripheral, auto pin>
    struct Pulse {
        Pulse() {
            Peripheral::BSRR::clear(pin);
            Peripheral::BSRR::set(pin);
        }

        ~Pulse() { Peripheral::BSRR::clear(pin); }
    };

    template <typename Peripheral, auto pin>
    struct PinBase {
        ~PinBase() { setMode<Peripheral, pin>(Mode::Input); }
    };

    template <typename PeripheralType, auto num>
    struct Input : PinBase<PeripheralType, num> {
        using Peripheral = PeripheralType;
        static auto const pin = num;

        Input() { setMode<Peripheral, pin>(Mode::Input); }
    };

    template <typename PeripheralType, auto num>
    struct Output : PinBase<PeripheralType, num> {
        using Peripheral = PeripheralType;
        static auto const pin = num;

        Output() { setMode<Peripheral, pin>(Mode::Output); }

        void set() { Peripheral::BSRR::set(pin); }

        void clear() { Peripheral::BSR::clear(pin); }

        void toggle() { Peripheral::ODR::toggle(pin); }
    };

    template <typename PeripheralType, auto num>
    struct Alternate : PinBase<PeripheralType, num> {
        using Peripheral = PeripheralType;
        static auto const pin = num;

        Alternate() { setMode<Peripheral, pin>(Mode::Alternative); }
    };

    template <typename PeripheralType, auto num>
    struct Analog : PinBase<PeripheralType, num> {
        using Peripheral = PeripheralType;
        static auto const pin = num;

        Analog() { setMode<Peripheral, pin>(Mode::Analog); }
    };

    template <typename PinType>
    struct Pin : PinType {
        Pin() {
            // PinType::Peripheral::LCKR::set(PinType::pin);
        }

        ~Pin() {
            // PinType::Peripheral::LCKR::clear(PinType::pin);
        }
    };

    template <typename Peripheral>
    struct Port {
        constexpr Port() { ClockEnable<Peripheral>::Field::write(1); }

        ~Port() { ClockEnable<Peripheral>::Field::write(0); }

        template <template <typename, auto> typename PinType, auto pin>
        Pin<PinType<Peripheral, pin>> createPin() {
            return {};
        }

        template <auto pin>
        auto output() {
            return createPin<Output, pin>();
        }

        template <auto pin>
        auto input() {
            return createPin<Input, pin>();
        }

        template <auto pin>
        auto alternate() {
            return createPin<Alternate, pin>();
        }

        template <auto pin>
        auto analog() {
            return createPin<Analog, pin>();
        }
    };
} // namespace Gpio

template <typename Mcu>
struct ClockEnableImpl<Mcu, typename Mcu::GPIOA> {
    using Field = typename Mcu::RCC::IOPENR::IOPAEN;
};

template <typename Mcu>
struct ClockEnableImpl<Mcu, typename Mcu::GPIOB> {
    using Field = typename Mcu::RCC::IOPENR::IOPBEN;
};

template <typename Mcu>
struct ClockEnableImpl<Mcu, typename Mcu::GPIOC> {
    using Field = typename Mcu::RCC::IOPENR::IOPCEN;
};

template <typename Mcu>
struct ClockEnableImpl<Mcu, typename Mcu::GPIOD> {
    using Field = typename Mcu::RCC::IOPENR::IOPDEN;
};

template <typename Mcu>
struct ClockEnableImpl<Mcu, typename Mcu::GPIOE> {
    using Field = typename Mcu::RCC::IOPENR::IOPEEN;
};
