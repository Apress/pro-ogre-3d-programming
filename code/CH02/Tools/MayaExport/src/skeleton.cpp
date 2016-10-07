#include "skeleton.h"

namespace OgreMayaExporter
{
	// Constructor
	Skeleton::Skeleton()
	{
		m_joints.clear();
		m_animations.clear();
	}


	// Destructor
	Skeleton::~Skeleton()
	{
		clear();
	}


	// Clear skeleton data
	void Skeleton::clear()
	{
		m_joints.clear();
		m_animations.clear();
	}


	// Load skeleton data from given skin cluster
	MStatus Skeleton::load(MFnSkinCluster* pSkinCluster,ParamList& params)
	{
		MStatus stat;
		//update skin cluster pointer
		if (!pSkinCluster)
		{
			std::cout << "Could not load skeleton data, no skin cluster specified\n";
			return MS::kFailure;
		}
		//retrieve and load joints from the skin cluster
		MDagPath jointDag,rootDag;
		MDagPathArray influenceDags;
		int numInfluenceObjs = pSkinCluster->influenceObjects(influenceDags,&stat);
		std::cout << "num influence objects: " << numInfluenceObjs << "\n";
		for (int i=0; i<numInfluenceObjs; i++)
		{
			jointDag = influenceDags[i];
			std::cout << "influence object " << i << ": " << jointDag.fullPathName().asChar() << "\n";
			if (influenceDags[i].hasFn(MFn::kJoint))
			{
				//retrieve root joint
				rootDag = jointDag;
				while (jointDag.length()>0)
				{
					jointDag.pop();
					if (jointDag.hasFn(MFn::kJoint) && jointDag.length()>0)
						rootDag = jointDag;
				}
				//check if skeleton has already been loaded
				bool skip = false;
				for (int j=0; j<m_joints.size() && !skip; j++)
				{
					//skip skeleton if already loaded
					if (rootDag.partialPathName() == m_joints[j].name)
					{
						skip = true;
						std::cout << "joint already loaded: skipped\n";
					}
				}
				//load joints data from root
				if (!skip)
				{
					std::cout <<  "Loading skeleton with root: " << rootDag.partialPathName().asChar() << "...\n";
					MSelectionList selectionList;
					MGlobal::getActiveSelectionList(selectionList);
					// Set Neutral Pose
					//check if we want the skin bind pose
					if (params.neutralPoseFrame == NPT_BINDPOSE)
					{
						// Note: we reset to the bind pose, then get current matrix
						// if bind pose could not be restored we use the current pose as a bind pose
						MGlobal::selectByName(jointDag.partialPathName(),MGlobal::kReplaceList);
						MGlobal::executeCommand("dagPose -r -g -bp");
					}
					//check if we want specified frame as neutral pose
					else if (params.neutralPoseType == NPT_FRAME)
					{
						//set time to desired time
						MTime npTime = (double)params.neutralPoseFrame;
						MAnimControl::setCurrentTime(npTime.as(MTime::kSeconds));
					}
					
					//load joints data
					stat = loadJoint(rootDag,NULL,params);
					if (MS::kSuccess == stat)
						std::cout << "OK\n";
					else
						std::cout << "Failed\n";

					/*
					//restore skeleton to neutral pose
					if (params.neutralPoseFrame == 1)
					{
						MGlobal::executeCommand("dagPose -r -g -bp");
						MGlobal::setActiveSelectionList(selectionList,MGlobal::kReplaceList);
					}
					else if (params.neutralPoseType == 2)
					{
						//set time to desired time
						MTime npTime = (double)params.neutralPoseFrame;
						MAnimControl::setCurrentTime(npTime.as(MTime::kSeconds));
					}
					*/
				}
			}
		}

		return MS::kSuccess;
	}


	// Load a joint
	MStatus Skeleton::loadJoint(MDagPath& jointDag,joint* parent,ParamList& params)
	{
		int i;
		joint newJoint;
		joint* parentJoint = parent;
		// if it is a joint node translate it and then proceed to child nodes, otherwise skip it
		// and proceed directly to child nodes
		if (jointDag.hasFn(MFn::kJoint))
		{
			MFnIkJoint jointFn(jointDag);
			// Display info
			std::cout << "Loading joint: " << jointFn.partialPathName().asChar();
			if (parent)
				std::cout << " (parent: " << parent->name.asChar() << ")\n";
			else
				std::cout << "\n";
			// Get parent index
			int idx=-1;
			if (parent)
			{
				for (i=0; i<m_joints.size() && idx<0; i++)
				{
					if (m_joints[i].name == parent->name)
						idx=i;
				}
			}
			// Get joint matrix
			MMatrix bindMatrix = jointDag.inclusiveMatrix();
			// Calculate scaling factor inherited by parent
			double scale[3];
			if (parent)
			{
				MTransformationMatrix M(parent->worldMatrix);
				M.getScale(scale,MSpace::kWorld);
			}
			else
			{
				scale[0] = 1;
				scale[1] = 1;
				scale[2] = 1;
			}
			// Calculate Local Matrix
			MMatrix localMatrix;
			if (parent)
				localMatrix = bindMatrix * parent->worldMatrix.inverse();
			else
			{	// root node of skeleton
				if (params.exportWorldCoords)
					localMatrix = bindMatrix;
				else
					localMatrix = bindMatrix * jointDag.exclusiveMatrix().inverse();
			}
			// Calculate rotation data
			double qx,qy,qz,qw;
			((MTransformationMatrix)localMatrix).getRotationQuaternion(qx,qy,qz,qw);
			MQuaternion rotation(qx,qy,qz,qw);
			MVector axis;
			double theta;
			rotation.getAxisAngle(axis,theta);
			axis.normalize();
			if (axis.length() < 0.5)
			{
				axis.x = 0;
				axis.y = 1;
				axis.z = 0;
				theta = 0;
			}
			// Set joint info
			newJoint.name = jointFn.partialPathName();
			newJoint.id = m_joints.size();
			newJoint.parentIndex = idx;
			newJoint.worldMatrix = bindMatrix;
			newJoint.localMatrix = localMatrix;
			newJoint.posx = localMatrix(3,0) * scale[0];
			newJoint.posy = localMatrix(3,1) * scale[1];
			newJoint.posz = localMatrix(3,2) * scale[2];
			newJoint.angle = theta;
			newJoint.axisx = axis.x;
			newJoint.axisy = axis.y;
			newJoint.axisz = axis.z;
			newJoint.jointDag = jointDag;
			m_joints.push_back(newJoint);
			// Get pointer to newly created joint
			parentJoint = &newJoint;
		}
		// Load children joints
		for (i=0; i<jointDag.childCount();i++)
		{
			MObject child;
			child = jointDag.child(i);
			MDagPath childDag = jointDag;
			childDag.push(child);
			loadJoint(childDag,parentJoint,params);
		}
		return MS::kSuccess;
	}


	// Load animations
	MStatus Skeleton::loadAnims(ParamList& params)
	{
		MStatus stat;
		int i;
		std::cout << "Loading joint animations...\n";
		// save current time for later restore
		double curtime = MAnimControl::currentTime().as(MTime::kSeconds);
		// clear animations list
		m_animations.clear();
		// load animation clips for the whole skeleton
		for (i=0; i<params.clipList.size(); i++)
		{
			stat = loadClip(params.clipList[i].name,params.clipList[i].start,
				params.clipList[i].stop,params.clipList[i].rate,params);
			if (stat == MS::kSuccess)
				std::cout << "Clip successfully loaded\n";
			else
				std::cout << "Failed loading clip\n";
		}
		//restore current time
		MAnimControl::setCurrentTime(MTime(curtime,MTime::kSeconds));
		return MS::kSuccess;
	}

	
	// Load an animation clip
	MStatus Skeleton::loadClip(MString clipName,double start,double stop,double rate,ParamList& params)
	{
		MStatus stat;
		int i,j;
		MString msg;
		std::vector<double> times;
		// if skeleton has no joint we can't load the clip
		if (m_joints.size() < 0)
			return MS::kFailure;
		// display clip name
		std::cout << "clip \"" << clipName.asChar() << "\"\n";
		// calculate times from clip sample rate
		times.clear();
		for (double t=start; t<stop; t+=rate)
			times.push_back(t);
		times.push_back(stop);
		// get animation length
		double length=0;
		if (times.size() > 0)
			length = times[times.size()-1] - times[0];
		// check if clip length is >0
		if (length <= 0)
		{
			std::cout << "the clip has 0 length, we skip it\n";
			return MS::kFailure;
		}
		// create the animation
		animation a;
		a.name = clipName.asChar();
		a.tracks.clear();
		a.length = length;
		m_animations.push_back(a);
		int animIdx = m_animations.size() - 1;
		// create a track for current clip for all joints
		std::vector<track> animTracks;
		for (i=0; i<m_joints.size(); i++)
		{
			track t;
			t.bone = m_joints[i].name;
			t.keyframes.clear();
			animTracks.push_back(t);
		}
		// evaluate animation curves at selected times
		for (i=0; i<times.size(); i++)
		{
			//set time to wanted sample time
			MAnimControl::setCurrentTime(MTime(times[i],MTime::kSeconds));
			//load a keyframe for every joint at current time
			for (j=0; j<m_joints.size(); j++)
			{
				keyframe key = loadKeyframe(m_joints[j],times[i]-times[0],params);
				//add keyframe to joint track
				animTracks[j].keyframes.push_back(key);
			}
		}
		// add created tracks to current clip
		for (i=0; i<animTracks.size(); i++)
		{
			m_animations[animIdx].tracks.push_back(animTracks[i]);
		}
		// display info
		std::cout << "length: " << m_animations[animIdx].length << "\n";
		std::cout << "num keyframes: " << animTracks[0].keyframes.size() << "\n";
		// clip successfully loaded
		return MS::kSuccess;
	}

	
	// Load a keyframe for a given joint at current time
	keyframe Skeleton::loadKeyframe(joint& j,double time,ParamList& params)
	{
		MFnIkJoint jointFn(j.jointDag);
		MTransformationMatrix matrix;
		MVector position;
		double scale[3];
		scale[0] = 1; scale[1] = 1; scale[2] = 1;
		int parentIdx = j.parentIndex;
		//get joint matrix at current time
		matrix = j.jointDag.inclusiveMatrix();
		if (parentIdx >= 0)
		{
			//calculate inherited scale factor
			((MTransformationMatrix)j.jointDag.exclusiveMatrix()).getScale(scale,MSpace::kWorld);
			//calculate relative matrix
			matrix = j.jointDag.inclusiveMatrix() * j.jointDag.exclusiveMatrixInverse();
		}
		else
		{	// root joint
			if (params.exportWorldCoords)
				matrix = j.jointDag.inclusiveMatrix();
			else
				matrix = j.jointDag.inclusiveMatrix() * j.jointDag.exclusiveMatrixInverse();
		}
		//calculate position of joint at given time
		position.x = matrix.asMatrix()(3,0) * scale[0];
		position.y = matrix.asMatrix()(3,1) * scale[1];
		position.z = matrix.asMatrix()(3,2) * scale[2];
		//get relative transformation matrix
		matrix = matrix.asMatrix() * j.localMatrix.inverse();
		//calculate rotation
		double qx,qy,qz,qw;
		((MTransformationMatrix)matrix).getRotationQuaternion(qx,qy,qz,qw);
		MQuaternion rotation(qx,qy,qz,qw);
		double theta;
		MVector axis;
		rotation.getAxisAngle(axis,theta);
		axis.normalize();
		if (axis.length() < 0.5)
		{
			axis.x = 0;
			axis.y = 1;
			axis.z = 0;
			theta = 0;
		}
		//create keyframe
		keyframe key;
		key.time = time;
		key.tx = position.x - j.posx;
		key.ty = position.y - j.posy;
		key.tz = position.z - j.posz;
		key.angle = theta;
		key.axis_x = axis.x;
		key.axis_y = axis.y;
		key.axis_z = axis.z;
		key.sx = 1;
		key.sy = 1;
		key.sz = 1;
		return key;
	}


	// Get joint list
	std::vector<joint>& Skeleton::getJoints()
	{
		return m_joints;
	}


	// Get animations
	std::vector<animation>& Skeleton::getAnimations()
	{
		return m_animations;
	}


	// Write skeleton data to Ogre XML file
	MStatus Skeleton::writeXML(ParamList &params)
	{
		// Start skeleton description
		params.outSkeleton << "<skeleton>\n";

		// Write bones list
		params.outSkeleton << "\t<bones>\n";
		// For each joint write it's description
		for (int i=0; i<m_joints.size(); i++)
		{
			params.outSkeleton << "\t\t<bone id=\"" << m_joints[i].id << "\" name=\"" << m_joints[i].name.asChar() << "\">\n";
			params.outSkeleton << "\t\t\t<position x=\"" << m_joints[i].posx << "\" y=\"" << m_joints[i].posy
				<< "\" z=\"" << m_joints[i].posz << "\"/>\n";
			params.outSkeleton << "\t\t\t<rotation angle=\"" << m_joints[i].angle << "\">\n";
			params.outSkeleton << "\t\t\t\t<axis x=\"" << m_joints[i].axisx << "\" y=\"" << m_joints[i].axisy
				<< "\" z=\"" << m_joints[i].axisz << "\"/>\n";
			params.outSkeleton << "\t\t\t</rotation>\n";
			params.outSkeleton << "\t\t</bone>\n";
		}
		params.outSkeleton << "\t</bones>\n";

		// Write bone hierarchy
		params.outSkeleton << "\t<bonehierarchy>\n";
		for (i=0; i<m_joints.size(); i++)
		{
			if (m_joints[i].parentIndex>=0)
			{
				params.outSkeleton << "\t\t<boneparent bone=\"" << m_joints[i].name.asChar() << "\" parent=\""
					<< m_joints[m_joints[i].parentIndex].name.asChar() << "\"/>\n";
			}
		}
		params.outSkeleton << "\t</bonehierarchy>\n";

		// Write animations description
		if (params.exportAnims)
		{
			params.outSkeleton << "\t<animations>\n";
			// For every animation
			for (i=0; i<m_animations.size(); i++)
			{
				// Write animation info
				params.outSkeleton << "\t\t<animation name=\"" << m_animations[i].name.asChar() << "\" length=\"" << 
					m_animations[i].length << "\">\n";
				// Write tracks
				params.outSkeleton << "\t\t\t<tracks>\n";
				// Cycle through tracks
				for (int j=0; j<m_animations[i].tracks.size(); j++)
				{
					track t = m_animations[i].tracks[j];
					params.outSkeleton << "\t\t\t\t<track bone=\"" << t.bone.asChar() << "\">\n";
					// Write track keyframes
					params.outSkeleton << "\t\t\t\t\t<keyframes>\n";
					for (int k=0; k<t.keyframes.size(); k++)
					{
						// time
						params.outSkeleton << "\t\t\t\t\t\t<keyframe time=\"" << t.keyframes[k].time << "\">\n";
						// translation
						params.outSkeleton << "\t\t\t\t\t\t\t<translate x=\"" << t.keyframes[k].tx << "\" y=\"" <<
							t.keyframes[k].ty << "\" z=\"" << t.keyframes[k].tz << "\"/>\n";
						// rotation
						params.outSkeleton << "\t\t\t\t\t\t\t<rotate angle=\"" << t.keyframes[k].angle << "\">\n";
						params.outSkeleton << "\t\t\t\t\t\t\t\t<axis x=\"" << t.keyframes[k].axis_x << "\" y=\"" <<
							t.keyframes[k].axis_y << "\" z=\"" << t.keyframes[k].axis_z << "\"/>\n";
						params.outSkeleton << "\t\t\t\t\t\t\t</rotate>\n";
						//scale
						params.outSkeleton << "\t\t\t\t\t\t\t<scale x=\"" << t.keyframes[k].sx << "\" y=\"" <<
							t.keyframes[k].sy << "\" z=\"" << t.keyframes[k].sz << "\"/>\n";
						params.outSkeleton << "\t\t\t\t\t\t</keyframe>\n";
					}
					params.outSkeleton << "\t\t\t\t\t</keyframes>\n";
					params.outSkeleton << "\t\t\t\t</track>\n";
				}
				// End tracks description
				params.outSkeleton << "\t\t\t</tracks>\n";
				// End animation description
				params.outSkeleton << "\t\t</animation>\n";
			}
			params.outSkeleton << "\t</animations>\n";
		}

		// End skeleton description
		params.outSkeleton << "</skeleton>\n";

		return MS::kSuccess;
	}


};	//end namespace