// STM32L0x3 Clock Control driver
//
// Author: Matthew Knight
// File Name: clock-control.hpp
// Date: 2019-08-18

#pragma once

#include "register-alias/util.hpp"

#include <algorithm>
#include <iterator>
#include <ratio>

namespace {
    constexpr std::uint32_t pll_muls[] = {3, 4, 6, 8, 12, 16, 24, 32, 48};
    constexpr std::uint32_t pll_divs[] = {2, 3, 4};
    constexpr std::uint32_t ahb_divs[] = {1};
    constexpr std::uint32_t apb_divs[] = {1};

    template <typename Iterator, typename Predicate>
    constexpr bool any_of(Iterator first, Iterator last, Predicate pred) {
        for (; first != last; ++first)
            if (pred(*first))
                return true;

        return false;
    }

    template <typename Iterator, typename Value>
    constexpr Iterator find(Iterator first, Iterator last, Value val) {
        for (; first != last; ++first)
            if (*first == val)
                return first;

        return first;
    }

    template <typename Array>
    constexpr auto array_index(Array const& arr, std::uint32_t val) {
        return std::distance(std::begin(arr),
                             find(std::begin(arr), std::end(arr), val));
    }

    template <typename Field, auto val>
    struct WriteField {
        static void execute() { Field::write(val); }
    };

    template <typename Field, auto val>
    struct FieldEquals {
        static void execute() {
            while (Field::read() != val)
                ;
        }
    };

    template <typename ModifyAction, typename WaitFor>
    static void modify_wait() {
        ModifyAction::execute();
        WaitFor::execute();
    }
} // namespace

using MaxSysClkFrequency = std::ratio<32000000, 1>;

template <typename Mcu>
struct Clock {
    enum class SysClkSource { Msi, Hsi16, Hse, Pll };
    enum class PllSource { Hsi16, Hse };

    template <typename Source, auto mul, auto div>
    struct Pll {
        static_assert(any_of(std::begin(pll_muls), std::end(pll_muls),
                             [](auto& val) { return val == mul; }),
                      "Invalid PLL multiplier value");
        static_assert(any_of(std::begin(pll_divs), std::end(pll_divs),
                             [](auto& val) { return val == div; }),
                      "Invalid PLL divider value");

        using Frequency = std::ratio_multiply<typename Source::Frequency,
                                              std::ratio<mul, div>>;
        static auto const sysclk_value = SysClkSource::Pll;

        static void init() {
            if (static_cast<SysClkSource>(Mcu::RCC::CFGR::SW::read()) ==
                SysClkSource::Pll) {
                modify_wait<
                    WriteField<typename Mcu::RCC::CFGR::SW,
                               static_cast<std::uint32_t>(SysClkSource::Hsi16)>,
                    FieldEquals<typename Mcu::RCC::CFGR::SWS,
                                static_cast<std::uint32_t>(
                                    SysClkSource::Hsi16)>>();
            }

            modify_wait<WriteField<typename Mcu::RCC::CR::PLLON, 0>,
                        FieldEquals<typename Mcu::RCC::CR::PLLRDY, 0>>();

            Mcu::Flash::ACR::LATENCY::write(1);
            Mcu::RCC::CFGR::PLLMUL::write(array_index(pll_muls, mul));
            Mcu::RCC::CFGR::PLLDIV::write(array_index(pll_divs, div));
            Mcu::RCC::CFGR::PLLSRC::write(
                static_cast<std::uint32_t>(Source::pll_value));

            Source::init();

            // Pll enable
            modify_wait<WriteField<typename Mcu::RCC::CR::PLLON, 1>,
                        FieldEquals<typename Mcu::RCC::CR::PLLRDY, 1>>();
        }
    };

    template <bool div_four = false>
    struct Hsi16 {
        using Frequency = std::conditional_t<div_four, std::ratio<4000000, 1>,
                                             std::ratio<16000000, 1>>;
        static auto const sysclk_value = SysClkSource::Hsi16;
        static auto const pll_value = PllSource::Hsi16;

        static void init() {
            Mcu::RCC::CR::HSI16DIVEN::write(div_four);

            modify_wait<WriteField<typename Mcu::RCC::CR::HSI16ON, 1>,
                        FieldEquals<typename Mcu::RCC::CR::HSI16RDYF, 1>>();
        }
    };

    template <typename Source>
    struct SysClk {
        using Frequency = typename Source::Frequency;

        static_assert(std::ratio_less_equal_v<Frequency, MaxSysClkFrequency>,
                      "SYSCLK is set too high (max is 32MHz)");

        static void init() {
            Source::init();
            modify_wait<
                WriteField<typename Mcu::RCC::CFGR::SW,
                           static_cast<std::uint32_t>(Source::sysclk_value)>,
                FieldEquals<typename Mcu::RCC::CFGR::SWS,
                            static_cast<std::uint32_t>(
                                Source::sysclk_value)>>();
        }
    };

    // Source should be SysClk
    template <typename Source, auto div>
    struct Ahb {
        static_assert(any_of(std::begin(ahb_divs), std::end(ahb_divs),
                             [](auto& elem) { return elem == div; }),
                      "Invalid AHB prescaler value");

        using Frequency =
            std::ratio_multiply<typename Source::Frequency, std::ratio<1, div>>;
        static void init() {
            // set register -- optimize for 1
        }
    };

    // Source should be Ahb
    template <typename Source, auto div>
    struct Apb1 {
        using Frequency =
            std::ratio_multiply<typename Source::Frequency, std::ratio<1, div>>;
        static void init() {}
    };

    // Source should be Ahb
    template <typename Source, auto div>
    struct Apb2 {
        using Frequency =
            std::ratio_multiply<typename Source::Frequency, std::ratio<1, div>>;
        static void init() {}
    };
};
