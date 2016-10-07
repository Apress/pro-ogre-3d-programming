#ifndef _SINGLETON_H__
#define _SINGLETON_H__

// Copied frome Ogre::Singleton, created by Steve Streeting for Ogre

namespace OgreMayaExporter 
{
    /** Template class for creating single-instance global classes.
    */
    template <typename T> class Singleton
    {
    protected:
        static T* ms_Singleton;

    public:
        Singleton(){
            assert( !ms_Singleton );
		    ms_Singleton = static_cast< T* >( this );
        }
        ~Singleton(){
			assert( ms_Singleton );
			ms_Singleton = 0;  
		}
		static T& getSingleton(){
			assert( ms_Singleton );  
			return ( *ms_Singleton ); 
		}
        static T* getSingletonPtr(){ 
			return ms_Singleton; 
		}
    };

}; // end namespace
#endif