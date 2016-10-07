#include "OgreFrameListener.h"

using namespace Ogre;

class MyFrameListener : public FrameListener {

public:

	// ctor/dtor
	MyFrameListener() { m_timeElapsed = 0.0f; }
	virtual ~MyFrameListener() {}

	// We will provide some meat to this method override
	virtual bool frameStarted(const FrameEvent &evt);

	// We do not need to provide a body for either of these methods, since 
	// Ogre provides a default implementation that does just this. However, for
	// the sake of illustration, we'll provide one here.
	virtual bool frameEnded(const FrameEvent &evt) { return true; }

private:
	float m_timeElapsed;
};