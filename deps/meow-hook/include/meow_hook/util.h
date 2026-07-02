#pragma once

#include <cstdint>
#include <type_traits>

namespace meow_hook
{

template <typename R> static R func_call(uintptr_t addr)
{
    return ((R(*)())(addr))();
}

template <typename R, typename... Args> struct func_call_member_helper {
    using func_t = R (func_call_member_helper::*)(Args...) const;

    R operator()(func_t func_ptr, Args... args) const
    {
        return (this->*func_ptr)(args...);
    }
};

template <typename R, class T, typename... Args>
R func_call_member(uintptr_t addr, T _this, Args... args)
{
    const func_call_member_helper<R, Args...> *helper =
        reinterpret_cast<const func_call_member_helper<R, Args...> *>(_this);
    auto           func_ptr = *(typename std::remove_pointer_t<decltype(helper)>::func_t *)(&addr);
    return helper->operator()(func_ptr, args...);
}

template <typename R, class T, typename... Args>
inline std::enable_if_t<!std::is_pointer_v<T> || !std::is_class_v<std::remove_pointer_t<T>>, R>
func_call(uintptr_t addr, T _this, Args... args)
{
    return ((R(*)(T, Args...))(addr))(_this, args...);
}

template <typename R, class T, typename... Args>
inline std::enable_if_t<std::is_pointer_v<T> && std::is_class_v<std::remove_pointer_t<T>>, R>
func_call(uintptr_t addr, T _this, Args... args)
{
    return func_call_member<R, T, Args...>(addr, _this, args...);
}

} // namespace meow_hook
