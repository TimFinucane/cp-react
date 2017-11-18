#pragma once

#include <functional>

#include "Observable.h"

namespace react
{
    /*
     * A connection array holds the connections for a reactive binding, and
     * will call a given functor whenever any connection changes. It will also
     * destroy every connection if one gets destroyed. This can be listened to.
     */
    struct ConnectionArray
    {
        using Connection = events::AutoConnection<>;
    public:
        ConnectionArray( const std::function<ChangeObserver>& onChange, const std::function<void()>& onClose = {} )
            : onChange( onChange ), count( 0 ), connections( nullptr ), onClose( onClose )
        {
        }
        ~ConnectionArray()
        {
            clear();
        }

        ConnectionArray( ConnectionArray&& m ) = delete;
        ConnectionArray& operator =( ConnectionArray&& m ) = delete;

        template <typename... Args>
        void reset( Observable<Args>&... listenables )
        {
            clear();

            count = sizeof...(Args);
            connections = static_cast<Connection*>(::operator new(sizeof( Connection ) * count));

            bindListeners( 0, listenables... );
        }
        void clear()
        {
            if( count > 0 )
            {
                count = 0;
                delete connections;
                if( onClose )
                    onClose();
            }
        }

    private:
        template <typename Arg, typename... Args>
        void        bindListeners( int i, Observable<Arg>& listenable, Observable<Args>&... listenables )
        {
            new (connections + i) Connection( listenable.addListener( onChange, [&](){ clear(); } ) );

            bindListeners<Args...>( i + 1, listenables... );
        }
        template <typename T = void>
        void        bindListeners( int ) {}

        std::function<ChangeObserver>   onChange;

        Connection* connections;
        size_t      count;

        std::function<void()>   onClose; // From what i understand, these functions are allowed to destroy themselves
    };

    template <typename Type>
    struct identity
    {
        using type = Type;
    };

    /*
     * A binding is the relationship that connects
     * inputs of observables with a function
     */
    template <typename Out>
    class Binding
    {
        template <typename... Inputs>
        using RelationFunc = typename identity<std::function<Out( const Inputs&... )>>::type;
    public:
        Binding( const std::function<ChangeObserver>& onChange, const std::function<void()>& onClose = {} )
            : connections( onChange, onClose )
        {
        }

        template <typename... Inputs>
        void reset( typename const RelationFunc<Inputs...>& binder, Observable<Inputs>&... inputs )
        {
            response = [binder, &inputs...]() -> Out{ return binder( inputs.get()... ); };
            connections.reset( inputs... );
        }

        void clear()
        {
            connections.clear();
        }

        Out value()
        {
            return response();
        }
        Out operator()()
        {
            return response();
        }

    private:
        std::function<Out()>    response;
        ConnectionArray         connections;
    };
}