/*
 *  ofxDepthImageRecorder.cpp
 *  PointcloudWriter
 *
 *  Created by Jim on 10/20/11.
 *  Copyright 2011 University of Washington. All rights reserved.
 *
 */

#include "ofxDepthImageRecorder.h"

#pragma mark Thread Implementation
void ofxRGBDEncoderThread::threadedFunction(){
	while(isThreadRunning()){
		delegate->encoderThreadCallback();
	}
}

void ofxRGBDRecorderThread::threadedFunction(){
	while(isThreadRunning()){
		delegate->recorderThreadCallback();
	}
}

#pragma mark Thread Implementation
ofxDepthImageRecorder::ofxDepthImageRecorder()
  : recorderThread(this),
	encoderThread(this)
{
	recording = false;
	lastFramePixs = NULL;
	encodingBuffer = NULL;
	framesToCompress = 0;
}

ofxDepthImageRecorder::~ofxDepthImageRecorder(){
	if(lastFramePixs != NULL){
		delete lastFramePixs;
	}
}

void ofxDepthImageRecorder::setup(){
    folderCount = 0;
	currentFrame = 0;
	
	lastFramePixs = new unsigned short[640*480];
	memset(lastFramePixs, 0, sizeof(unsigned short)*640*480);

    recorderThread.startThread(true, false);
	encoderThread.startThread(true, false);
}

void ofxDepthImageRecorder::setRecordLocation(string directory, string filePrefix){
	targetDirectory = directory;
	ofDirectory dir(directory);
	if(!dir.exists()){
		dir.create(true);
	}
	
	targetFilePrefix = filePrefix;
	
	//convert any hanging uncoverted take files
	vector<string> takePaths = getTakePaths();
	encoderThread.lock();
	for(int i = 0; i < takePaths.size(); i++){
		encodeDirectories.push( takePaths[i] );
	}
	encoderThread.unlock();	
}

vector<string> ofxDepthImageRecorder::getTakePaths(){
	ofDirectory dir = ofDirectory(targetDirectory);
	dir.listDir();
	dir.sort();
	vector<string> paths;
	for(int i = 0; i < dir.numFiles(); i++){
		paths.push_back(dir.getPath(i));
	}
	return paths;
}

bool ofxDepthImageRecorder::addImage(ofShortPixels& image){
	addImage(image.getPixels());
}

bool ofxDepthImageRecorder::addImage(unsigned short* image){
	//confirm that it isn't a duplicate of the most recent frame;
	int framebytes = 640*480*sizeof(unsigned short);
	if(0 != memcmp(image, lastFramePixs, framebytes)){
		QueuedFrame frame;
		frame.timestamp = ofGetElapsedTimeMillis() - recordingStartTime;
		frame.pixels = new unsigned short[640*480];
		memcpy(frame.pixels, image, framebytes);
		memcpy(lastFramePixs, image, framebytes);
		
		char filenumber[512];
		sprintf(filenumber, "%05d", currentFrame); 
		
		char millisstring[512];
		sprintf(millisstring, "%010d", frame.timestamp);
		frame.filename = targetFilePrefix + "_" + filenumber +  "_millis_" + millisstring + ".raw";
		frame.directory = targetDirectory +  "/" + currentFolderPrefix + "/";
				
		recorderThread.lock();
		saveQueue.push( frame );
		recorderThread.unlock();
		
		currentFrame++;
		return true;
	}
	return false;
}

int ofxDepthImageRecorder::numFramesWaitingSave(){
	return saveQueue.size();
}

int ofxDepthImageRecorder::numFramesWaitingCompession(){
	return framesToCompress;
}

int ofxDepthImageRecorder::numDirectoriesWaitingCompression(){
	return encodeDirectories.size();
}

void ofxDepthImageRecorder::toggleRecord(){
	recording = !recording;
	if(recording){
		incrementTake();		
	}
	else {
		compressCurrentTake();
	}
}

bool ofxDepthImageRecorder::isRecording(){
	return recording;
}

//start converting the current directory
void ofxDepthImageRecorder::compressCurrentTake(){
	if(currentFolderPrefix != ""){
		encoderThread.lock();
		encodeDirectories.push( targetDirectory + "/" + currentFolderPrefix );
		encoderThread.unlock();
	}	
}

void ofxDepthImageRecorder::incrementTake(){
	char takeString[1024] ;
	sprintf(takeString, "TAKE_%02d_%02d_%02d_%02d_%02d", ofGetMonth(), ofGetDay(), ofGetHours(), ofGetMinutes(), ofGetSeconds());
    currentFolderPrefix = string(takeString);
    ofDirectory dir(targetDirectory + "/" + currentFolderPrefix);
    
	if(!dir.exists()){
		dir.create(true);
	}
	
    currentFrame = 0;	
	recordingStartTime = ofGetElapsedTimeMillis();
}

void ofxDepthImageRecorder::shutdown(){
	recorderThread.stopThread(true);
	encoderThread.stopThread(true);
}
											  
void ofxDepthImageRecorder::recorderThreadCallback(){

	QueuedFrame frame;
	bool foundFrame = false;
	recorderThread.lock();
	if(saveQueue.size() != 0){
		frame = saveQueue.front();
		saveQueue.pop();
		foundFrame = true;
	}
	recorderThread.unlock();
	
	if(foundFrame){
		char filenumber[512];
		sprintf(filenumber, "%05d", currentFrame); 
		if(compressor.saveToRaw(frame.directory+frame.filename, frame.pixels)){
			delete frame.pixels;
		}
		else {
			//if the save fils push it back on tehs tack
			recorderThread.lock();
			saveQueue.push(frame);
			recorderThread.unlock();
			ofLogError("ofxDepthImageRecorder -- Save Failed! readding to queue");
		}
	}
	ofSleepMillis(2);
}

void ofxDepthImageRecorder::encoderThreadCallback(){
	string dir;
	bool foundDir = false;

	encoderThread.lock();
	if(encodeDirectories.size() != 0){
		foundDir = true;
		dir = encodeDirectories.front();
		encodeDirectories.pop();
	}
	encoderThread.unlock();

	if(foundDir){
		//start to convert
		ofDirectory rawDir(dir);
		rawDir.allowExt("raw");
		rawDir.allowExt("xkcd");
		rawDir.listDir();
		if(encodingBuffer == NULL){
			encodingBuffer = new unsigned short[640*480];
		}
		cout << "ofxDepthImageCompressor -- Starting to convert " << rawDir.numFiles() << " in " << dir << endl;
		framesToCompress = rawDir.numFiles();
		for(int i = 0; i < rawDir.numFiles(); i++){
			//don't do this while recording
			while(recording){
				ofSleepMillis(250);
			}
			string path = rawDir.getPath(i);
			compressor.readDepthFrame(path, encodingBuffer);
			compressor.saveToCompressedPng(ofFilePath::removeExt(path)+".png", encodingBuffer);
			rawDir.getFile(i, ofFile::ReadOnly, true).remove();
			framesToCompress--;
		}
	}
	ofSleepMillis(2);
}

