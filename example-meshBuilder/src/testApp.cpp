/**
 * Example - Mesh Builder
 * This example shows how to create a RGBD Mesh on the CPU
 *
 *
 * James George 2012 
 * Released under the MIT License
 *
 * The RGBDToolkit has been developed with support from the STUDIO for Creative Inquiry and Eyebeam
 */

#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofBackground(25);
    
    //set up the game camera
    cam.setup();
    cam.speed = 20;
    cam.autosavePosition = true;
    cam.targetNode.setPosition(ofVec3f());
    cam.targetNode.setOrientation(ofQuaternion());
    cam.targetXRot = -180;
    cam.targetYRot = 0;
    cam.rotationZ = 0;    
    
	
    simplify = 1;
    xshift = 0;
    yshift = 0;
    
    gui.setup("tests");
    gui.add(xshift.setup("xshift", ofxParameter<float>(), -.15, .15));
    gui.add(yshift.setup("yshift", ofxParameter<float>(), -.15, .15));
    gui.add(simplify.setup("simplify", ofxParameter<int>(), 1, 8));
    gui.add(loadNew.setup("load new"));
           
    gui.loadFromFile("defaultSettings.xml");
    
	meshBuilder.addColors = true;
	meshBuilder.cacheValidVertices = true;
	
    //attemping to load the last scene
    loadDefaultScene();
}

//--------------------------------------------------------------
bool testApp::loadNewScene(){
    ofFileDialogResult r = ofSystemLoadDialog("Select a Scene", true);
    if(r.bSuccess){
        return loadScene(r.getPath());
    }
    return false;
}

//--------------------------------------------------------------
bool testApp::loadDefaultScene(){
    ofxXmlSettings settings;
    if(settings.loadFile("RGBDSimpleSceneDefaults.xml")){
        if(!loadScene(settings.getValue("defaultScene", ""))){
            return loadNewScene();
        }
        return true;
    }
    return loadNewScene();
}

//--------------------------------------------------------------
bool testApp::loadScene(string takeDirectory){
    if(player.setup(takeDirectory)){
        ofxXmlSettings settings;
        settings.loadFile("RGBDSimpleSceneDefaults.xml");
        settings.setValue("defaultScene", player.getScene().mediaFolder);
        settings.saveFile();
        meshBuilder.setup(player.getScene().calibrationFolder);
        
        //populate
        player.getVideoPlayer()->setPosition(.5);
        player.update();
        
        meshBuilder.setXYShift(player.getXYShift());
		meshBuilder.setTexture(*player.getVideoPlayer());
		meshBuilder.setDepthPixels(player.getDepthPixels());
        //this will compensate if we are using an offline video that is of a different scale
//        meshBuilder.setTextureScaleForImage(*player.getVideoPlayer());
        //update the first mesh
        meshBuilder.update(player.getDepthPixels());
        return true;
    }
    return false;
}

//--------------------------------------------------------------
void testApp::update(){
    if(loadNew){
        loadNewScene();
    }
    
    //copy any GUI changes into the mesh builder
    if(meshBuilder.shift.x != xshift || meshBuilder.shift.y != yshift || meshBuilder.getSimplification() != simplify){
        meshBuilder.setXYShift(ofVec2f(xshift,yshift));
        meshBuilder.setSimplification(simplify);
        simplify = meshBuilder.getSimplification();
        meshBuilder.update();
    }
    
    //update the mesh if there is a new depth frame in the player
    player.update();
    if(player.isFrameNew()){
        meshBuilder.update();
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    if(player.isLoaded()){
        cam.begin();
		ofSetColor(255);
        glEnable(GL_DEPTH_TEST);
		ofEnableBlendMode(OF_BLENDMODE_SCREEN);
		player.getVideoPlayer()->getTextureReference().bind();
        meshBuilder.getMesh().drawWireframe();
		player.getVideoPlayer()->getTextureReference().unbind();
		glDisable(GL_DEPTH_TEST);
        cam.end();
    }

    gui.draw();
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    if(key == ' '){
        player.togglePlay();
    }
}

//--------------------------------------------------------------
void testApp::exit(){
    gui.saveToFile("defaultSettings.xml");
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
	ofDirectory dir(dragInfo.files[0]);
	if( dir.isDirectory() && ofxRGBDScene::isFolderValid(dragInfo.files[0]) ){
		loadScene(dragInfo.files[0]);
	}
}
