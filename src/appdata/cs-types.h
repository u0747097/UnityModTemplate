#pragma once

#define FUNC0(R) Func0<R>
#define FUNC1(A1, R) Func1<A1, R>
#define FUNC2(A1, A2, R) Func2<A1, A2, R>
#define FUNC3(A1, A2, A3, R) Func3<A1, A2, A3, R>
#define FUNC4(A1, A2, A3, A4, R) Func4<A1, A2, A3, A4, R>
#define FUNC5(A1, A2, A3, A4, A5, R) Func5<A1, A2, A3, A4, A5, R>
#define FUNC6(A1, A2, A3, A4, A5, A6, R) Func6<A1, A2, A3, A4, A5, A6, R>
#define ACTION0 Action
#define ACTION1(T1) Action1<T1>
#define ACTION2(T1, T2) Action2<T1, T2>
#define ACTION3(T1, T2, T3) Action3<T1, T2, T3>
#define ACTION4(T1, T2, T3, T4) Action4<T1, T2, T3, T4>
#define ACTION5(T1, T2, T3, T4, T5) Action5<T1, T2, T3, T4, T5>
#define ACTION6(T1, T2, T3, T4, T5, T6) Action6<T1, T2, T3, T4, T5, T6>
#define VALUETUPLE2(T1, T2) ValueTuple2<T1, T2>

// Func<TResult>
template <typename TResult>
struct Func0 : UnityResolve::UnityType::Object
{
    TResult Invoke()
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`1")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this);
        return TResult();
    }
};

// Func<T, TResult>
template <typename T1, typename TResult>
struct Func1 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`2")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1);
        return TResult();
    }
};

// Func<T1, T2, TResult>
template <typename T1, typename T2, typename TResult>
struct Func2 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1, T2 a2)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`3")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1, a2);
        return TResult();
    }
};

// Func<T1, T2, T3, TResult>
template <typename T1, typename T2, typename T3, typename TResult>
struct Func3 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1, T2 a2, T3 a3)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`4")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1, a2, a3);
        return TResult();
    }
};

// Func<T1, T2, T3, T4, TResult>
template <typename T1, typename T2, typename T3, typename T4, typename TResult>
struct Func4 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1, T2 a2, T3 a3, T4 a4)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`5")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1, a2, a3, a4);
        return TResult();
    }
};

// Func<T1, T2, T3, T4, T5, TResult>
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename TResult>
struct Func5 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`6")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1, a2, a3, a4, a5);
        return TResult();
    }
};

// Func<T1, T2, T3, T4, T5, T6, TResult>
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename TResult>
struct Func6 : UnityResolve::UnityType::Object
{
    TResult Invoke(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Func`7")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) return method->Invoke<TResult>(this, a1, a2, a3, a4, a5, a6);
        return TResult();
    }
};

// Action()
struct Action : UnityResolve::UnityType::Object
{
    void Invoke()
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this);
    }
};

// Action<T1>
template <typename T1>
struct Action1 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`1")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1);
    }
};

// Action<T1, T2>
template <typename T1, typename T2>
struct Action2 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1, T2 a2)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`2")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1, a2);
    }
};

// Action<T1, T2, T3>
template <typename T1, typename T2, typename T3>
struct Action3 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1, T2 a2, T3 a3)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`3")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1, a2, a3);
    }
};

// Action<T1, T2, T3, T4>
template <typename T1, typename T2, typename T3, typename T4>
struct Action4 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1, T2 a2, T3 a3, T4 a4)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`4")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1, a2, a3, a4);
    }
};

// Action<T1, T2, T3, T4, T5>
template <typename T1, typename T2, typename T3, typename T4, typename T5>
struct Action5 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`5")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1, a2, a3, a4, a5);
    }
};

// Action<T1, T2, T3, T4, T5, T6>
template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct Action6 : UnityResolve::UnityType::Object
{
    void Invoke(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
    {
        static UnityResolve::Method* method;
        if (!method)
            method = UnityResolve::Get("mscorlib.dll")
                     ->Get("System.Action`6")
                     ->Get<UnityResolve::Method>("Invoke");
        if (method) method->Invoke<void>(this, a1, a2, a3, a4, a5, a6);
    }
};

// ValueTuple<T1, T2>
template <typename T1, typename T2>
struct ValueTuple2 : UnityResolve::UnityType::Object
{
    T1 Item1;
    T2 Item2;

    T1 GetItem1() { return Item1; }
    T2 GetItem2() { return Item2; }

    static ValueTuple2* New(T1 a1, T2 a2)
    {
        auto klass = UnityResolve::Get("mscorlib.dll")
                     ->Get("System")
                     ->Get<UnityResolve::Class>("ValueTuple`2");
        auto tuple = klass->New<ValueTuple2>();
        tuple->Item1 = a1;
        tuple->Item2 = a2;
        return tuple;
    }
};
