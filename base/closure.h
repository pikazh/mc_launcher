#pragma once

#include <memory>

class Closure
{
public:
    virtual ~Closure(){}

    virtual void run() = 0;

    virtual void cancel()
    {
        cancel_ = true;
    }

    virtual bool is_canceled()
    {
        return cancel_;
    }

private:
    bool cancel_ = false;
};


template<typename Func>
class ClosureForFunc0
    : public Closure
{
public:
    ClosureForFunc0(Func f)
        : f_(f)
    {

    }

    virtual void run() override
    {
        f_();
    }

private:
    Func f_;
};

template<typename Func, typename Arg1>
class ClosureForFunc1
    : public Closure
{
public:
    ClosureForFunc1(Func f, Arg1 arg1)
        : f_(f)
        , arg1_(arg1)
    {

    }

    virtual void run() override
    {
        f_(arg1_);
    }

private:
    Func f_;
    Arg1 arg1_;
};

template<typename Func, typename Arg1, typename Arg2>
class ClosureForFunc2
    : public Closure
{
public:
    ClosureForFunc2(Func f, Arg1 arg1, Arg2 arg2)
        : f_(f)
        , arg1_(arg1)
        , arg2_(arg2)
    {

    }

    virtual void run() override
    {
        f_(arg1_, arg2_);
    }

private:
    Func f_;
    Arg1 arg1_;
    Arg2 arg2_;
};

template<typename Func, typename Arg1, typename Arg2, typename Arg3>
class ClosureForFunc3
    : public Closure
{
public:
    ClosureForFunc3(Func f, Arg1 arg1, Arg2 arg2, Arg3 arg3)
        : f_(f)
        , arg1_(arg1)
        , arg2_(arg2)
        , arg3_(arg3)
    {

    }

    virtual void run() override
    {
        f_(arg1_, arg2_, arg3_);
    }

private:
    Func f_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
};

template<typename MemFunc, typename Cls>
class ClosureForMemberFunc0
    : public Closure
{
public:
    ClosureForMemberFunc0(MemFunc f, std::shared_ptr<Cls> obj)
        : f_(f)
        , weak_obj_(obj)
    {

    }

    virtual void run() override
    {
        auto obj = weak_obj_.lock();
        if (!!obj)
            (obj.get()->*f_)();
    }

private:
    MemFunc f_;
    std::weak_ptr<Cls> weak_obj_;
};

template<typename MemFunc, typename Cls, typename Arg1>
class ClosureForMemberFunc1
    : public Closure
{
public:
    ClosureForMemberFunc1(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1)
        : f_(f)
        , weak_obj_(obj)
        , arg1_(arg1)
    {

    }

    virtual void run() override
    {
        auto obj = weak_obj_.lock();
        if (!!obj)
            (obj.get()->*f_)(arg1_);
    }

private:
    MemFunc f_;
    std::weak_ptr<Cls> weak_obj_;
    Arg1 arg1_;
};

template<typename MemFunc, typename Cls, typename Arg1, typename Arg2>
class ClosureForMemberFunc2
    : public Closure
{
public:
    ClosureForMemberFunc2(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1, Arg2 arg2)
        : f_(f)
        , weak_obj_(obj)
        , arg1_(arg1)
        , arg2_(arg2)
    {

    }

    virtual void run() override
    {
        auto obj = weak_obj_.lock();
        if (!!obj)
            (obj.get()->*f_)(arg1_, arg2_);
    }

private:
    MemFunc f_;
    std::weak_ptr<Cls> weak_obj_;
    Arg1 arg1_;
    Arg2 arg2_;
};

template<typename MemFunc, typename Cls, typename Arg1, typename Arg2, typename Arg3>
class ClosureForMemberFunc3
    : public Closure
{
public:
    ClosureForMemberFunc3(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1, Arg2 arg2, Arg3 arg3)
        : f_(f)
        , weak_obj_(obj)
        , arg1_(arg1)
        , arg2_(arg2)
        , arg3_(arg3)
    {

    }

    virtual void run() override
    {
        auto obj = weak_obj_.lock();
        if (!!obj)
            (obj.get()->*f_)(arg1_, arg2_, arg3_);
    }

private:
    MemFunc f_;
    std::weak_ptr<Cls> weak_obj_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
};

template <typename Func>
std::shared_ptr<Closure> make_closure(Func f)
{
    return std::make_shared<ClosureForFunc0<Func>>(f);
}

template <typename Func, typename Arg1>
std::shared_ptr<Closure> make_closure(Func f, Arg1 arg1)
{
    return std::make_shared<ClosureForFunc1<Func, Arg1>>(f, arg1);
}

template <typename Func, typename Arg1, typename Arg2>
std::shared_ptr<Closure> make_closure(Func f, Arg1 arg1, Arg2 arg2)
{
    return std::make_shared<ClosureForFunc2<Func, Arg1, Arg2>>(f, arg1, arg2);
}

template <typename Func, typename Arg1, typename Arg2, typename Arg3>
std::shared_ptr<Closure> make_closure(Func f, Arg1 arg1, Arg2 arg2, Arg3 arg3)
{
    return std::make_shared<ClosureForFunc3<Func, Arg1, Arg2, Arg3>>(f, arg1, arg2, arg3);
}

template <typename MemFunc, typename Cls>
std::shared_ptr<Closure> make_closure(MemFunc f, std::shared_ptr<Cls> obj)
{
    return std::make_shared<ClosureForMemberFunc0<MemFunc, Cls>>(f, obj);
}

template <typename MemFunc, typename Cls, typename Arg1>
std::shared_ptr<Closure> make_closure(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1)
{
    return std::make_shared<ClosureForMemberFunc1<MemFunc, Cls, Arg1>>(f, obj, arg1);
}

template <typename MemFunc, typename Cls, typename Arg1, typename Arg2>
std::shared_ptr<Closure> make_closure(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1, Arg2 arg2)
{
    return std::make_shared<ClosureForMemberFunc2<MemFunc, Cls, Arg1, Arg2>>(f, obj, arg1, arg2);
}

template <typename MemFunc, typename Cls, typename Arg1, typename Arg2, typename Arg3>
std::shared_ptr<Closure> make_closure(MemFunc f, std::shared_ptr<Cls> obj, Arg1 arg1, Arg2 arg2, Arg3 arg3)
{
    return std::make_shared<ClosureForMemberFunc3<MemFunc, Cls, Arg1, Arg2, Arg3>>(f, obj, arg1, arg2, arg3);
}