#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
//#include "cinder/gl/GlslProg.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/gl.h"
//#include "cinder/Camera.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"
//#include "cinder/ImageIo.h"
//#include "cinder/Rand.h"

static const int KINECT_X_RES = 640;
static const int KINECT_Y_RES = 480;

using namespace ci;
using namespace ci::app;
using namespace std;

class kinectMesh : public App {
public:
	void prepareSettings(Settings *settings);

	void setup();

	void updateMesh();

	void update();

	void draw();

	void setupMeshRes();

	void setupDepthScale();

	std::shared_ptr<uint16_t> getDepthData();

	// PARAMS
	params::InterfaceGl mParams;
	float avgFramerate;
	float mScale;
	float mXOff, mYOff;
	int mMinDepth, mMaxDepth;
	float mTexOffsetX, mTexOffsetY;
	bool showTexture;
	bool showInfrared, setShowInfrared;
	bool showWireframe, setShowWireframe;

	// CAMERA
	CameraPersp mCam;
	quat mSceneRotation;
	vec3 mEye, mCenter, mUp;
	float mCameraDistance;
	float mKinectTilt;

	// KINECT AND TEXTURES
//  Kinect                    mKinect;
//  gl::Texture               mImageTexture;
	std::shared_ptr<uint16_t> mDepthData;

	int MESH_DIV;
	int setMeshDiv;
	int MESH_X_RES;
	int MESH_Y_RES;
	float *xValues;
	float *yValues;

	float *zValues;
	float depthScale;
	float setDepthScale;

	Color *depthColors;

	// VBO AND SHADER
	//gl::GlslProg	mShader;
	TriMesh mMesh;
};

void prepareSettings(App::Settings *settings) {
	settings->setWindowSize(1280, 720);
}

void kinectMesh::setup() {
	// MESH SETUP
	mMesh = TriMesh(TriMesh::Format().positions().colors(3).texCoords0());

	// SETUP PARAMS
	mParams = params::InterfaceGl("Sembiki Kinect Mesh", vec2(200, 300));
	mParams.addParam("Kinect Tilt", &mKinectTilt, "min=-31 max=31 keyIncr=q keyDecr=a");

	mParams.addParam("Scene Rotation", &mSceneRotation, "opened=1");
	mParams.addParam("Cam Distance", &mCameraDistance, "min=100.0 max=5000.0 step=10.0 keyIncr=s keyDecr=w");
	mParams.addParam("Min Depth", &mMinDepth, "min=0 max=65000 step=100 keyIncr=e keyDecr=d");
	mParams.addParam("Max Depth", &mMaxDepth, "min=0 max=65000 step=100 keyIncr=r keyDecr=f");
	mParams.addParam("Depth Scale", &setDepthScale, "min=0 max=5 step=.01 keyIncr=t keyDecr=g");

	mParams.addParam("Mesh Size", &setMeshDiv, "min=1 max=100 step=1 keyIncr=u keyDecr=j");
	mParams.addParam("Show Wireframe", &setShowWireframe, "");
	mParams.addParam("Show Texture", &showTexture, "");
	mParams.addParam("Show Infrared", &setShowInfrared, "");
	mParams.addParam("RGB Offset X", &mTexOffsetX, "min=-.5 max=.5 step=.001 keyIncr=i keyDecr=k");
	mParams.addParam("RGB Offset Y", &mTexOffsetY, "min=-.5 max=.5 step=.001 keyIncr=o keyDecr=l");

	mParams.addParam("Framerate", &avgFramerate, "min=0 max=120 step=1");

	avgFramerate = 0.0;

	// SETUP CAMERA
	mCameraDistance = 1000.0f;
	mEye = vec3(0.0f, 0.0f, mCameraDistance);
	mCenter = vec3(0);
	mUp = vec3(0, 1, 0);
	mCam.setPerspective(75.0f, getWindowAspectRatio(), 1.0f, 8000.0f);
	mKinectTilt = 0;

	// SETUP KINECT AND TEXTURES
	// console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;
//  mKinect     = Kinect( Kinect::Device() ); // use the default Kinect
	mMinDepth = 0;
	mMaxDepth = 64000;
	mTexOffsetX = -0.024;
	mTexOffsetY = 0.038;
	showTexture = false;
	showInfrared = false;
	setShowInfrared = false;
	showWireframe = false;
	setShowWireframe = false;

	MESH_DIV = 3;
	setMeshDiv = MESH_DIV;
	setupMeshRes();

	depthScale = 1;
	setDepthScale = depthScale;
	setupDepthScale();

	depthColors = new Color[66000];
	for (int i = 0; i < 66000; ++i) {
		float hue = 1.0f - (float) i / 66000.0f + 0.5f;
		if (hue > 1.0f)
			hue -= 1.0f;
		float sat = 1.0f;
		float val = 0.5f + (float) i / 66000.0f * 0.5f;
		depthColors[i] = Color(CM_HSV, hue, sat, val);
	}

	// SETUP GL
	//mShader	= gl::GlslProg( loadResource( RES_VERT_ID ), loadResource( RES_FRAG_ID ) );
	gl::enableDepthWrite();
	gl::enableDepthRead();

}

void kinectMesh::setupMeshRes() {
	MESH_X_RES = KINECT_X_RES / MESH_DIV;
	MESH_Y_RES = KINECT_Y_RES / MESH_DIV;

	// X and Y lookup table
	xValues = new float[MESH_X_RES];
	for (int i = 0; i < MESH_X_RES; ++i) {
		xValues[i] = (float) i / (float) MESH_X_RES * 800.0f - 400.0f;
	}
	yValues = new float[MESH_Y_RES];
	for (int i = 0; i < MESH_Y_RES; ++i) {
		yValues[i] = 300.0f - (float) i / (float) MESH_Y_RES * 600.0f;
	}

}

void kinectMesh::setupDepthScale() {
	// Depth lookup table
	zValues = new float[66000];
	for (int i = 0; i < 66000; ++i) {
		zValues[i] = (float) i / 75 * depthScale;
	}
}

void kinectMesh::updateMesh() {
	for (int y = 0; y < MESH_Y_RES; ++y) {
		for (int x = 0; x < MESH_X_RES; ++x) {


			int depth = mDepthData.get()[x * MESH_DIV * KINECT_Y_RES + y * MESH_DIV];

			float zPos = zValues[mMinDepth];
			if (mMinDepth <= depth && depth <= mMaxDepth) {
				zPos = zValues[depth];
				mMesh.appendPosition(vec3(xValues[x], yValues[y], zPos));
				mMesh.appendColorRgb(depthColors[depth]);
			}
//			   if (showTexture) {
//			     if (showInfrared)
//			       mMesh.appendTexCoord( vec2((float)x/(float)MESH_X_RES, (float)y/(float)MESH_Y_RES) );
//			     else
//			       mMesh.appendTexCoord( vec2((float)x/(float)MESH_X_RES+mTexOffsetX, (float)y/(float)MESH_Y_RES+mTexOffsetY) );
//			   } else {
			// Rainbow mesh
//			   }

			if (x > 0 && y > 0) {
				// Two triangles per square
				int idx = mMesh.getNumVertices() - 1;
				mMesh.appendTriangle(idx - 1 - MESH_X_RES, idx - MESH_X_RES, idx); // \|.
				mMesh.appendTriangle(idx - 1 - MESH_X_RES, idx - 1, idx); // |\.
			}

		}
	}

}

void kinectMesh::update() {
	// if( mKinectTilt != mKinect.getTilt() )
	//   mKinect.setTilt( mKinectTilt );

	// if( !mKinect.checkNewDepthFrame() )
	//   return;

	if (MESH_DIV != setMeshDiv) {
		MESH_DIV = setMeshDiv;
		setupMeshRes();
	}

	if (depthScale != setDepthScale) {
		depthScale = setDepthScale;
		setupDepthScale();
	}

	//  mDepthSurface = mKinect.getDepthImage();
//  mDepthData = mKinect.getDepthData();
	mDepthData = getDepthData();

	if (showInfrared != setShowInfrared) {
		showInfrared = setShowInfrared;
		//   mKinect.setVideoInfrared(showInfrared);
	}

	if (showWireframe != setShowWireframe) {
		showWireframe = setShowWireframe;
		if (showWireframe)
			gl::enableWireframe();
		else
			gl::disableWireframe();
	}

//  if( showTexture && mKinect.checkNewVideoFrame() )
	//   mImageTexture = gl::Texture(mKinect.getVideoImage());

	// Update mesh with new webcam capture frame
	mMesh.clear();
	updateMesh();

	// update camera information
	mEye = vec3(0.0f, 0.0f, mCameraDistance);
	mCam.lookAt(mEye, mCenter, mUp);
	gl::setMatrices(mCam);
	gl::rotate(mSceneRotation);
}

void kinectMesh::draw() {
	gl::clear(Color(0.0f, 0.0f, 0.0f), true);
	if (showTexture) {
//    mImageTexture.enableAndBind();
		gl::draw(mMesh);
		//   mImageTexture.unbind();
	} else {
//    gl::draw(mMesh);
	}

	avgFramerate = getAverageFps();
	mParams.draw();
}

std::shared_ptr<uint16_t> kinectMesh::getDepthData() {
	const int PLANEDEPTH = 50000;
	std::shared_ptr<uint16_t> depthData(new uint16_t[KINECT_X_RES * KINECT_Y_RES]);
	int depth;
	//std::shared_ptr<uint16_t> depthData = new std::shared_ptr<uint16_t>[MESH_X_RES*MESH_Y_RES];

	for (int i = 0; i < KINECT_X_RES; ++i)
		for (int j = 0; j < KINECT_Y_RES; ++j) {
			depth = 0;
			if (i > KINECT_X_RES / 6 && i < 5 * KINECT_X_RES / 6 && j > KINECT_Y_RES / 6 && j < 5 * KINECT_Y_RES / 6) {

				int x2 = (KINECT_X_RES / 2 - i) * (KINECT_X_RES / 2 - i);
				int y2 = (KINECT_Y_RES / 2 - j) * (KINECT_Y_RES / 2 - j);
				depth = PLANEDEPTH - x2 / 3 - y2 / 3;
			}

			//if (i < MESH_X_RES / 2 && j < MESH_Y_RES / 2)
			depthData.get()[i * KINECT_Y_RES + j] = static_cast<uint16_t>(depth);
		}
	return depthData;
}

CINDER_APP(kinectMesh, RendererGl, prepareSettings)