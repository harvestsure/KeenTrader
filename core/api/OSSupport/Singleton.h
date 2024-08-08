#pragma once

template <class T>
class Singleton
{
public:
    static T &get_instance()
    {
        static std::unique_ptr<T> instance;
        if (!instance)
        {
            instance = std::make_unique<T>();
        }
        return *instance;
    }

    DISALLOW_COPY_AND_ASSIGN(Singleton);

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};
