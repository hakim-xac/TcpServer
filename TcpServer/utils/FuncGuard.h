#pragma once

#include "utils.h"
#include "helper.h"

#include <type_traits>

namespace kas::utils
{
    template <typename FOut>
        requires std::is_invocable_v<FOut>
    class FuncGuard final {
    public:
        
        template <typename FIn>
            requires std::is_invocable_v<FIn>
        explicit FuncGuard(FIn&& func_in_ctor, FOut func_in_dtor)
            : m_func_in_dtor{ std::move(func_in_dtor) }
        {
            std::invoke(std::forward<FIn>(func_in_ctor));
        }

        FuncGuard(const FuncGuard&) = delete;
        FuncGuard& operator=(const FuncGuard&) = delete;
        FuncGuard(FuncGuard&&) = delete;
        FuncGuard& operator=(FuncGuard&&) = delete;

        ~FuncGuard()
        {
            try {
                m_func_in_dtor();
            }
            catch (const std::exception& ex) {

                kas::utils::logger::LoggerSync{}
                    .print(L"FuncGuard deleter threw:" + helper::toWide(ex.what()));

                std::terminate();
            }
            catch (...) {
                kas::utils::logger::LoggerSync{}
                    .print(L"FuncGuard deleter threw unknown exception");

                std::terminate();
            }
        }

    private:
        FOut m_func_in_dtor;
    };
}