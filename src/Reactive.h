#pragma once

#include "Observable.h"
#include "Binding.h"

namespace react
{
    // TODO: Proper debugging info.
    class AlreadyBoundException : public std::exception
    {
    public:
        AlreadyBoundException()
            : std::exception( "Tried to assign a value to a reactive variable that has already been bound" )
        {}
    };

    /*
     * A reactive is an observable variable that can also be bound
     * to other observables, so that whenever it's dependants change, it does too.
     */
    template <typename Type>
    class Reactive : public Observable<Type>
    {
    public:
        Reactive()
            : value{}, binding( [&]() { invalidate(); }, [&]() { update(); } )
        {
        }
        Reactive( const Type& type )
            : value( type ), binding( [&](){ invalidate(); }, [&](){ update(); } )
        {
        }
        Reactive( Type&& type )
            : value( std::forward<Type>( type ) ), binding( [&](){ invalidate(); }, [&](){ update(); } )
        {
        }
        template <typename SimilarType>
        Reactive( Observable<SimilarType>& observable )
            : Reactive()
        {
            bind( []( SimilarType input ) { return (Type)input; }, observable );
        }
        ~Reactive()
        {
            binding.clear();
        }

        /*
         * Sets the reactive's value to be the aggregate of one or more observables. The reactive keeps this
         * reaction.
         */
        template <typename... Inputs>
        void    bind( typename const Binding<Type>::RelationFunc<Inputs...>& binder, Observable<Inputs>&... inputs )
        {
            binding.reset( binder, inputs... );

            valid = false;
        }

        /*
         * Removes the binding relationship. The reactive value will be left at the most recently set value.
         */
        void    unbind()
        {
            get(); // Ensure we have latest value. This helps ensure a well defined behaviour.

            binding.clear();
        }

        template <typename StandIn>
        void set( StandIn&& standin )
        {
            if( binding )
                throw AlreadyBoundException();

            // This way we retain the old value, but ensure the new value is
            // contained in the class
            Type temp = std::forward<StandIn>( standin );
            std::swap( value, temp );

            change.notify();
            valueChange.notify( temp, value );
        }

        const Type& get() const
        {
            if( !valid )
                const_cast<Reactive<Type>*>(this)->update();

            return value;
        }

        template <typename StandIn>
        const Reactive& operator =( StandIn&& newValue )
        {
            set( std::forward<StandIn>( newValue ) );
            return *this;
        }

        using Observable::operator const Type &;

    private:
        void    update()
        {
            // This way we retain the old value, but ensure the new value is
            // contained in the class
            Type temp = binding();
            std::swap( value, temp );

            valid = true;

            valueChange.notify( temp, value );
        }
        void    invalidate()
        {
            valid = false;
            change.notify();
        }

        Binding<Type>       binding;

        mutable bool        valid = true;
        mutable Type        value;
    };
}